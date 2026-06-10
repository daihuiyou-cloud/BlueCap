#pragma once

#include "paint/PaintTheme.h"
#include "utils/WindowDragHelper.h"

#include <QDialog>
#include <QString>

class QLabel;

class PaintedDialog : public QDialog {
    Q_OBJECT

public:
    explicit PaintedDialog(const QString &title, const QString &message, QWidget *parent = nullptr);
    void setDarkMode(bool dark);

    static bool question(QWidget *parent, const QString &title, const QString &message,
                         const QString &yesText = QStringLiteral("确定"),
                         const QString &noText = QStringLiteral("取消"));
    static void warning(QWidget *parent, const QString &title, const QString &message);
    static QString getText(QWidget *parent, const QString &title, const QString &label,
                           const QString &text, bool *ok);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QLabel *m_titleLabel = nullptr;
    window_drag::State m_dragState;
    bool m_darkMode = false;
    paint::Palette m_palette;
};
