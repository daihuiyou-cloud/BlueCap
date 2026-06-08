#pragma once

#include <QDialog>
#include <QStringList>

class QListWidget;
class QLineEdit;

class WindowPicker : public QDialog
{
    Q_OBJECT

public:
    explicit WindowPicker(QWidget *parent = nullptr);

    QString selectedWindow() const;

private:
    QListWidget *m_list = nullptr;
    QLineEdit *m_filterEdit = nullptr;
    QStringList m_windows;
    QString m_selected;
};
