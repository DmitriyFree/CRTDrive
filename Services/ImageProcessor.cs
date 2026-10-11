using Avalonia.Media.Imaging;
using Avalonia;
using System;

namespace CrtDrive.Services;

public static class ImageProcessor
{
    const int midW = 300;
    const int midH = 300;

    // ============================================================
    // PUBLIC API
    // ============================================================

    public static byte[,] To8Bit(
        WriteableBitmap bitmap,
        int outW,
        int outH,
        ImageSettings s)
    {
        var gray = ProcessGray(bitmap, s);

        return Downscale(gray, outW, outH, g => g);
    }
    public static byte[,] ToGray300(
        WriteableBitmap bitmap,
        ImageSettings settings)
    {
        return ProcessGray(bitmap, settings);
    }

    // ============================================================
    // RENDER
    // ============================================================

    public static WriteableBitmap RenderDisplay(byte[,] px)
    {
        return Render(px, v =>
        {
            if (v == 0) return (0, 0, 0);
            return (255, 120, 0);
        });
    }

    public static WriteableBitmap RenderDisplay8Bit(byte[,] px)
    {
        return Render(px, v =>
        {
            // normalize
            float t = v / 255f;

            // plasma-style curve (nonlinear glow)
            // tweakable!
            float glow = t * t; // gamma ~2

            byte r = (byte)(glow * 255);
            byte g = (byte)(glow * 120);
            byte b = 0;

            return (r, g, b);
        });
    }

    // ============================================================
    // CORE PIPELINE
    // ============================================================

    private static byte[,] ProcessGray(WriteableBitmap bitmap, ImageSettings s)
    {
        var gray = BuildGray(bitmap, midW, midH, s);
        int h = gray.GetLength(0);
        int w = gray.GetLength(1);
        if (s.Erode > 0)
            for (int i = 0; i < s.Erode; i++)
                gray = ErodeGray(gray);

        if (s.Dilate > 0)
            for (int i = 0; i < s.Dilate; i++)
                gray = DilateGray(gray);

        if (s.EdgeStrength > 0)
            gray = Sobel(gray, s.EdgeStrength);

        if (s.Gamma != 1.0f)
        {
            float inv = 1f / s.Gamma;

            for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                float t = gray[y, x] / 255f;
                t = MathF.Pow(t, inv);
                gray[y, x] = (byte)(t * 255f);
            }
        }

        if (s.AdaptiveExposure > 0)
            gray = AdaptiveExposure(gray, s.AdaptiveExposure);

