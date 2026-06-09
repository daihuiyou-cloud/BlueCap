#pragma once

#include <QString>
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
    void setStatusText(const QString &text);
    void setHintText(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QRect m_area;
    QTimer *m_pulseTimer = nullptr;
    bool m_pulseState = false;
    bool m_isFullscreen = false;
    QString m_statusText;
    QString m_hintText;
};
