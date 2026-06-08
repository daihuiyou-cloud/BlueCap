#pragma once

#include <QPixmap>
#include <QWidget>
#include <QMap>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QStackedWidget;
class QTimer;
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
    void renameSelected();
    void showContextMenu(const QPoint &pos);

private:
    void applyFilter();
    void showToast(const QString &message);
    QPixmap getVideoThumbnail(const QString &filePath);

    VideoLibrary *m_library = nullptr;
    QStackedWidget *m_stack = nullptr;
    QListWidget *m_list = nullptr;
    QWidget *m_emptyWidget = nullptr;
    QLineEdit *m_filterEdit = nullptr;
    QStringList m_allVideos;
    QWidget *m_toastWidget = nullptr;
    QLabel *m_toastLabel = nullptr;
    QTimer *m_toastTimer = nullptr;
    QMap<QString, QPixmap> m_thumbnailCache;
    QStringList m_thumbnailLRU;
    static constexpr int kMaxThumbnails = 50;
};
