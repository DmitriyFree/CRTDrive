
using Avalonia;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using System;
using System.Collections.Generic;
using System.Linq;

namespace CrtDrive.Services;

public static class VectorImageProcessor
{
    private readonly record struct Point(int X, int Y);

    private sealed class Segment(VectorImageProcessor.Point a, VectorImageProcessor.Point b)
    {
        public Point A = a;
        public Point B = b;
    }
    private sealed class Contour
    {
        public List<Point> Points { get; }
        public bool IsClosed { get; }

        public Contour(List<Point> points, bool isClosed)
        {
            Points = points;
            IsClosed = isClosed;
        }

        public int SegmentCount =>
            IsClosed ? Points.Count : Math.Max(0, Points.Count - 1);
    }

    public static VectorFrame Process(
        byte[,] gray,
        int maxSegments = 100,
        byte threshold = 170,
        float tolerance = 1.5f)
    {
        int h = gray.GetLength(0);
        int w = gray.GetLength(1);

        if (w < 1 || h < 1)
            throw new ArgumentException("Image dimensions must be positive.");

        maxSegments = Math.Max(0, maxSegments);
        tolerance = Math.Max(0, tolerance);

        // Binary foreground mask: brighter pixels are the object.
        bool[,] mask = new bool[h, w];

        for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            mask[y, x] = gray[y, x] >= threshold;

        // Build boundaries only between foreground and background pixels.
        // The image border itself is not considered an object boundary.
        var edges = new List<Segment>();

        for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            if (!mask[y, x])
                continue;

            if (y > 0 && !mask[y - 1, x])
                edges.Add(new(new(x, y), new(x + 1, y)));

            if (x < w - 1 && !mask[y, x + 1])
                edges.Add(new(new(x + 1, y), new(x + 1, y + 1)));

            if (y < h - 1 && !mask[y + 1, x])
                edges.Add(new(new(x + 1, y + 1), new(x, y + 1)));

            if (x > 0 && !mask[y, x - 1])
                edges.Add(new(new(x, y + 1), new(x, y)));
        }

        var contours = TraceContours(edges);

        float currentTolerance = tolerance;
        List<Contour> simplified;

        while (true)
        {
            simplified = contours
                .Select(c =>
                {
                    List<Point> points = c.IsClosed
                        ? SimplifyClosed(c.Points, currentTolerance)
                        : SimplifyOpen(c.Points, currentTolerance);

                    return new Contour(points, c.IsClosed);
                })
                .Where(c => c.SegmentCount > 0)
                .ToList();

            int count = simplified.Sum(c => c.SegmentCount);

            if (count <= maxSegments || currentTolerance >= 200)
                break;

            currentTolerance = currentTolerance == 0
                ? 0.5f
                : currentTolerance * 1.5f;
        }

        // Convert full-resolution contour coordinates into byte coordinates.
        // Contour extraction and simplification happen before this conversion.
        var lines = new List<Esp32ImageSender.LineSegment>();

        foreach (var contour in simplified)
        {
            var points = contour.Points;
            int count = contour.IsClosed ? points.Count : points.Count - 1;

            for (int i = 0; i < count; i++)
            {
                Point a = points[i];
                Point b = points[(i + 1) % points.Count];

                byte x1 = (byte)Math.Clamp(
                    (int)Math.Round(a.X * 255.0 / w), 0, 255);
                byte y1 = (byte)Math.Clamp(
                    (int)Math.Round(a.Y * 255.0 / h), 0, 255);
                byte x2 = (byte)Math.Clamp(
                    (int)Math.Round(b.X * 255.0 / w), 0, 255);
                byte y2 = (byte)Math.Clamp(
                    (int)Math.Round(b.Y * 255.0 / h), 0, 255);

                lines.Add(new(x1, y1, x2, y2));
            }
        }
        
