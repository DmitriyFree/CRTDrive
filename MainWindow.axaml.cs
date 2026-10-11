
using Avalonia.Controls;
using Avalonia.Media.Imaging;
using Avalonia.Threading;
using CrtDrive.Services;
using System;
using System.Threading;
using System.Threading.Tasks;

namespace CrtDrive;

public partial class MainWindow : Window
{
    private readonly ScreenCaptureService _capture = new();
    private readonly Esp32ImageSender _sender = new();

    private readonly object _frameLock = new();
    private byte[]? _latestRawFrame;
    private bool _framePending;
    private int _isProcessing;

    private bool _closed;

    private sealed record FrameSettings(
        float Brightness,
        float Contrast,
        byte Threshold,
        int Erode,
        int Dilate,
        float EdgeStrength,
        float Gamma,
        float AdaptiveExposure,
        bool VectorMode,
        int MaxSegments,
        byte VectorThreshold,
        float VectorTolerance);

    public MainWindow()
    {
        InitializeComponent();

        Closed += (_, _) =>
        {
            _closed = true;
            _capture.FrameReceived -= OnFrame;
            _capture.Stop();
            _sender.Dispose();
        };

        _capture.FrameReceived += OnFrame;
        _capture.Start();
    }

    private void OnFrame(byte[] frame)
    {
        if (_closed)
            return;

        lock (_frameLock)
        {
            // Keep only the newest frame waiting to be processed.
            _latestRawFrame = (byte[])frame.Clone();
            _framePending = true;
        }

        StartWorkerIfNeeded();
    }

    private void StartWorkerIfNeeded()
    {
        if (Interlocked.CompareExchange(ref _isProcessing, 1, 0) == 0)
            _ = Task.Run(ProcessLoop);
    }

    private async Task ProcessLoop()
    {
        try
        {
            while (!_closed)
            {
                byte[]? frame;

                lock (_frameLock)
                {
                    if (!_framePending)
                        break;

                    frame = _latestRawFrame;
                    _framePending = false;
                }

                if (frame == null)
                    continue;

                // Read all Avalonia controls on the UI thread.
                FrameSettings settings =
                    await Dispatcher.UIThread.InvokeAsync(ReadSettings);

                if (_closed)
                    break;

                int width = _capture.Width;
                int height = _capture.Height;

                // This bitmap belongs to this frame; it is not being
                // modified while the UI displays it.
                WriteableBitmap sourceBitmap =
                    RawFrameConverter.Convert(frame, width, height);

                var imageSettings = new ImageSettings
                {
                    Brightness = settings.Brightness,
                    Contrast = settings.Contrast,
                    Threshold = settings.Threshold,
                    Erode = settings.Erode,
                    Dilate = settings.Dilate,
                    EdgeStrength = settings.EdgeStrength,
                    Gamma = settings.Gamma,
                    AdaptiveExposure = settings.AdaptiveExposure
                };

                byte[,] processed = ImageProcessor.To8Bit(
                    sourceBitmap, 100, 100, imageSettings);

                WriteableBitmap processedBitmap;

                if (settings.VectorMode)
                {
                    var vector = VectorImageProcessor.Process(
                        processed,
                        maxSegments: settings.MaxSegments,
                        threshold: settings.VectorThreshold,
                        tolerance: settings.VectorTolerance);

                    if (_sender._port?.IsOpen == true)
                        _sender.SendVectorLines(vector.Lines);

                    processedBitmap = vector.Preview;
                }
                else
                {
                    if (_sender._port?.IsOpen == true)
                        _sender.SendImage2Bit(processed);

                    processedBitmap =
                        ImageProcessor.RenderDisplay8Bit(processed);
                }

                // Only update Avalonia controls on the UI thread.
                await Dispatcher.UIThread.InvokeAsync(() =>
                {
                    if (_closed)
                        return;

                    OriginalImage.Source = sourceBitmap;
                    OriginalImage.InvalidateVisual();

                    ProcessedImage.Source = processedBitmap;
                    ProcessedImage.InvalidateVisual();
                }, DispatcherPriority.Background);
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Frame processing error: {ex}");
        }
        finally
        {
            Interlocked.Exchange(ref _isProcessing, 0);

            // Catch a frame that arrived as the worker was stopping.
            bool pending;

            lock (_frameLock)
                pending = _framePending;

            if (pending && !_closed)
                StartWorkerIfNeeded();
        }
    }

    private FrameSettings ReadSettings()
    {
        return new FrameSettings(
            Brightness: (float)BrightnessSlider.Value,
            Contrast: (float)ContrastSlider.Value,
            Threshold: (byte)Math.Clamp(
                (int)Math.Round(ThresholdSlider.Value), 0, 255),
            Erode: (int)ErodeSlider.Value,
            Dilate: (int)DilateSlider.Value,
            EdgeStrength: (float)EdgeStrengthSlider.Value,
            Gamma: (float)GammaSlider.Value,
            AdaptiveExposure: (float)AdaptiveExposureSlider.Value,
            VectorMode: VectorModeCheckBox.IsChecked == true,
            MaxSegments: (int)MaxSegmentsSlider.Value,
            VectorThreshold: (byte)Math.Clamp(
                (int)Math.Round(VectorThresholdSlider.Value), 0, 255),
            VectorTolerance: (float)VectorToleranceSlider.Value
        );
    }
}