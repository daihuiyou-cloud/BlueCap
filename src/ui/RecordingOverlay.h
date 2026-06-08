#pragma once

#include <QWidget>

class QTimer;

class RecordingOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit RecordingOverlay(QWidget *parent = nullptr);

    void showForRegion(const QRect &region);
    void showForFullscreen();
    void hideOverlay();

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QRect m_area;
    QTimer *m_pulseTimer = nullptr;
    bool m_pulseState = false;
    bool m_isFullscreen = false;
};