        // Coordinates now occupy a 256x256 preview space.
        return new VectorFrame
        {
            Lines = lines,
            Preview = Render(lines, 256, 256)
        };
    }

    private static List<Contour> TraceContours(List<Segment> edges)
    {
        var outgoing = new Dictionary<Point, List<int>>();
        var incomingCount = new Dictionary<Point, int>();

        for (int i = 0; i < edges.Count; i++)
        {
            Segment edge = edges[i];

            if (!outgoing.TryGetValue(edge.A, out var list))
                outgoing[edge.A] = list = new List<int>();

            list.Add(i);

            incomingCount.TryGetValue(edge.B, out int count);
            incomingCount[edge.B] = count + 1;
        }

        var used = new bool[edges.Count];
        var contours = new List<Contour>();

        void TraceFrom(int first)
        {
            if (used[first])
                return;

            var points = new List<Point>();
            Point start = edges[first].A;
            int current = first;
            bool closed = false;

            for (int guard = 0; guard <= edges.Count; guard++)
            {
                if (used[current])
                    break;

                used[current] = true;

                Segment edge = edges[current];
                points.Add(edge.A);

                Point end = edge.B;

                if (end.Equals(start))
                {
                    closed = true;
                    break;
                }

                if (!outgoing.TryGetValue(end, out var candidates))
                    break;

                int next = -1;

                foreach (int candidate in candidates)
                {
                    if (!used[candidate])
                    {
                        next = candidate;
                        break;
                    }
                }

                if (next < 0)
                    break;

                current = next;
            }

            if (points.Count >= 2)
                contours.Add(new Contour(points, closed));
        }

        // First trace open paths starting at vertices with no incoming edge.
        for (int i = 0; i < edges.Count; i++)
        {
            if (used[i])
                continue;

            Point start = edges[i].A;

            if (!incomingCount.ContainsKey(start))
                TraceFrom(i);
        }

        // Trace any remaining paths and closed loops.
        for (int i = 0; i < edges.Count; i++)
        {
            if (!used[i])
                TraceFrom(i);
        }

        return contours;
    }

    private static List<Point> SimplifyClosed(
        List<Point> points,
        float tolerance)
    {
        if (points.Count <= 3)
            return points;

        // Split the loop at points farthest apart, simplify both arcs,
        // then combine them back into one closed contour.
        int farthest = 0;
        double maxDist = 0;

        for (int i = 1; i < points.Count; i++)
        {
            double d = DistanceSquared(
                points[0].X, points[0].Y,
                points[i].X, points[i].Y);

            if (d > maxDist)
            {
                maxDist = d;
                farthest = i;
            }
        }

        var arc1 = points.Take(farthest + 1).ToList();
        var arc2 = points.Skip(farthest)
            .Concat(points.Take(1))
            .ToList();

        var a = SimplifyOpen(arc1, tolerance);
        var b = SimplifyOpen(arc2, tolerance);

        var result = new List<Point>(a);
        if (b.Count > 2)
            result.AddRange(b.Skip(1).Take(b.Count - 2));

        return result;
    }

    private static List<Point> SimplifyOpen(
        List<Point> points,
        float tolerance)
    {
        if (points.Count <= 2)
            return points;

        double max = 0;
        int index = 0;

        Point a = points[0];
        Point b = points[^1];

        for (int i = 1; i < points.Count - 1; i++)
        {
            double d = PerpendicularDistanceSquared(points[i], a, b);

            if (d > max)
            {
                max = d;
                index = i;
            }
        }

        if (max <= tolerance * tolerance)
            return [a, b];

        var left = SimplifyOpen(points.Take(index + 1).ToList(), tolerance);
        var right = SimplifyOpen(points.Skip(index).ToList(), tolerance);

        left.RemoveAt(left.Count - 1);
        left.AddRange(right);
        return left;
    }

    private static double PerpendicularDistanceSquared(
        Point p, Point a, Point b)
    {
        double dx = b.X - a.X;
        double dy = b.Y - a.Y;

        if (dx == 0 && dy == 0)
            return DistanceSquared(p.X, p.Y, a.X, a.Y);

        double t = ((p.X - a.X) * dx + (p.Y - a.Y) * dy)
                   / (dx * dx + dy * dy);

        t = Math.Clamp(t, 0, 1);

        double px = a.X + t * dx;
        double py = a.Y + t * dy;

        return DistanceSquared(p.X, p.Y, px, py);
    }

    private static double DistanceSquared(
        double x1, double y1, double x2, double y2)
    {
        double dx = x2 - x1;
        double dy = y2 - y1;
        return dx * dx + dy * dy;
    }

    private static WriteableBitmap Render(
        List<Esp32ImageSender.LineSegment> lines,
        int w,
        int h)
    {
        var bmp = new WriteableBitmap(
            new PixelSize(w, h),
            new Vector(96, 96),
            PixelFormat.Bgra8888,
            AlphaFormat.Opaque);

        using var fb = bmp.Lock();
        unsafe
        {
            byte* ptr = (byte*)fb.Address;

            for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                int i = y * fb.RowBytes + x * 4;
                ptr[i] = 0;
                ptr[i + 1] = 0;
                ptr[i + 2] = 0;
                ptr[i + 3] = 255;
            }

            foreach (var line in lines)
            {
                DrawLine(
                    ptr, fb.RowBytes,
                    line.X1, line.Y1,
                    line.X2, line.Y2);
            }
        }

        return bmp;
    }

    private static unsafe void DrawLine(
        byte* ptr, int stride,
        int x0, int y0, int x1, int y1)
    {
        int dx = Math.Abs(x1 - x0);
        int sx = x0 < x1 ? 1 : -1;
        int dy = -Math.Abs(y1 - y0);
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;

        while (true)
        {
            // CRT preview color: orange.
            int i = y0 * stride + x0 * 4;
            ptr[i] = 0;
            ptr[i + 1] = 120;
            ptr[i + 2] = 255;
            ptr[i + 3] = 255;

            if (x0 == x1 && y0 == y1)
                break;

            int e2 = 2 * err;

            if (e2 >= dy)
            {
                err += dy;
                x0 += sx;
            }

            if (e2 <= dx)
            {
                err += dx;
                y0 += sy;
            }
        }
    }
}