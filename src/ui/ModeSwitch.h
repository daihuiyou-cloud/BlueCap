#pragma once

#include <QWidget>

class QButtonGroup;

class ModeSwitch : public QWidget
{
    Q_OBJECT

public:
    explicit ModeSwitch(QWidget *parent = nullptr);

    QString currentMode() const;

signals:
    void modeChanged(const QString &mode);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QButtonGroup *m_group = nullptr;
};
