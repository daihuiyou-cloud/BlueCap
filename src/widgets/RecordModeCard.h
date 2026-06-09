#pragma once

#include "RecordMode.h"
#include "paint/PaintTheme.h"

#include <QAbstractButton>
#include <QString>

class RecordModeCard : public QAbstractButton {
    Q_OBJECT

public:
    explicit RecordModeCard(RecordMode mode, const QString &title,
                            const QString &subtitle, const QString &iconPath,
                            QWidget *parent = nullptr);

    RecordMode mode() const { return m_mode; }
    void setDarkMode(bool dark);

    QSize sizeHint() const override;

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void recolorIcon();

    RecordMode m_mode;
    QString m_title;
    QString m_subtitle;
    QString m_iconPath;
    QPixmap m_iconCache;
    bool m_darkMode = false;
    bool m_hovered = false;
    paint::Palette m_palette;
};
