using Avalonia.Controls;
using Avalonia.Threading;
using CrtDrive.Services;
using Avalonia.Media.Imaging;
using System;
using System.Threading;
using Avalonia;
using System.Threading.Tasks;

namespace CrtDrive;

public partial class MainWindow : Window
{
    private readonly ScreenCaptureService _capture = new();
    private readonly Esp32ImageSender _sender = new();
    private WriteableBitmap? _bitmap;
    private volatile byte[]? _latestRawFrame;
    private volatile bool _framePending = false;
    private int _isProcessing = 0; // 0 = free, 1 = busy
    private readonly ImageSettings _settings = new();
    

    private float _brightness;
    private float _contrast;
    private byte _threshold;
    private int _erode;
    private int _dilate;
    private float _edgeStrength = 2;
    private float _gamma = 1f;
    private float _adaptiveExposure = 0.5f;

    public MainWindow()
    {
        InitializeComponent();

        _capture.FrameReceived += OnFrame;
        _capture.Start();
    }

    private void OnFrame(byte[] frame)
    {
        _latestRawFrame = frame;
        _framePending = true;

        // start worker if not running
        if (Interlocked.CompareExchange(ref _isProcessing, 1, 0) == 0)
        {
            Task.Run(ProcessLoop);
        }
    }
    private void ProcessLoop()
    {
        while (true)
        {
            if (!_framePending)
                break;

            _framePending = false;

            var frame = _latestRawFrame;
            if (frame == null)
                break;

            // 🔥 SKIP if newer frame already arrived
            if (_framePending)
                continue;

            _bitmap ??= new WriteableBitmap(
                new PixelSize(_capture.Width, _capture.Height),
                new Vector(96, 96),
                Avalonia.Platform.PixelFormat.Bgra8888,
                Avalonia.Platform.AlphaFormat.Opaque
            );

            RawFrameConverter.CopyInto(_bitmap, frame);

            var settings = new ImageSettings
            {
                Brightness = _brightness,
                Contrast = _contrast,
                Threshold = _threshold,
                Erode = _erode,
                Dilate = _dilate,
                EdgeStrength = _edgeStrength,
                Gamma = _gamma,
                AdaptiveExposure = _adaptiveExposure
            };

            var processed = ImageProcessor.To8Bit(
                _bitmap, 100, 100, settings);
           
            bool vectorMode = true;//VectorModeCheckBox.IsChecked == true; //gets stuck somehow
            
            if (vectorMode)
            {
                var vector = VectorImageProcessor.Process(
                    processed,
                    maxSegments: 200,//(int)MaxSegmentsSlider.Value,//also getting stuck
                    threshold: 200,//(byte)VectorThresholdSlider.Value,
                    tolerance: 1);//(float)VectorToleranceSlider.Value); 

                if (_sender._port?.IsOpen == true)
                    _sender.SendVectorLines(vector.Lines);
                Console.WriteLine(vector.Lines.Count);//not getting here
                Dispatcher.UIThread.Post(() =>
                {
                    OriginalImage.Source = _bitmap;
                    ProcessedImage.Source = vector.Preview;
                });
            }
            else
            {
                if (_sender._port?.IsOpen == true)
                    _sender.SendImage2Bit(processed);
                
                Dispatcher.UIThread.Post(() =>
                {
                    OriginalImage.Source = _bitmap;
                    ProcessedImage.Source =
                        ImageProcessor.RenderDisplay8Bit(processed);
                });
            }

            Dispatcher.UIThread.Post(() =>
            {
                OriginalImage.Source = _bitmap;
                OriginalImage.InvalidateVisual();
                //if (processed != null)
                //    ProcessedImage.Source = ImageProcessor.RenderDisplay8Bit(processed);

                _brightness = (float)BrightnessSlider.Value;
                _contrast = (float)ContrastSlider.Value;
                _threshold = (byte)ThresholdSlider.Value;
                _erode = (int)ErodeSlider.Value;
                _dilate = (int)DilateSlider.Value;
                _edgeStrength = (float)EdgeStrengthSlider.Value;
                _adaptiveExposure = (float)AdaptiveExposureSlider.Value;
                _gamma = (float)GammaSlider.Value;

            }, DispatcherPriority.Background);
        }

        Interlocked.Exchange(ref _isProcessing, 0);

        // 🔥 CRITICAL: restart if a frame arrived while we were stopping
        if (_framePending &&
            Interlocked.CompareExchange(ref _isProcessing, 1, 0) == 0)
        {
            Task.Run(ProcessLoop);
        }
    }
}