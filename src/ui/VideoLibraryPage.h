#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
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
    void applyFilter();

    VideoLibrary *m_library = nullptr;
    QStackedWidget *m_stack = nullptr;
    QListWidget *m_list = nullptr;
    QWidget *m_emptyWidget = nullptr;
    QLineEdit *m_filterEdit = nullptr;
    QStringList m_allVideos;
};
