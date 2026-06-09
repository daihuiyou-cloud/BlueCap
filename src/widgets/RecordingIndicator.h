#pragma once

#include "paint/PaintTheme.h"

#include <QLabel>

class QTimer;

class RecordingIndicator : public QLabel
{
    Q_OBJECT
public:
    explicit RecordingIndicator(QWidget *parent = nullptr);
    void setDarkMode(bool dark);
    void startPulse();
    void stopPulse();
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QTimer *m_pulseTimer = nullptr;
    bool m_pulseState = false;
    bool m_darkMode = false;
    paint::Palette m_palette;
};
