#pragma once

#include <QDialog>
#include <QMap>
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
    void populateList(const QString &filter = {});

    QListWidget *m_list = nullptr;
    QLineEdit *m_filterEdit = nullptr;
    QMap<QString, QString> m_windows;
};
