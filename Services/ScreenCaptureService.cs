using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;

namespace CrtDrive.Services;

public sealed class ScreenCaptureService : IDisposable
{
    public int Width { get; }
    public int Height { get; }

    public event Action<byte[]>? FrameReceived;

    private Process? _ffmpeg;
    private CancellationTokenSource? _cts;
    private Task? _readTask;
    private Task? _logTask;

    public ScreenCaptureService(int width = 480, int height = 300/*int width = 960, int height = 600*/)
    {
        Width = width;
        Height = height;
    }

    // =========================
    // Public API
    // =========================

    public void Start()
    {
        Stop();

        int screenIndex = GetScreenIndex();
        var (screenWidth, screenHeight) = GetMainDisplayResolution();

        string args = BuildFfmpegArgs(screenIndex, screenWidth, screenHeight);

        _cts = new CancellationTokenSource();

        _ffmpeg = new Process
        {
            StartInfo = new ProcessStartInfo
            {
                FileName = "ffmpeg",
                Arguments = args,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            }
        };

        _ffmpeg.Start();

        _readTask = Task.Run(() => ReadFrames(_cts.Token));
        _logTask = Task.Run(() => DrainErrors(_cts.Token));
    }

    public void Stop()
    {
        if (_cts == null)
            return;

        try
        {
            _cts.Cancel();

            if (_ffmpeg != null && !_ffmpeg.HasExited)
                _ffmpeg.Kill(true);
        }
        catch { }

        try { _readTask?.Wait(200); } catch { }
        try { _logTask?.Wait(200); } catch { }

        _ffmpeg?.Dispose();
        _cts.Dispose();

        _ffmpeg = null;
        _cts = null;
        _readTask = null;
        _logTask = null;
    }

    public void Dispose() => Stop();

    // =========================
    // FFmpeg
    // =========================

    private string BuildFfmpegArgs(int screenIndex, int screenWidth, int screenHeight)
    {
        return string.Join(" ", new[]
        {
            "-f avfoundation",
            "-re",
            "-r 100",

            "-capture_raw_data true",

            "-capture_cursor 0",
            "-capture_mouse_clicks 0",

            // 🔥 LOWER FPS = less internal buffering
            "-framerate 30",

            $"-video_size {screenWidth}x{screenHeight}",
            $"-i \"{screenIndex}:none\"",

            // 🔥 SCALE EARLY (important)
            $"-vf scale={Width}:{Height}",

            "-pix_fmt bgr0",
            "-vcodec rawvideo",

            // 🔥 CRITICAL: kill buffering harder
            "-fflags nobuffer+discardcorrupt",
            "-flags low_delay",
            "-strict experimental",

            // 🔥 THIS is missing and VERY important
            "-flush_packets 1",

            // 🔥 Force frame dropping inside FFmpeg
            "-vsync drop",
            "-fps_mode drop",

            // 🔥 Prevent internal queue buildup
            "-thread_queue_size 1",

            "-f rawvideo -"
        });
    }

    private async Task ReadFrames(CancellationToken token)
    {
        if (_ffmpeg == null)
            return;

        int frameSize = Width * Height * 4;

        byte[] bufferA = new byte[frameSize];
        byte[] bufferB = new byte[frameSize];
        byte[] current = bufferA;

        var stream = _ffmpeg.StandardOutput.BaseStream;

        // 🔥 KEY FIX: single shared latest frame
        byte[] latestFrame = bufferA;
        object gate = new object();

        // consumer copy (optional)
        void Publish(byte[] frame)
        {
            FrameReceived?.Invoke(frame);
        }

        try
        {
            while (!token.IsCancellationRequested)
            {
                int read = 0;

                // ALWAYS read full frame
                while (read < frameSize)
                {
                    int n = await stream.ReadAsync(current, read, frameSize - read, token);
                    if (n == 0)
                        return;

                    read += n;
                }

                // 🔥 CRITICAL: overwrite latest frame atomically
                byte[] toSend;

                lock (gate)
                {
                    toSend = current;
                    latestFrame = current;
                }

                Publish(toSend);

                // swap buffers
                current = ReferenceEquals(current, bufferA) ? bufferB : bufferA;
            }
        }
        catch (OperationCanceledException) { }
    }

    private async Task DrainErrors(CancellationToken token)
    {
        if (_ffmpeg == null)
            return;

        try
        {
            while (!token.IsCancellationRequested)
            {
                string? line = await _ffmpeg.StandardError.ReadLineAsync();
                if (line == null)
                    break;

                // Optional:
                // Console.WriteLine("[FFmpeg] " + line);
            }
        }
        catch { }
    }

    // =========================
    // Screen Detection
    // =========================

    private static int GetScreenIndex()
    {
        using var p = new Process
        {
            StartInfo = new ProcessStartInfo
            {
                FileName = "ffmpeg",
                Arguments = "-f avfoundation -list_devices true -i \"\"",
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            }
        };

        p.Start();
        string output = p.StandardError.ReadToEnd();
        p.WaitForExit();

        var matches = Regex.Matches(output, @"\[(\d+)\]\s+Capture screen");

        if (matches.Count == 0)
            throw new InvalidOperationException("No screen capture device found.");

        return int.Parse(matches[0].Groups[1].Value);
    }

    // =========================
    // macOS Display Resolution
    // =========================

    [DllImport("/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics")]
    private static extern IntPtr CGMainDisplayID();

    [DllImport("/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics")]
    private static extern CGRect CGDisplayBounds(IntPtr display);

    [StructLayout(LayoutKind.Sequential)]
    private struct CGRect
    {
        public CGPoint origin;
        public CGSize size;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct CGPoint
    {
        public double x;
        public double y;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct CGSize
    {
        public double width;
        public double height;
    }

    private static (int width, int height) GetMainDisplayResolution()
    {
        var display = CGMainDisplayID();
        var bounds = CGDisplayBounds(display);

        int w = (int)bounds.size.width;
        int h = (int)bounds.size.height;

        double scale = 2.0; // Retina default
        return ((int)(w * scale), (int)(h * scale));
    }
}