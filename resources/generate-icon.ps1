Add-Type -AssemblyName System.Drawing

$sizes = @(16, 32, 48, 256)
$pngPath = Join-Path $PSScriptRoot "icons\app-logo.png"
$icoPath = Join-Path $PSScriptRoot "icons\app-icon.ico"
$images = @()

function New-RoundedRectPath {
    param(
        [System.Drawing.RectangleF]$Rect,
        [double]$Radius
    )

    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $diameter = [float]($Radius * 2)
    $path.AddArc($Rect.X, $Rect.Y, $diameter, $diameter, 180, 90)
    $path.AddArc($Rect.Right - $diameter, $Rect.Y, $diameter, $diameter, 270, 90)
    $path.AddArc($Rect.Right - $diameter, $Rect.Bottom - $diameter, $diameter, $diameter, 0, 90)
    $path.AddArc($Rect.X, $Rect.Bottom - $diameter, $diameter, $diameter, 90, 90)
    $path.CloseFigure()
    return $path
}

function New-BlendBrush {
    param(
        [System.Drawing.PointF]$Start,
        [System.Drawing.PointF]$End,
        [System.Drawing.Color[]]$Colors,
        [single[]]$Positions
    )

    $brush = [System.Drawing.Drawing2D.LinearGradientBrush]::new($Start, $End, $Colors[0], $Colors[$Colors.Length - 1])
    $blend = [System.Drawing.Drawing2D.ColorBlend]::new()
    $blend.Colors = $Colors
    $blend.Positions = $Positions
    $brush.InterpolationColors = $blend
    return $brush
}

