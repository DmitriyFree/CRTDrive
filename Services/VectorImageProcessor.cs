
using Avalonia;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using System;
using System.Collections.Generic;
using System.Linq;

namespace CrtDrive.Services;

public static class VectorImageProcessor
{
    private const int AngleCount = 180;
    private const int MinLineLength = 3;
    private const int MinLineVotes = 4;

    private sealed class Candidate
    {
        public int Angle;
        public int Rho;
        public int Votes;
    }

    public static VectorFrame Process(
        byte[,] gray,
        int maxSegments = 100,
        byte threshold = 55,
        float tolerance = 1.5f)
    {
        int h = gray.GetLength(0);
        int w = gray.GetLength(1);

        if (w != 100 || h != 100)
            throw new ArgumentException("Expected a 100x100 image.");

        maxSegments = Math.Clamp(maxSegments, 0, 1000);
        tolerance = Math.Clamp(tolerance, 0.5f, 5f);

        if (maxSegments == 0)
        {
            var empty = new List<Esp32ImageSender.LineSegment>();
            return new VectorFrame
            {
                Lines = empty,
                Preview = Render(empty, w, h)
            };
        }

        // 1. Smooth the image slightly to reduce fine texture.
        byte[,] blurred = Blur(gray);

        // 2. Calculate Sobel gradients.
        float[,] gx = new float[h, w];
        float[,] gy = new float[h, w];
        float[,] magnitude = new float[h, w];

        for (int y = 1; y < h - 1; y++)
        for (int x = 1; x < w - 1; x++)
        {
            int sx =
                -blurred[y - 1, x - 1] + blurred[y - 1, x + 1]
                - 2 * blurred[y, x - 1] + 2 * blurred[y, x + 1]
                - blurred[y + 1, x - 1] + blurred[y + 1, x + 1];

            int sy =
                -blurred[y - 1, x - 1] - 2 * blurred[y - 1, x]
                - blurred[y - 1, x + 1]
                + blurred[y + 1, x - 1] + 2 * blurred[y + 1, x]
                + blurred[y + 1, x + 1];

            gx[y, x] = sx;
            gy[y, x] = sy;
            magnitude[y, x] = MathF.Sqrt(sx * sx + sy * sy);
        }

        // 3. Non-maximum suppression: retain local edge maxima.
        bool[,] edge = new bool[h, w];
        float highThreshold = Math.Max((byte)1, (byte)threshold);
        float lowThreshold = highThreshold * 0.5f;
        bool[,] weak = new bool[h, w];

        for (int y = 1; y < h - 1; y++)
        for (int x = 1; x < w - 1; x++)
        {
            float m = magnitude[y, x];
            if (m < lowThreshold)
                continue;

            float angle = MathF.Atan2(gy[y, x], gx[y, x])
                          * (180f / MathF.PI);

            if (angle < 0)
                angle += 180f;

            int dx, dy;

            if (angle < 22.5f || angle >= 157.5f)
                (dx, dy) = (1, 0);
            else if (angle < 67.5f)
                (dx, dy) = (1, 1);
            else if (angle < 112.5f)
                (dx, dy) = (0, 1);
            else
                (dx, dy) = (-1, 1);

            if (m < magnitude[y + dy, x + dx] ||
                m < magnitude[y - dy, x - dx])
                continue;

            weak[y, x] = true;
            if (m >= highThreshold)
                edge[y, x] = true;
        }

        // 4. Hysteresis: retain weak edges connected to strong edges.
        var queue = new Queue<(int x, int y)>();

        for (int y = 1; y < h - 1; y++)
        for (int x = 1; x < w - 1; x++)
        {
            if (edge[y, x])
                queue.Enqueue((x, y));
        }

        while (queue.Count > 0)
        {
            var (x, y) = queue.Dequeue();

            for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
            {
                if (dx == 0 && dy == 0)
                    continue;

                int nx = x + dx;
                int ny = y + dy;

                if (weak[ny, nx] && !edge[ny, nx])
                {
                    edge[ny, nx] = true;
                    queue.Enqueue((nx, ny));
                }
            }
        }

        var points = new List<(int x, int y, float strength)>();

        for (int y = 1; y < h - 1; y++)
        for (int x = 1; x < w - 1; x++)
        {
            if (edge[y, x])
                points.Add((x, y, magnitude[y, x]));
        }

        if (points.Count == 0)
        {
            var empty = new List<Esp32ImageSender.LineSegment>();
            return new VectorFrame
            {
                Lines = empty,
                Preview = Render(empty, w, h)
            };
        }

        // 5. Hough transform: vote for straight lines.
        int maxRho = (int)Math.Ceiling(
            Math.Sqrt(w * w + h * h));

        int rhoCount = maxRho * 2 + 1;
        int[,] accumulator = new int[AngleCount, rhoCount];

        float[] cosTable = new float[AngleCount];
        float[] sinTable = new float[AngleCount];

        for (int a = 0; a < AngleCount; a++)
        {
            float rad = a * MathF.PI / 180f;
            cosTable[a] = MathF.Cos(rad);
            sinTable[a] = MathF.Sin(rad);
        }

        foreach (var p in points)
        {
            for (int a = 0; a < AngleCount; a++)
            {
                int rho = (int)MathF.Round(
                    p.x * cosTable[a] + p.y * sinTable[a]);

                accumulator[a, rho + maxRho]++;
            }
        }

        // 6. Collect local accumulator peaks.
        var candidates = new List<Candidate>();

        for (int a = 0; a < AngleCount; a++)
        for (int r = 1; r < rhoCount - 1; r++)
        {
            int votes = accumulator[a, r];
            if (votes < MinLineVotes)
                continue;

            bool peak = true;

            for (int da = -1; da <= 1 && peak; da++)
            for (int dr = -1; dr <= 1; dr++)
            {
                if (da == 0 && dr == 0)
                    continue;

                int na = (a + da + AngleCount) % AngleCount;
                int nr = r + dr;

                if (nr >= 0 && nr < rhoCount &&
                    accumulator[na, nr] > votes)
                {
                    peak = false;
                    break;
                }
            }

            if (peak)
            {
                candidates.Add(new Candidate
                {
                    Angle = a,
                    Rho = r - maxRho,
                    Votes = votes
                });
            }
        }

        candidates = candidates
            .OrderByDescending(c => c.Votes)
            .ToList();

        // 7. Convert each peak into a finite segment.
        // Suppress edge points already explained by accepted lines.
        bool[] consumed = new bool[points.Count];
        var lines = new List<Esp32ImageSender.LineSegment>();

        float lineTolerance = Math.Max(1f, tolerance);

        foreach (var candidate in candidates)
        {
            if (lines.Count >= maxSegments)
                break;

            float cos = cosTable[candidate.Angle];
            float sin = sinTable[candidate.Angle];

            // Line normal is (cos, sin); tangent is (-sin, cos).
            float tx = -sin;
            float ty = cos;

            var projected = new List<(float t, int index)>();

            for (int i = 0; i < points.Count; i++)
            {
                if (consumed[i])
                    continue;

                var p = points[i];

                float distance = MathF.Abs(
                    p.x * cos + p.y * sin - candidate.Rho);

                if (distance <= lineTolerance)
                {
                    float t = p.x * tx + p.y * ty;
                    projected.Add((t, i));
                }
            }

            if (projected.Count < MinLineVotes)
                continue;

            projected.Sort((a, b) => a.t.CompareTo(b.t));

            // Find the longest contiguous run along the line.
            // Small gaps are tolerated; large gaps split the stroke.
            int bestStart = 0, bestEnd = 0;
            int runStart = 0;

            for (int i = 1; i <= projected.Count; i++)
            {
                bool gap = i == projected.Count ||
                           projected[i].t - projected[i - 1].t > 3.0f;

                if (!gap)
                    continue;

                if (projected[runStart].t >= projected[i - 1].t)
                {
                    runStart = i;
                    continue;
                }

                if (i - runStart > bestEnd - bestStart)
                {
                    bestStart = runStart;
                    bestEnd = i;
                }

                runStart = i;
            }

            if (bestEnd <= bestStart)
                continue;

            float t0 = projected[bestStart].t;
            float t1 = projected[bestEnd - 1].t;

            if (t1 - t0 < MinLineLength)
                continue;

            // Reconstruct endpoints on the detected line.
            float x0 = candidate.Rho * cos + t0 * tx;
            float y0 = candidate.Rho * sin + t0 * ty;
            float x1 = candidate.Rho * cos + t1 * tx;
            float y1 = candidate.Rho * sin + t1 * ty;

            int ix0 = Math.Clamp((int)MathF.Round(x0), 0, w - 1);
            int iy0 = Math.Clamp((int)MathF.Round(y0), 0, h - 1);
            int ix1 = Math.Clamp((int)MathF.Round(x1), 0, w - 1);
            int iy1 = Math.Clamp((int)MathF.Round(y1), 0, h - 1);

            if (ix0 == ix1 && iy0 == iy1)
                continue;

            lines.Add(new(
                (byte)ix0, (byte)iy0,
                (byte)ix1, (byte)iy1));

            // Consume nearby edge pixels along the accepted segment.
            for (int i = 0; i < points.Count; i++)
            {
                var p = points[i];

                float along = (p.x - x0) * tx + (p.y - y0) * ty;
                float across = MathF.Abs(
                    (p.x - x0) * cos + (p.y - y0) * sin);

                if (along >= -1 && along <= t1 - t0 + 1 &&
                    across <= lineTolerance + 0.5f)
                {
                    consumed[i] = true;
                }
            }
        }

        return new VectorFrame
        {
            Lines = lines,
            Preview = Render(lines, w, h)
        };
    }

    private static byte[,] Blur(byte[,] src)
    {
        int h = src.GetLength(0);
        int w = src.GetLength(1);
        var dst = new byte[h, w];

        int[,] kernel =
        {
            { 1, 2, 1 },
            { 2, 4, 2 },
            { 1, 2, 1 }
        };

        for (int y = 1; y < h - 1; y++)
        for (int x = 1; x < w - 1; x++)
        {
            int sum = 0;

            for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
                sum += src[y + dy, x + dx] * kernel[dy + 1, dx + 1];

            dst[y, x] = (byte)(sum / 16);
        }

        return dst;
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
                DrawLine(ptr, fb.RowBytes,
                    line.X1, line.Y1, line.X2, line.Y2);
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