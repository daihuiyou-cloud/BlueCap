#pragma once

#include <QFrame>
#include <QString>

class QLabel;
class QPushButton;

class RecordPageBottomBar : public QFrame
{
    Q_OBJECT

public:
    explicit RecordPageBottomBar(QWidget *parent = nullptr);

    void setRecentVideoDetail(const QString &text);
    void setDarkMode(bool dark);

signals:
    void recentVideosClicked();
    void openSaveFolderRequested();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void updateIcons();

    QFrame *m_bottomNavSection = nullptr;
    QLabel *m_recentIcon = nullptr;
    QLabel *m_recentDetailLabel = nullptr;
    QLabel *m_keyboardIcon = nullptr;
    QLabel *m_chevronIcon = nullptr;
    QPushButton *m_openFolderIcon = nullptr;
    bool m_darkMode = false;
};
