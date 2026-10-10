
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

    private sealed class Segment
    {
        public Point A;
        public Point B;

        public Segment(Point a, Point b)
        {
            A = a;
            B = b;
        }
    }

    public static VectorFrame Process(
        byte[,] gray,
        int maxSegments = 100,
        byte threshold = 170,
        float tolerance = 1.5f)
    {
        int h = gray.GetLength(0);
        int w = gray.GetLength(1);

        if (w != 100 || h != 100)
            throw new ArgumentException("Expected a 100x100 image.");

        maxSegments = Math.Max(0, maxSegments);
        tolerance = Math.Max(0, tolerance);

        // Binary foreground mask: brighter pixels are the object.
        bool[,] mask = new bool[h, w];

        for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            mask[y, x] = gray[y, x] >= threshold;

        // Build the boundary of every foreground pixel.
        // Each edge lies on the pixel grid, so coordinates range 0..100.
        var edges = new List<Segment>();

        for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            if (!mask[y, x])
                continue;

            if (y == 0 || !mask[y - 1, x])
                edges.Add(new(new(x, y), new(x + 1, y)));

            if (x == w - 1 || !mask[y, x + 1])
                edges.Add(new(new(x + 1, y), new(x + 1, y + 1)));

            if (y == h - 1 || !mask[y + 1, x])
                edges.Add(new(new(x + 1, y + 1), new(x, y + 1)));

            if (x == 0 || !mask[y, x - 1])
                edges.Add(new(new(x, y + 1), new(x, y)));
        }

        var contours = TraceContours(edges);

        // Simplify the contours more aggressively until the budget fits.
        float currentTolerance = tolerance;
        List<List<Point>> simplified;

        while (true)
        {
            simplified = [.. contours
                .Select(c => SimplifyClosed(c, currentTolerance))
                .Where(c => c.Count >= 2)];

            int count = simplified.Sum(c => c.Count);

            if (count <= maxSegments || currentTolerance >= 200)
                break;

            currentTolerance = currentTolerance == 0
                ? 0.5f
                : currentTolerance * 1.5f;
        }

        // Convert each contour into CRT line segments.
        var lines = new List<Esp32ImageSender.LineSegment>();

        foreach (var contour in simplified)
        {
            for (int i = 0; i < contour.Count; i++)
            {
                Point a = contour[i];
                Point b = contour[(i + 1) % contour.Count];

                lines.Add(new(
                    (byte)Math.Clamp(a.X, 0, 99),
                    (byte)Math.Clamp(a.Y, 0, 99),
                    (byte)Math.Clamp(b.X, 0, 99),
                    (byte)Math.Clamp(b.Y, 0, 99)));
            }
        }

        // Safety cap. Normally simplification should reach this first.
        if (lines.Count > maxSegments)
            lines = [.. lines
                .OrderByDescending(l =>
                    DistanceSquared(l.X1, l.Y1, l.X2, l.Y2))
                .Take(maxSegments)];

        return new VectorFrame
        {
            Lines = lines,
            Preview = Render(lines, w, h)
        };
    }

    private static List<List<Point>> TraceContours(List<Segment> edges)
    {
        var outgoing = new Dictionary<Point, List<int>>();

        for (int i = 0; i < edges.Count; i++)
        {
            if (!outgoing.TryGetValue(edges[i].A, out var list))
                outgoing[edges[i].A] = list = new List<int>();

            list.Add(i);
        }

        var used = new bool[edges.Count];
        var contours = new List<List<Point>>();

        for (int first = 0; first < edges.Count; first++)
        {
            if (used[first])
                continue;

            var contour = new List<Point>();
            int current = first;
            Point start = edges[first].A;

            for (int guard = 0; guard <= edges.Count; guard++)
            {
                if (used[current])
                    break;

                used[current] = true;
                Segment edge = edges[current];
                contour.Add(edge.A);

                Point end = edge.B;

                if (end.Equals(start))
                    break;

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

            if (contour.Count >= 3)
                contours.Add(contour);
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
            return new List<Point> { a, b };

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