using System;
using System.IO.Ports;
using System.Threading;
using System.Collections.Generic;

namespace CrtDrive.Services;

public class Esp32ImageSender : IDisposable
{
    public readonly SerialPort? _port;
    private readonly object _lock = new();

    private byte[]? _latestFrame;
    private byte[]? _lastSentFrame;

    private bool _sending;
    private volatile bool _ready = true;

    private FrameMode _currentMode;

    private readonly Timer _ackWatchdog;
    private const int ACK_TIMEOUT_MS = 1000;//67; // ~1/15 sec

    private FrameMode _lastSentMode;
    private bool _hasLastSentFrame;

    private enum FrameMode : byte
    {
        Bit1Packed = 0,
        Bit8Raw = 1,
        Bit2Packed = 2,
        VectorLines = 3
    }

    public Esp32ImageSender(int baud = 3000000)
    {
        _ackWatchdog = new Timer(OnAckTimeout);

        var ports = SerialPort.GetPortNames();

        if (ports.Length == 0)
        {
            Console.WriteLine("❌ No serial ports found");
            return;
        }

        foreach (var p in ports)
        {
            try
            {
                if (!(p.Contains("usbserial") || p.Contains("usbmodem") || p.Contains("ttyUSB") || p.Contains("COM")))
                    continue;

                Console.WriteLine($"Trying port: {p}");

                var testPort = new SerialPort(p, baud)
                {
                    Parity = Parity.None,
                    DataBits = 8,
                    StopBits = StopBits.One,
                    Handshake = Handshake.None,
                    ReadTimeout = 200,
                    WriteTimeout = 200
                };

                testPort.Open();
                

                if (!testPort.IsOpen) continue;

                Console.WriteLine($"✅ Connected to {p}");

                _port = testPort;
                _port.DataReceived += OnDataReceived;
                return;
            }
            catch
            {
                // ignore
            }
        }

        Console.WriteLine("❌ Could not open any serial port");
    }

    // =========================
    // PUBLIC API
    // =========================

