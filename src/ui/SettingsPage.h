#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;
class QTimer;

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

signals:
    void frameRateChanged(int fps);
    void presetChanged(const QString &preset);
    void savePathChanged(const QString &path);
    void confirmStopChanged(bool confirm);
    void showCursorChanged(bool show);
    void startTimeoutChanged(int ms);
    void stopTimeoutChanged(int ms);

private slots:
    void browsePath();
    void applySettings();

private:
    QLineEdit *m_pathEdit = nullptr;
    QSpinBox *m_fpsSpin = nullptr;
    QComboBox *m_qualityCombo = nullptr;
    QCheckBox *m_confirmStopCheck = nullptr;
    QCheckBox *m_showCursorCheck = nullptr;
    QSpinBox *m_startTimeoutSpin = nullptr;
    QSpinBox *m_stopTimeoutSpin = nullptr;
    QLabel *m_saveFeedback = nullptr;
    QTimer *m_feedbackTimer = nullptr;
};
