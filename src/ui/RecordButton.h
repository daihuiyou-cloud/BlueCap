#pragma once

#include <QAbstractButton>
#include <QPixmap>

class RecordButton : public QAbstractButton
{
    Q_OBJECT

public:
    explicit RecordButton(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    void setRecording(bool recording);
    void setDarkMode(bool dark);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void renderCache();

    bool m_recording = false;
    bool m_darkMode = false;
    bool m_hovered = false;
    bool m_pressed = false;
    QPixmap m_bgCache;
};
