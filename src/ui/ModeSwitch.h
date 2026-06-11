#pragma once

#include "recorder/RecordMode.h"

#include <QList>
#include <QWidget>

class QButtonGroup;
class RecordModeCard;

class ModeSwitch : public QWidget {
    Q_OBJECT

public:
    explicit ModeSwitch(QWidget *parent = nullptr);

    RecordMode currentMode() const;
    void setModeEnabled(bool enabled);
    void setDarkMode(bool dark);

signals:
    void modeChanged(RecordMode mode);

private:
    void updateCards();

    QButtonGroup *m_group = nullptr;
    QList<RecordModeCard *> m_buttons;
    bool m_darkMode = false;
};
