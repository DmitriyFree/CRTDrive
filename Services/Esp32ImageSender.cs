using System;
using System.IO.Ports;
using System.Threading;

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
    private const int ACK_TIMEOUT_MS = 200;//67; // ~1/15 sec

    private enum FrameMode : byte
    {
        Bit1Packed = 0,
        Bit8Raw = 1
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
            if (IsDuplicate(raw)) return;

            _currentMode = FrameMode.Bit8Raw;
            _latestFrame = raw;

            TrySend();
        }
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
            Console.WriteLine("Frame sent!");
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
                    Console.WriteLine("ACK!");

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

    private bool IsDuplicate(byte[] frame)
    {
        if (_lastSentFrame == null) return false;
        return AreEqual(_lastSentFrame, frame);
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