    public void SendImage8Bit(byte[,] pixels)
    {
        if (_port == null || !_port.IsOpen) return;

        var raw = Flatten(pixels);

        lock (_lock)
        {
            if (IsDuplicate(raw, FrameMode.Bit8Raw)) return;

            _currentMode = FrameMode.Bit8Raw;
            _latestFrame = raw;
            _lastSentFrame = (byte[])_latestFrame.Clone();
            _lastSentMode = _currentMode;
            _hasLastSentFrame = true;

            TrySend();
        }
    }
    public void SendImage2Bit(byte[,] pixels)
    {
        if (_port == null || !_port.IsOpen) return;

        var packed = Pack2Bit(pixels);

        lock (_lock)
        {
            if (IsDuplicate(packed, FrameMode.Bit2Packed)) return;

            _currentMode = FrameMode.Bit2Packed;
            _latestFrame = packed;
            _lastSentFrame = (byte[])_latestFrame.Clone();
            _lastSentMode = _currentMode;
            _hasLastSentFrame = true;

            TrySend();
        }
    }
    private static byte[] Pack2Bit(byte[,] px)
    {
        int h = px.GetLength(0);
        int w = px.GetLength(1);

        if (w != 100 || h != 100)
            throw new ArgumentException(
                $"Expected 100x100 image, got {w}x{h}");

        const int pixelCount = 100 * 100;
        const int byteCount = pixelCount / 4;

        var result = new byte[byteCount];

        int pixel = 0;

        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                byte v = px[y, x];
                int q = v >> 6;

                result[pixel >> 2] |=
                    (byte)(q << ((pixel & 3) * 2));

                pixel++;
            }
        }

        return result;
    }

    // =========================
    // SEND PIPELINE
    // =========================

    private void TrySend()
    {
        if (!_ready || _sending) return;
        SendLatest();
    }

    private void SendLatest()
    {
        if (_port == null || !_port.IsOpen) return;
        if (_latestFrame == null || !_ready) return;

        var frame = BuildFrame(_latestFrame, _currentMode);

        _sending = true;
        _ready = false;

        try
        {
            _port.Write(frame, 0, frame.Length);
            _ackWatchdog.Change(ACK_TIMEOUT_MS, Timeout.Infinite);
        }
        catch
        {
            _ready = true;
        }

        _lastSentFrame = (byte[])_latestFrame.Clone();
        _sending = false;
    }

    // =========================
    // ACK HANDLING
    // =========================

   private void OnDataReceived(object sender, SerialDataReceivedEventArgs e)
    {
        if (_port == null || !_port.IsOpen)
            return;

        try
        {
            while (_port.BytesToRead > 0)
            {
                int b = _port.ReadByte();

                if (b != 'k')
                    continue;

                lock (_lock)
                {
                    _ackWatchdog.Change(
                        Timeout.Infinite,
                        Timeout.Infinite
                    );

                    _ready = true;
                    TrySend();
                }
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Serial RX error: {ex.Message}");
        }
    }

    private void OnAckTimeout(object? state)
    {
        lock (_lock)
        {
            if (_ready)
                return;

            Console.WriteLine("ACK TIMEOUT");

            // Don't resend automatically.
            _ready = true;
        }
    }

    // =========================
    // HELPERS
    // =========================

    private bool IsDuplicate(byte[] frame, FrameMode mode)
    {
        return _hasLastSentFrame &&
            _lastSentMode == mode &&
            _lastSentFrame != null &&
            AreEqual(_lastSentFrame, frame);
    }

    private static bool AreEqual(byte[] a, byte[] b)
    {
        if (ReferenceEquals(a, b)) return true;
        if (a.Length != b.Length) return false;

        for (int i = 0; i < a.Length; i++)
            if (a[i] != b[i])
                return false;

        return true;
    }

    private static byte[] Flatten(byte[,] px)
    {
        int h = px.GetLength(0);
        int w = px.GetLength(1);

        var result = new byte[w * h];
        int idx = 0;

        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                result[idx++] = px[y, x];

        return result;
    }

    // =========================
    // FRAME BUILD
    // =========================

    private static byte[] BuildFrame(byte[] data, FrameMode mode)
    {
        byte[] frame = new byte[data.Length + 6];

        frame[0] = 0xAA;
        frame[1] = 0x55;

        frame[2] = (byte)(data.Length & 0xFF);
        frame[3] = (byte)(data.Length >> 8);

        frame[4] = (byte)mode;

        Buffer.BlockCopy(data, 0, frame, 5, data.Length);

        byte chk = (byte)mode;
        for (int i = 0; i < data.Length; i++)
            chk ^= data[i];

        frame[^1] = chk;

        return frame;
    }


public readonly record struct LineSegment(
    byte X1, byte Y1,
    byte X2, byte Y2);

    public void SendVectorLines(IReadOnlyList<LineSegment> lines)
    {
        if (_port == null || !_port.IsOpen)
            return;

        // Four bytes per line.
        if (lines.Count > 16000)
            throw new ArgumentException("Too many line segments.");

        var data = new byte[lines.Count * 4];

        for (int i = 0; i < lines.Count; i++)
        {
            var line = lines[i];
            int offset = i * 4;

            data[offset]     = line.X1;
            data[offset + 1] = line.Y1;
            data[offset + 2] = line.X2;
            data[offset + 3] = line.Y2;
        }

        lock (_lock)
        {
            if (IsDuplicate(data, FrameMode.Bit1Packed)) return;

            _currentMode = FrameMode.Bit1Packed;
            _latestFrame = data;
            _lastSentFrame = (byte[])_latestFrame.Clone();
            _lastSentMode = _currentMode;
            _hasLastSentFrame = true;
            TrySend();
        }
    }

    // =========================
    // DISPOSE
    // =========================

    public void Dispose()
    {
        _ackWatchdog.Dispose();

        if (_port != null && _port.IsOpen)
            _port.Close();
    }
}