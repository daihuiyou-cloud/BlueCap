Add-Type -AssemblyName System.Drawing

$sizes = @(16, 32, 48, 256)
$images = @()

$tileTop = [System.Drawing.Color]::FromArgb(167, 228, 255)
$tileMid = [System.Drawing.Color]::FromArgb(53, 155, 255)
$tileAccent = [System.Drawing.Color]::FromArgb(9, 103, 242)
$tileBottom = [System.Drawing.Color]::FromArgb(5, 76, 196)
$white = [System.Drawing.Color]::White
$cameraTop = [System.Drawing.Color]::FromArgb(248, 252, 255)
$cameraBottom = [System.Drawing.Color]::FromArgb(220, 238, 255)
$recordTop = [System.Drawing.Color]::FromArgb(255, 125, 134)
$recordMid = [System.Drawing.Color]::FromArgb(239, 51, 72)
$recordBottom = [System.Drawing.Color]::FromArgb(201, 31, 54)

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

foreach ($size in $sizes) {
    $bmp = New-Object System.Drawing.Bitmap($size, $size)
    $bmp.SetResolution(96, 96)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.Clear([System.Drawing.Color]::Transparent)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality

    $s = $size / 48.0
    $outerRect = [System.Drawing.RectangleF]::new(4 * $s, 3.5 * $s, 40 * $s, 40 * $s)
    $outerPath = New-RoundedRectPath $outerRect (13 * $s)

    if ($size -ge 32) {
        $shadowRect = [System.Drawing.RectangleF]::new(5 * $s, 6 * $s, 38 * $s, 38 * $s)
        $shadowPath = New-RoundedRectPath $shadowRect (13 * $s)
        $shadow = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(56, 6, 62, 158))
        $g.FillPath($shadow, $shadowPath)
        $shadow.Dispose()
        $shadowPath.Dispose()
    }

    $tileBrush = New-BlendBrush `
        ([System.Drawing.PointF]::new(8 * $s, 5 * $s)) `
        ([System.Drawing.PointF]::new(41 * $s, 43 * $s)) `
        @($tileTop, $tileMid, $tileAccent, $tileBottom) `
        @([single]0, [single]0.32, [single]0.68, [single]1)
    $g.FillPath($tileBrush, $outerPath)
    $tileBrush.Dispose()

    if ($size -ge 32) {
        $shinePath = New-Object System.Drawing.Drawing2D.GraphicsPath
        $shinePath.AddBezier((9.2 * $s), (10.2 * $s), (12.8 * $s), (6.8 * $s), (17.9 * $s), (5 * $s), (24.2 * $s), (5 * $s))
        $shinePath.AddLine((36 * $s), (5 * $s), (42 * $s), (11 * $s))
        $shinePath.AddLine((42 * $s), (11 * $s), (42 * $s), (17.6 * $s))
        $shinePath.AddBezier((36.2 * $s), (14.1 * $s), (29.4 * $s), (11.9 * $s), (22.6 * $s), (11.1 * $s), (22.6 * $s), (11.1 * $s))
        $shinePath.AddBezier((17.9 * $s), (10.5 * $s), (13.3 * $s), (10.4 * $s), (9.2 * $s), (10.2 * $s), (9.2 * $s), (10.2 * $s))
        $shinePath.CloseFigure()
        $shine = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(46, 255, 255, 255))
        $g.FillPath($shine, $shinePath)
        $shine.Dispose()
        $shinePath.Dispose()
    }

    if ($size -ge 24) {
        $cameraPath = New-Object System.Drawing.Drawing2D.GraphicsPath
        $cameraPath.AddArc((13 * $s), (13 * $s), (7.4 * $s), (7.4 * $s), 180, 90)
        $cameraPath.AddLine((20.4 * $s), (13 * $s), (29.6 * $s), (13 * $s))
        $cameraPath.AddBezier((31.5 * $s), (13 * $s), (33.2 * $s), (14.4 * $s), (33.6 * $s), (16.2 * $s), (33.6 * $s), (16.2 * $s))
        $cameraPath.AddLine((33.6 * $s), (16.2 * $s), (38.8 * $s), (13.5 * $s))
        $cameraPath.AddBezier((40.1 * $s), (12.8 * $s), (41.6 * $s), (13.8 * $s), (41.6 * $s), (15.2 * $s), (41.6 * $s), (15.2 * $s))
        $cameraPath.AddLine((41.6 * $s), (15.2 * $s), (41.6 * $s), (32.8 * $s))
        $cameraPath.AddBezier((41.6 * $s), (34.2 * $s), (40.1 * $s), (35.2 * $s), (38.8 * $s), (34.5 * $s), (38.8 * $s), (34.5 * $s))
        $cameraPath.AddLine((38.8 * $s), (34.5 * $s), (33.6 * $s), (31.8 * $s))
        $cameraPath.AddBezier((33.2 * $s), (33.6 * $s), (31.5 * $s), (35 * $s), (29.6 * $s), (35 * $s), (29.6 * $s), (35 * $s))
        $cameraPath.AddLine((29.6 * $s), (35 * $s), (16.7 * $s), (35 * $s))
        $cameraPath.AddArc((13 * $s), (27.6 * $s), (7.4 * $s), (7.4 * $s), 90, 90)
        $cameraPath.AddLine((13 * $s), (27.6 * $s), (13 * $s), (16.7 * $s))
        $cameraPath.CloseFigure()
        $g.FillPath([System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(245, 255, 255, 255)), $cameraPath)

        $bodyRect = [System.Drawing.RectangleF]::new(16.1 * $s, 16.2 * $s, 15.2 * $s, 15.6 * $s)
        $bodyPath = New-RoundedRectPath $bodyRect (4.4 * $s)
        $bodyBrush = [System.Drawing.Drawing2D.LinearGradientBrush]::new(
            [System.Drawing.PointF]::new(18 * $s, 16 * $s),
            [System.Drawing.PointF]::new(30 * $s, 32 * $s),
            $cameraTop,
            $cameraBottom)
        $g.FillPath($bodyBrush, $bodyPath)
        $bodyBrush.Dispose()

        $g.FillEllipse([System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(216, 255, 255, 255)),
            16.8 * $s, 17.1 * $s, 13.8 * $s, 13.8 * $s)

        $recordBrush = New-BlendBrush `
            ([System.Drawing.PointF]::new(19 * $s, 18.5 * $s)) `
            ([System.Drawing.PointF]::new(29 * $s, 31 * $s)) `
            @($recordTop, $recordMid, $recordBottom) `
            @([single]0, [single]0.55, [single]1)
        $g.FillEllipse($recordBrush, 18.1 * $s, 18.4 * $s, 11.2 * $s, 11.2 * $s)
        $recordBrush.Dispose()

        if ($size -ge 48) {
            $g.FillEllipse([System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(158, 255, 255, 255)),
                20.1 * $s, 20.5 * $s, 2.8 * $s, 2.8 * $s)
            $penA = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(242, 191, 228, 255), 1.6 * $s)
            $penA.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
            $penA.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
            $g.DrawLine($penA, (14.2 * $s), (18.8 * $s), (14.2 * $s), (16.9 * $s))
            $g.DrawLine($penA, (14.2 * $s), (16.9 * $s), (16.9 * $s), (14.2 * $s))
            $g.DrawLine($penA, (16.9 * $s), (14.2 * $s), (19 * $s), (14.2 * $s))
            $penA.Dispose()
        }

        $bodyPath.Dispose()
        $cameraPath.Dispose()
    } else {
        $recordBrush = New-BlendBrush `
            ([System.Drawing.PointF]::new(15 * $s, 15 * $s)) `
            ([System.Drawing.PointF]::new(31 * $s, 31 * $s)) `
            @($recordTop, $recordMid, $recordBottom) `
            @([single]0, [single]0.55, [single]1)
        $g.FillEllipse([System.Drawing.SolidBrush]::new($white), 13 * $s, 13 * $s, 22 * $s, 22 * $s)
        $g.FillEllipse($recordBrush, 17 * $s, 17 * $s, 14 * $s, 14 * $s)
        $recordBrush.Dispose()
    }

    $outerPath.Dispose()
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