function New-AppLogoPng {
    param([string]$Path)

    $size = 512
    $bmp = [System.Drawing.Bitmap]::new($size, $size)
    $bmp.SetResolution(96, 96)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.Clear([System.Drawing.Color]::Transparent)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality

    $navy = [System.Drawing.Color]::FromArgb(255, 18, 31, 108)
    $bodyTop = [System.Drawing.Color]::FromArgb(255, 48, 105, 215)
    $bodyMid = [System.Drawing.Color]::FromArgb(255, 24, 69, 168)
    $bodyBottom = [System.Drawing.Color]::FromArgb(255, 15, 36, 119)
    $cyan = [System.Drawing.Color]::FromArgb(255, 82, 223, 238)

    $shadowBrush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(72, 8, 22, 86))
    $cameraShadow = New-RoundedRectPath ([System.Drawing.RectangleF]::new(90, 170, 300, 240)) 46
    $matrix = [System.Drawing.Drawing2D.Matrix]::new()
    $matrix.RotateAt(-6, [System.Drawing.PointF]::new(240, 290))
    $cameraShadow.Transform($matrix)
    $g.FillPath($shadowBrush, $cameraShadow)
    $matrix.Dispose()
    $cameraShadow.Dispose()
    $shadowBrush.Dispose()

    $cameraPath = New-RoundedRectPath ([System.Drawing.RectangleF]::new(82, 150, 296, 236)) 42
    $bodyMatrix = [System.Drawing.Drawing2D.Matrix]::new()
    $bodyMatrix.RotateAt(-6, [System.Drawing.PointF]::new(230, 270))
    $cameraPath.Transform($bodyMatrix)

    $bodyBrush = New-BlendBrush `
        ([System.Drawing.PointF]::new(110, 140)) `
        ([System.Drawing.PointF]::new(360, 400)) `
        @($bodyTop, $bodyMid, $bodyBottom) `
        @([single]0, [single]0.54, [single]1)
    $g.FillPath($bodyBrush, $cameraPath)
    $bodyBrush.Dispose()

    $outlinePen = [System.Drawing.Pen]::new($navy, 18)
    $outlinePen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
    $g.DrawPath($outlinePen, $cameraPath)
    $outlinePen.Dispose()

    $topHighlight = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(110, 117, 224, 255), 8)
    $topHighlight.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
    $topHighlight.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $g.DrawLine($topHighlight, 128, 164, 330, 142)
    $topHighlight.Dispose()

    $videoPath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $videoPath.AddPolygon(@(
        [System.Drawing.PointF]::new(373, 204),
        [System.Drawing.PointF]::new(456, 150),
        [System.Drawing.PointF]::new(472, 308),
        [System.Drawing.PointF]::new(390, 270)
    ))
    $videoBrush = New-BlendBrush `
        ([System.Drawing.PointF]::new(380, 170)) `
        ([System.Drawing.PointF]::new(470, 300)) `
        @([System.Drawing.Color]::FromArgb(255, 73, 214, 235), [System.Drawing.Color]::FromArgb(255, 31, 86, 174)) `
        @([single]0, [single]1)
    $g.FillPath($videoBrush, $videoPath)
    $videoBrush.Dispose()
    $videoPen = [System.Drawing.Pen]::new($navy, 16)
    $videoPen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
    $g.DrawPath($videoPen, $videoPath)
    $videoPen.Dispose()

    $innerVideoPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(130, 186, 249, 255), 8)
    $innerVideoPen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
    $g.DrawLine($innerVideoPen, 430, 184, 442, 282)
    $innerVideoPen.Dispose()
    $videoPath.Dispose()

    foreach ($bubble in @(
        @{ X = 128; Y = 82; D = 84 },
        @{ X = 230; Y = 50; D = 110 }
    )) {
        $ringRect = [System.Drawing.RectangleF]::new($bubble.X, $bubble.Y, $bubble.D, $bubble.D)
        $ringBrush = New-BlendBrush `
            ([System.Drawing.PointF]::new($bubble.X, $bubble.Y)) `
            ([System.Drawing.PointF]::new($bubble.X + $bubble.D, $bubble.Y + $bubble.D)) `
            @([System.Drawing.Color]::FromArgb(255, 58, 104, 216), [System.Drawing.Color]::FromArgb(255, 10, 28, 103)) `
            @([single]0, [single]1)
        $g.FillEllipse($ringBrush, $ringRect)
        $ringBrush.Dispose()
        $g.FillEllipse([System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(210, 79, 219, 236)),
            $bubble.X + $bubble.D * 0.18, $bubble.Y + $bubble.D * 0.18,
            $bubble.D * 0.62, $bubble.D * 0.62)
    }

    $lensOuterBrush = New-BlendBrush `
        ([System.Drawing.PointF]::new(170, 220)) `
        ([System.Drawing.PointF]::new(290, 340)) `
        @([System.Drawing.Color]::FromArgb(255, 35, 113, 202), [System.Drawing.Color]::FromArgb(255, 8, 28, 103)) `
        @([single]0, [single]1)
    $g.FillEllipse($lensOuterBrush, 168, 218, 128, 128)
    $lensOuterBrush.Dispose()
    $g.DrawEllipse([System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(230, 85, 191, 226), 9), 180, 230, 104, 104)

    $lensBrush = New-BlendBrush `
        ([System.Drawing.PointF]::new(205, 240)) `
        ([System.Drawing.PointF]::new(260, 315)) `
        @([System.Drawing.Color]::FromArgb(255, 96, 224, 244), [System.Drawing.Color]::FromArgb(255, 12, 43, 134)) `
        @([single]0, [single]1)
    $g.FillEllipse($lensBrush, 196, 246, 76, 76)
    $lensBrush.Dispose()
    $g.FillEllipse([System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(190, 223, 255, 255)), 228, 252, 20, 20)
    $g.FillEllipse([System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(120, 223, 255, 255)), 252, 276, 10, 10)
    $g.FillEllipse([System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(210, 94, 223, 230)), 310, 190, 36, 36)
    $g.DrawEllipse([System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(190, 166, 246, 255), 6), 310, 190, 36, 36)

    $cameraPath.Dispose()
    $bodyMatrix.Dispose()
    $g.Dispose()

    $parent = Split-Path $Path -Parent
    if (-not (Test-Path $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}

if (-not (Test-Path $pngPath)) {
    New-AppLogoPng $pngPath
}

$source = [System.Drawing.Bitmap]::FromFile($pngPath)
foreach ($size in $sizes) {
    $bmp = [System.Drawing.Bitmap]::new($size, $size)
    $bmp.SetResolution(96, 96)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.Clear([System.Drawing.Color]::Transparent)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $g.DrawImage($source, [System.Drawing.Rectangle]::new(0, 0, $size, $size))
    $g.Dispose()

    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $images += @{ Data = $ms.ToArray(); Width = $size }
    $ms.Dispose()
    $bmp.Dispose()
}
$source.Dispose()

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

Write-Output "PNG source: $pngPath"
Write-Output "ICO generated: $icoPath"
Write-Output "Sizes: $($images.Count) ($($sizes -join ', '))"
