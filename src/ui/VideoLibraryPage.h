#pragma once

#include <QWidget>

class QLabel;
class QListWidget;
class QStackedWidget;
class VideoLibrary;

class VideoLibraryPage : public QWidget
{
    Q_OBJECT

public:
    explicit VideoLibraryPage(VideoLibrary *library, QWidget *parent = nullptr);

private slots:
    void refreshList(const QStringList &videos);
    void openSelected();
    void deleteSelected();
    void showContextMenu(const QPoint &pos);

private:
    VideoLibrary *m_library = nullptr;
    QStackedWidget *m_stack = nullptr;
    QListWidget *m_list = nullptr;
    QWidget *m_emptyWidget = nullptr;
};
