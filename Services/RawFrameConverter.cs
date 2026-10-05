using Avalonia;
using Avalonia.Media.Imaging;
using System;

namespace CrtDrive.Services;

public static class RawFrameConverter
{
    public static WriteableBitmap Convert(byte[] raw, int width, int height)
    {
        var bitmap = new WriteableBitmap(
            new PixelSize(width, height),
            new Vector(96, 96),
            Avalonia.Platform.PixelFormat.Bgra8888, // matches 0rgb/bgr0 converted
            Avalonia.Platform.AlphaFormat.Opaque
        );

        using var fb = bitmap.Lock();

        unsafe
        {
            fixed (byte* srcPtr = raw)
            {
                Buffer.MemoryCopy(
                    srcPtr,
                    (void*)fb.Address,
                    width * height * 4,
                    width * height * 4
                );
            }
        }

        return bitmap;
    }
    public static byte[] RawBytesFromBitmap(WriteableBitmap bitmap)
    {
        int width = bitmap.PixelSize.Width;
        int height = bitmap.PixelSize.Height;
        int bytesPerPixel = 4;
        int frameSize = width * height * bytesPerPixel;

        var data = new byte[frameSize];

        using var fb = bitmap.Lock();
        unsafe
        {
            fixed (byte* dstPtr = data)
            {
                Buffer.MemoryCopy((void*)fb.Address, dstPtr, frameSize, frameSize);
            }
        }

        return data;
    }
    public static void CopyInto(WriteableBitmap bitmap, byte[] raw)
    {
        using var fb = bitmap.Lock();

        int width = bitmap.PixelSize.Width;
        int height = bitmap.PixelSize.Height;

        unsafe
        {
            fixed (byte* srcPtr = raw)
            {
                byte* src = srcPtr;
                byte* dst = (byte*)fb.Address;

                int srcStride = width * 4;
                int dstStride = fb.RowBytes;

                // 🔥 handle stride properly (important!)
                for (int y = 0; y < height; y++)
                {
                    Buffer.MemoryCopy(
                        src + y * srcStride,
                        dst + y * dstStride,
                        dstStride,
                        srcStride
                    );
                }
            }
        }
    }
}