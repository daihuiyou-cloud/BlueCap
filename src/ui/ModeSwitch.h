#pragma once

#include <QWidget>

class QButtonGroup;

enum class RecordMode { FullScreen, Region, Window };

class ModeSwitch : public QWidget
{
    Q_OBJECT

public:
    explicit ModeSwitch(QWidget *parent = nullptr);

    RecordMode currentMode() const;
    void setModeEnabled(bool enabled);

signals:
    void modeChanged(RecordMode mode);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QButtonGroup *m_group = nullptr;
};
