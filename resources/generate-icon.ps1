Add-Type -AssemblyName System.Drawing

$sizes = @(16, 32, 48, 256)
$images = @()

$blueLight = [System.Drawing.Color]::FromArgb(128, 200, 255)
$blueMid = [System.Drawing.Color]::FromArgb(23, 118, 244)
$blueDark = [System.Drawing.Color]::FromArgb(7, 88, 232)
$white = [System.Drawing.Color]::White
$red = [System.Drawing.Color]::FromArgb(239, 51, 72)

foreach ($size in $sizes) {
    $bmp = New-Object System.Drawing.Bitmap($size, $size)
    $bmp.SetResolution(96, 96)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half

    $s = $size / 48.0

    $rectPen = [System.Drawing.Pen]::new([System.Drawing.Drawing2D.LinearGradientBrush]::new(
        [System.Drawing.Point]::new(0, 0), [System.Drawing.Point]::new($size, $size),
        $blueLight, $blueDark), 0)

    $outerRect = [System.Drawing.RectangleF]::new(4 * $s, 4 * $s, 40 * $s, 40 * $s)
    $outerRad = 12 * $s
    $outerPath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $outerPath.AddArc($outerRect.X, $outerRect.Y, $outerRad * 2, $outerRad * 2, 180, 90)
    $outerPath.AddArc($outerRect.Right - $outerRad * 2, $outerRect.Y, $outerRad * 2, $outerRad * 2, 270, 90)
    $outerPath.AddArc($outerRect.Right - $outerRad * 2, $outerRect.Bottom - $outerRad * 2, $outerRad * 2, $outerRad * 2, 0, 90)
    $outerPath.AddArc($outerRect.X, $outerRect.Bottom - $outerRad * 2, $outerRad * 2, $outerRad * 2, 90, 90)
    $outerPath.CloseFigure()

    $gradBrush = [System.Drawing.Drawing2D.LinearGradientBrush]::new(
        [System.Drawing.Point]::new(0, 0), [System.Drawing.Point]::new($size, $size),
        $blueLight, $blueDark)
    $g.FillPath($gradBrush, $outerPath)

    if ($size -ge 32) {
        $innerR = [System.Drawing.RectangleF]::new(12 * $s, 14 * $s, 24 * $s, 20 * $s)
        $innerRad = 6 * $s
        $innerPath = New-Object System.Drawing.Drawing2D.GraphicsPath
        $innerPath.AddArc($innerR.X, $innerR.Y, $innerRad * 2, $innerRad * 2, 180, 90)
        $innerPath.AddArc($innerR.Right - $innerRad * 2, $innerR.Y, $innerRad * 2, $innerRad * 2, 270, 90)
        $innerPath.AddArc($innerR.Right - $innerRad * 2, $innerR.Bottom - $innerRad * 2, $innerRad * 2, $innerRad * 2, 0, 90)
        $innerPath.AddArc($innerR.X, $innerR.Bottom - $innerRad * 2, $innerRad * 2, $innerRad * 2, 90, 90)
        $innerPath.CloseFigure()

        $g.FillPath([System.Drawing.SolidBrush]::new($white), $innerPath)

        $fillR = [System.Drawing.RectangleF]::new(15 * $s, 17 * $s, 18 * $s, 14 * $s)
        $fillRad = 4 * $s
        $fillPath = New-Object System.Drawing.Drawing2D.GraphicsPath
        $fillPath.AddArc($fillR.X, $fillR.Y, $fillRad * 2, $fillRad * 2, 180, 90)
        $fillPath.AddArc($fillR.Right - $fillRad * 2, $fillR.Y, $fillRad * 2, $fillRad * 2, 270, 90)
        $fillPath.AddArc($fillR.Right - $fillRad * 2, $fillR.Bottom - $fillRad * 2, $fillRad * 2, $fillRad * 2, 0, 90)
        $fillPath.AddArc($fillR.X, $fillR.Bottom - $fillRad * 2, $fillRad * 2, $fillRad * 2, 90, 90)
        $fillPath.CloseFigure()
        $lightBg = [System.Drawing.Color]::FromArgb(244, 248, 255)
        $g.FillPath([System.Drawing.SolidBrush]::new($lightBg), $fillPath)

        $cx = 24 * $s
        $cy = 24 * $s
        $cr = 5 * $s
        $g.FillEllipse([System.Drawing.SolidBrush]::new($red), $cx - $cr, $cy - $cr, $cr * 2, $cr * 2)
    } else {
        $g.FillEllipse([System.Drawing.SolidBrush]::new($red),
            $outerRect.X + $outerRect.Width * 0.25,
            $outerRect.Y + $outerRect.Height * 0.25,
            $outerRect.Width * 0.5,
            $outerRect.Height * 0.5)
    }

    $g.Dispose()

    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $images += @{ Data = $ms.ToArray(); Width = $size }
    $ms.Dispose()
    $bmp.Dispose()
}

$icoPath = Join-Path $PSScriptRoot "icons\app-icon.ico"
$parent = Split-Path $icoPath -Parent
if (-not (Test-Path $parent)) {
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
}

$fs = [System.IO.File]::Open($icoPath, [System.IO.FileMode]::Create)
$writer = New-Object System.IO.BinaryWriter($fs)

$writer.Write([UInt16]0)
$writer.Write([UInt16]1)
$writer.Write([UInt16]$images.Count)

$offset = 6 + $images.Count * 16
foreach ($img in $images) {
    $w = if ($img.Width -eq 256) { 0 } else { $img.Width }
    $h = if ($img.Width -eq 256) { 0 } else { $img.Width }
    $writer.Write([Byte]$w)
    $writer.Write([Byte]$h)
    $writer.Write([Byte]0)
    $writer.Write([Byte]0)
    $writer.Write([UInt16]1)
    $writer.Write([UInt16]32)
    $writer.Write([UInt32]$img.Data.Length)
    $writer.Write([UInt32]$offset)
    $offset += $img.Data.Length
}

foreach ($img in $images) {
    $writer.Write($img.Data)
}

$writer.Dispose()
$fs.Dispose()

Write-Output "ICO generated: $icoPath"
Write-Output "Sizes: $($images.Count) ($($sizes -join ', '))"
