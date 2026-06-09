#pragma once

#include <QProxyStyle>

class QPainter;
struct ThemeAppearance;

class BlueCapStyle : public QProxyStyle
{
    Q_OBJECT

public:
    explicit BlueCapStyle();

    void setDarkMode(bool dark);
    bool isDarkMode() const { return s_darkMode; }

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget = nullptr) const override;

    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget = nullptr) const override;

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget = nullptr) const override;

    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &size, const QWidget *widget) const override;

    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                         SubControl sc, const QWidget *widget) const override;

    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override;

private:
    static const BlueCapStyle *styleInstance();
    static const ThemeAppearance &styleColors();

    static bool s_darkMode;
};
