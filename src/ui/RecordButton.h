#pragma once

#include <QAbstractButton>

class RecordButton : public QAbstractButton
{
    Q_OBJECT

public:
    explicit RecordButton(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    void setRecording(bool recording);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool m_recording = false;
};
