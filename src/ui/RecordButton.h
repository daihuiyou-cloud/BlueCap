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

private:
    void renderCache();

    bool m_recording = false;
    bool m_darkMode = false;
    QPixmap m_bgCache;
};