        if(s.Threshold > 0)
        {
            for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                if(gray[y, x] < s.Threshold)
                    gray[y, x] = 0;
            }
        }

        return gray;
    }

    private static float ComputeAdaptiveThreshold(byte[,] gray, ImageSettings s)
    {
        int sum = 0;

        int h = gray.GetLength(0);
        int w = gray.GetLength(1);

        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                sum += gray[y, x];

        float adaptive = sum / (float)(w * h);
        return (adaptive * 0.7f) + (s.Threshold * 0.3f);
    }

    private static byte[,] Downscale(
        byte[,] gray,
        int outW,
        int outH,
        Func<byte, byte> quantize)
    {
        var result = new byte[outH, outW];

        for (int y = 0; y < outH; y++)
        {
            int srcY = y * midH / outH;

            for (int x = 0; x < outW; x++)
            {
                int srcX = x * midW / outW;
                byte val = quantize(gray[srcY, srcX]);
                result[y, x] = val;
            }
        }

        return result;
    }

    private static WriteableBitmap Render(byte[,] px, Func<byte, (byte r, byte g, byte b)> map)
    {
        int h = px.GetLength(0);
        int w = px.GetLength(1);

        var bmp = new WriteableBitmap(
            new PixelSize(w, h),
            new Vector(96, 96),
            Avalonia.Platform.PixelFormat.Bgra8888,
            Avalonia.Platform.AlphaFormat.Opaque
        );

        using var fb = bmp.Lock();

        unsafe
        {
            byte* ptr = (byte*)fb.Address;

            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    int idx = (y * w + x) * 4;

                    var (r, g, b) = map(px[y, x]);

                    ptr[idx + 0] = b;
                    ptr[idx + 1] = g;
                    ptr[idx + 2] = r;
                    ptr[idx + 3] = 255;
                }
            }
        }

        return bmp;
    }

    // ============================================================
    // LOW LEVEL
    // ============================================================

    private static byte[,] BuildGray(
        WriteableBitmap bitmap,
        int midW,
        int midH,
        ImageSettings s)
    {
        var gray = new byte[midH, midW];

        using var fb = bitmap.Lock();

        unsafe
        {
            byte* ptr = (byte*)fb.Address;
            int stride = fb.RowBytes;

            int srcW = bitmap.PixelSize.Width;
            int srcH = bitmap.PixelSize.Height;

            int cropSize = (int)(srcW * 0.35);
            if (cropSize > srcH) cropSize = srcH;

            int startX = (srcW - cropSize) / 2;
            int startY = (srcH - cropSize) / 2;

            for (int y = 0; y < midH; y++)
            {
                int srcY = startY + (cropSize - 1 - (y * cropSize / midH));

                for (int x = 0; x < midW; x++)
                {
                    int srcX = startX + (x * cropSize / midW);
                    byte* pixel = ptr + srcY * stride + srcX * 4;

                    float g = (pixel[0] + pixel[1] + pixel[2]) / 3f;

                    g += s.Brightness * 255f;
                    g = (g - 128f) * s.Contrast + 128f;

                    if (g < 0) g = 0;
                    if (g > 255) g = 255;

                    gray[y, x] = (byte)g;
                }
            }
        }

        return gray;
    }

    private static byte[,] ErodeGray(byte[,] src)
    {
        int h = src.GetLength(0);
        int w = src.GetLength(1);
        var dst = new byte[h, w];

        for (int y = 1; y < h - 1; y++)
        for (int x = 1; x < w - 1; x++)
        {
            byte min = 255;

            for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
            {
                byte v = src[y + dy, x + dx];
                if (v < min) min = v;
            }

            dst[y, x] = min;
        }

        return dst;
    }

    private static byte[,] DilateGray(byte[,] src)
    {
        int h = src.GetLength(0);
        int w = src.GetLength(1);
        var dst = new byte[h, w];

        for (int y = 1; y < h - 1; y++)
        for (int x = 1; x < w - 1; x++)
        {
            byte max = 0;

            for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
            {
                byte v = src[y + dy, x + dx];
                if (v > max) max = v;
            }

            dst[y, x] = max;
        }

        return dst;
    }

    private static byte[,] Sobel(byte[,] src, float strength)
    {
        int h = src.GetLength(0);
        int w = src.GetLength(1);
        var dst = new byte[h, w];

        for (int y = 1; y < h - 1; y++)
        for (int x = 1; x < w - 1; x++)
        {
            int gx =
                -src[y - 1, x - 1] - 2 * src[y, x - 1] - src[y + 1, x - 1] +
                src[y - 1, x + 1] + 2 * src[y, x + 1] + src[y + 1, x + 1];

            int gy =
                -src[y - 1, x - 1] - 2 * src[y - 1, x] - src[y - 1, x + 1] +
                src[y + 1, x - 1] + 2 * src[y + 1, x] + src[y + 1, x + 1];

            float mag = MathF.Sqrt(gx * gx + gy * gy);

            float baseVal = src[y, x];

            // Smooth polarity factor:
            // dark pixels -> +1
            // bright pixels -> -1
            // mid tones -> near 0
            float polarity = (128f - baseVal) / 128f;

            float v = baseVal + mag * strength * polarity;

            if (v < 0) v = 0;
            if (v > 255) v = 255;

            dst[y, x] = (byte)v;
        }

        return dst;
    }
    private static byte[,] AdaptiveExposure(byte[,] src, float strength)
    {
        int h = src.GetLength(0);
        int w = src.GetLength(1);
        var dst = new byte[h, w];

        int radius = 12; // size of local area (tweakable)
        int area = (radius * 2 + 1) * (radius * 2 + 1);

        // build integral image for fast local averages
        int[,] integral = new int[h + 1, w + 1];

        for (int y = 1; y <= h; y++)
        for (int x = 1; x <= w; x++)
            integral[y, x] =
                src[y - 1, x - 1]
            + integral[y - 1, x]
            + integral[y, x - 1]
            - integral[y - 1, x - 1];

        for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            int x0 = Math.Max(0, x - radius);
            int y0 = Math.Max(0, y - radius);
            int x1 = Math.Min(w - 1, x + radius);
            int y1 = Math.Min(h - 1, y + radius);

            int count = (x1 - x0 + 1) * (y1 - y0 + 1);

            int sum =
                integral[y1 + 1, x1 + 1]
            - integral[y0, x1 + 1]
            - integral[y1 + 1, x0]
            + integral[y0, x0];

            float localMean = sum / (float)count;

            float pixel = src[y, x];

            // compute local contrast gain
            float gain = 1f + strength * 2f;

            // stretch around local mean
            float v = (pixel - localMean) * gain + localMean;

            // IMPORTANT: preserve true black & white anchors
            if (pixel <= 2) v = 0;
            else if (pixel >= 253) v = 255;

            if (v < 0) v = 0;
            if (v > 255) v = 255;

            dst[y, x] = (byte)v;
        }

        return dst;
    }
}