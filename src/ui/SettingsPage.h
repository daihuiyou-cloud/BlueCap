#pragma once

#include <QWidget>

class QComboBox;
class QLineEdit;
class QSpinBox;

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

signals:
    void frameRateChanged(int fps);
    void savePathChanged(const QString &path);

private slots:
    void browsePath();
    void applySettings();

private:
    QLineEdit *m_pathEdit = nullptr;
    QSpinBox *m_fpsSpin = nullptr;
    QComboBox *m_qualityCombo = nullptr;
};
