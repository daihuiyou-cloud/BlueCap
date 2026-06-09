#pragma once

#include <QPixmap>
#include <QImage>

#include <windows.h>

namespace win32 {

inline QPixmap hBitmapToPixmap(HBITMAP hBitmap)
{
    BITMAP bm;
    if (!GetObject(hBitmap, sizeof(bm), &bm) || !bm.bmWidth || !bm.bmHeight)
        return {};

    QImage img(bm.bmWidth, bm.bmHeight, QImage::Format_ARGB32_Premultiplied);
    if (img.isNull()) return {};

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = bm.bmWidth;
    bi.bmiHeader.biHeight = -bm.bmHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(nullptr);
    int lines = GetDIBits(hdc, hBitmap, 0, bm.bmHeight, img.bits(), &bi, DIB_RGB_COLORS);
    ReleaseDC(nullptr, hdc);

    return lines > 0 ? QPixmap::fromImage(img) : QPixmap();
}

inline QPixmap iconFromHICON(HICON hIcon)
{
    if (!hIcon) return {};
    ICONINFO ii = {};
    if (!GetIconInfo(hIcon, &ii))
        return {};
    BITMAP bm = {};
    if (!GetObject(ii.hbmColor, sizeof(bm), &bm)) {
        DeleteObject(ii.hbmColor);
        DeleteObject(ii.hbmMask);
        return {};
    }
    QPixmap px = hBitmapToPixmap(ii.hbmColor);
    DeleteObject(ii.hbmColor);
    DeleteObject(ii.hbmMask);
    return px;
}

}
