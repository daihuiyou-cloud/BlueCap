#pragma once

#include <QColor>
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
    void setDarkMode(bool dark);

signals:
    void modeChanged(RecordMode mode);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QButtonGroup *m_group = nullptr;
    QColor m_pillBorder;
    QColor m_pillFill;
    QColor m_dividerColor;
};
