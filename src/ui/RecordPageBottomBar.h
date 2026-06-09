#pragma once

#include "widgets/BottomBar.h"
#include <QString>

class QFrame;
class QLabel;
class QPushButton;
class BottomNavSection;

class RecordPageBottomBar : public BottomBar
{
    Q_OBJECT

public:
    explicit RecordPageBottomBar(QWidget *parent = nullptr);

    void setRecentVideoDetail(const QString &text);
    void setDarkMode(bool dark) override;

signals:
    void recentVideosClicked();
    void openSaveFolderRequested();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void updateIcons();

    QFrame *m_separator = nullptr;
    BottomNavSection *m_bottomNavSection = nullptr;
    QLabel *m_recentIcon = nullptr;
    QLabel *m_recentTitle = nullptr;
    QLabel *m_recentDetailLabel = nullptr;
    QLabel *m_keyboardIcon = nullptr;
    QLabel *m_shortcutLabel = nullptr;
    QLabel *m_chevronIcon = nullptr;
    QPushButton *m_openFolderIcon = nullptr;
    bool m_darkMode = false;
};
