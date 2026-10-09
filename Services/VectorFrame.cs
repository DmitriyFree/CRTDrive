
using System.Collections.Generic;
using Avalonia.Media.Imaging;

namespace CrtDrive.Services;

public sealed class VectorFrame
{
    public required List<Esp32ImageSender.LineSegment> Lines { get; init; }
    public required WriteableBitmap Preview { get; init; }
}