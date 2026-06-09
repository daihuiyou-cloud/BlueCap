#pragma once

#include <QWidget>

class QLabel;
class QPropertyAnimation;
class QTimer;

class ToastWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal toastOpacity READ toastOpacity WRITE setToastOpacity)

public:
    explicit ToastWidget(QWidget *parent = nullptr);
    void showMessage(const QString &message, int durationMs = 5000);
    void dismissImmediately();
    void setDarkMode(bool dark);

    qreal toastOpacity() const { return m_toastOpacity; }
    void setToastOpacity(qreal opacity);

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void positionAtBottom();
    void fadeIn();
    void fadeOut();
    void applyTextColor();

    QLabel *m_label = nullptr;
    QTimer *m_hideTimer = nullptr;
    QPropertyAnimation *m_fadeAnim = nullptr;
    bool m_darkMode = false;
    bool m_animatingHide = false;
    qreal m_toastOpacity = 0.0;
};
