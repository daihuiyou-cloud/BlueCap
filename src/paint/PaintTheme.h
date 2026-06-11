#pragma once

#include "theme/ThemeColors.h"

#include <QColor>

namespace paint {

struct Palette {
    ThemeColors colors;
    QColor windowOuter;
    QColor panelBg;
    QColor panelBorder;
    QColor cardBg;
    QColor cardHoverBg;
    QColor cardCheckedBg;
    QColor cardBorder;
    QColor cardCheckedBorder;
    QColor primary;
    QColor primary2;
    QColor accentGreen;
    QColor accentPurple;
    QColor danger;
    QColor text;
    QColor mutedText;
    QColor faintText;
    QColor hairline;
    QColor iconBg;
};

inline Palette theme(bool dark)
{
    Palette p;
    p.colors = ThemeColors::forMode(dark);
    const auto &a = p.colors.app;
    if (dark) {
        p.windowOuter = QColor(13, 16, 22);
        p.panelBg = a.surfaceBg;
        p.panelBorder = a.surfaceBorder;
        p.cardBg = QColor(25, 30, 40, 224);
        p.cardHoverBg = QColor(32, 38, 50, 238);
        p.cardCheckedBg = QColor(28, 36, 51, 246);
        p.cardBorder = QColor(61, 61, 61, 220);
        p.cardCheckedBorder = QColor(74, 139, 255);
        p.primary = QColor(76, 151, 255);
        p.primary2 = QColor(40, 103, 235);
        p.accentGreen = QColor(48, 198, 164);
        p.accentPurple = QColor(157, 105, 245);
        p.danger = QColor(244, 66, 76);
        p.text = QColor(232, 237, 246);
        p.mutedText = QColor(163, 174, 190);
        p.faintText = QColor(100, 114, 136);
        p.hairline = QColor(61, 61, 61);
        p.iconBg = QColor(37, 46, 62);
    } else {
        p.windowOuter = QColor(234, 240, 250);
        p.panelBg = a.surfaceBg;
        p.panelBorder = a.surfaceBorder;
        p.cardBg = QColor(255, 255, 255, 218);
        p.cardHoverBg = QColor(247, 251, 255, 245);
        p.cardCheckedBg = QColor(237, 245, 255, 248);
        p.cardBorder = QColor(210, 210, 210);
        p.cardCheckedBorder = QColor(9, 103, 242);
        p.primary = QColor(45, 132, 255);
        p.primary2 = QColor(9, 91, 238);
        p.accentGreen = QColor(25, 166, 136);
        p.accentPurple = QColor(137, 88, 236);
        p.danger = QColor(224, 57, 70);
        p.text = QColor(21, 31, 52);
        p.mutedText = QColor(92, 106, 128);
        p.faintText = QColor(134, 148, 170);
        p.hairline = QColor(214, 214, 214);
        p.iconBg = QColor(230, 238, 251);
    }
    return p;
}

}
