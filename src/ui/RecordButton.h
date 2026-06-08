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

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void renderCache();

    bool m_recording = false;
    QPixmap m_bgCache;
};
