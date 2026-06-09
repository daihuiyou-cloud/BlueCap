#pragma once

#include <QLabel>

class SaveFeedbackLabel : public QLabel
{
    Q_OBJECT
public:
    explicit SaveFeedbackLabel(QWidget *parent = nullptr);
    void setSuccess(bool success);
    void setDarkMode(bool dark);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    bool m_darkMode = false;
    bool m_success = false;
    bool m_hasResult = false;
};
