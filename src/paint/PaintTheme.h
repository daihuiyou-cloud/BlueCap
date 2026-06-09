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
        p.windowOuter = QColor(10, 12, 17);
        p.panelBg = QColor(16, 19, 26, 248);
        p.panelBorder = QColor(42, 50, 66);
        p.cardBg = QColor(24, 28, 37, 230);
        p.cardHoverBg = QColor(30, 36, 48, 242);
        p.cardCheckedBg = QColor(24, 31, 47, 248);
        p.cardBorder = QColor(45, 52, 68);
        p.cardCheckedBorder = QColor(55, 116, 255);
        p.primary = QColor(57, 133, 255);
        p.primary2 = QColor(34, 86, 236);
        p.accentGreen = QColor(43, 198, 164);
        p.accentPurple = QColor(153, 98, 255);
        p.danger = QColor(244, 66, 76);
        p.text = QColor(226, 232, 242);
        p.mutedText = QColor(154, 165, 184);
        p.faintText = QColor(94, 107, 130);
        p.hairline = QColor(53, 61, 78);
        p.iconBg = QColor(35, 43, 62);
    } else {
        p.windowOuter = QColor(234, 240, 250);
        p.panelBg = a.surfaceBg;
        p.panelBorder = a.surfaceBorder;
        p.cardBg = QColor(255, 255, 255, 226);
        p.cardHoverBg = QColor(247, 251, 255, 245);
        p.cardCheckedBg = QColor(237, 245, 255, 248);
        p.cardBorder = QColor(198, 211, 232);
        p.cardCheckedBorder = QColor(9, 103, 242);
        p.primary = QColor(45, 132, 255);
        p.primary2 = QColor(9, 91, 238);
        p.accentGreen = QColor(25, 166, 136);
        p.accentPurple = QColor(137, 88, 236);
        p.danger = QColor(224, 57, 70);
        p.text = QColor(21, 31, 52);
        p.mutedText = QColor(92, 106, 128);
        p.faintText = QColor(134, 148, 170);
        p.hairline = QColor(207, 218, 236);
        p.iconBg = QColor(230, 238, 251);
    }
    return p;
}

}
