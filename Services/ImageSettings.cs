using Avalonia.Controls;
using Avalonia.Threading;
using CrtDrive.Services;
using Avalonia.Media.Imaging;
using Avalonia.Interactivity;
using System;

namespace CrtDrive;
public class ImageSettings
{
    public float Brightness { get; set; } = 0f;     // -1 .. +1
    public float Contrast { get; set; } = 1f;       // 0.5 .. 3
    public byte Threshold { get; set; } = 170;      // 0..255

    public int Erode { get; set; } = 0;             // iterations
    public int Dilate { get; set; } = 0;
    public float EdgeStrength { get; set; } = 0f; 
    public float Gamma { get; set; } = 1f;
    public float AdaptiveExposure { get; set; } = 1.0f; // 0..1 strength
}