#pragma once

#include "utils/WindowDragHelper.h"
#include "utils/win32/WindowEnumerator.h"

#include <QDialog>
#include <QHash>
#include <QList>
#include <QPoint>
#include <QPixmap>
#include <QStringList>

class ActionButton;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTimer;
class QWidget;
class StyledListWidget;

class WindowPicker : public QDialog
{
    Q_OBJECT

public:
    explicit WindowPicker(QWidget *parent = nullptr);

    void setDarkMode(bool dark);
    QString selectedWindow() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void populateList(const QString &filter = {});
    void refreshWindows();

    QWidget *m_surface = nullptr;
    QWidget *m_titleBar = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QPushButton *m_okBtn = nullptr;
    ActionButton *m_cancelBtn = nullptr;
    StyledListWidget *m_list = nullptr;
    QLineEdit *m_filterEdit = nullptr;
    QPushButton *m_refreshBtn = nullptr;
    QHash<qulonglong, QPixmap> m_iconCache;
    QList<WindowEntry> m_windows;
    window_drag::State m_dragState;
    QTimer *m_filterDebounce = nullptr;
    QString m_filterPending;
    bool m_darkMode = false;
};
