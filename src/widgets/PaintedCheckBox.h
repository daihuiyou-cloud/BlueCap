#pragma once

#include "paint/PaintTheme.h"

#include <QCheckBox>

class PaintedCheckBox : public QCheckBox {
    Q_OBJECT

public:
    explicit PaintedCheckBox(const QString &text, QWidget *parent = nullptr);
    void setDarkMode(bool dark);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    bool m_darkMode = false;
    bool m_hovered = false;
    paint::Palette m_palette;
};
