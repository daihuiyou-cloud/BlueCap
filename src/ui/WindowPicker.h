#pragma once

#include "recorder/RecorderController.h"

#include <QDialog>
#include <QList>
#include <QStringList>

class QListWidget;
class QLineEdit;
class QPushButton;

class WindowPicker : public QDialog
{
    Q_OBJECT

public:
    explicit WindowPicker(QWidget *parent = nullptr);

    QString selectedWindow() const;

private:
    void populateList(const QString &filter = {});
    void refreshWindows();

    QListWidget *m_list = nullptr;
    QLineEdit *m_filterEdit = nullptr;
    QPushButton *m_refreshBtn = nullptr;
    QList<RecorderController::WindowEntry> m_windows;
};
