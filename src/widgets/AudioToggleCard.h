#pragma once

#include "paint/PaintTheme.h"

#include <QWidget>

class AudioToggleCard : public QWidget {
    Q_OBJECT

public:
    enum Accent { Microphone, SystemAudio };

    explicit AudioToggleCard(const QString &title, Accent accent, QWidget *parent = nullptr);
    void setDarkMode(bool dark);
    bool isAudioEnabled() const { return m_audioEnabled; }
    void setAudioEnabled(bool on);

signals:
    void toggled(bool on);

protected:
    void changeEvent(QEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_title;
    QString m_enabledText;
    QString m_disabledText;
    Accent m_accent;
    bool m_darkMode = false;
    bool m_hovered = false;
    bool m_pressed = false;
    bool m_audioEnabled = true;
    paint::Palette m_palette;
};
