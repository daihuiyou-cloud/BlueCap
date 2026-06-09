#pragma once

#include <QHash>
#include <QIcon>
#include <QMap>
#include <QPixmap>
#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QStackedWidget;
class QTimer;
class VideoLibrary;

class VideoLibraryPage : public QWidget
{
    Q_OBJECT

public:
    explicit VideoLibraryPage(VideoLibrary *library, QWidget *parent = nullptr);
    void setDarkMode(bool dark);

private slots:
    void refreshList(const QStringList &videos);
    void openSelected();
    void deleteSelected();
    void renameSelected();
    void showContextMenu(const QPoint &pos);

private:
    void applyFilter();
    void showToast(const QString &message);
    void processNextThumbnail();
    QWidget *createVideoItemWidget(const QString &path, const QPixmap &thumbnail);
    void updateItemSelectionStyles();

    VideoLibrary *m_library = nullptr;
    QStackedWidget *m_stack = nullptr;
    QListWidget *m_list = nullptr;
    QWidget *m_emptyWidget = nullptr;
    QLineEdit *m_filterEdit = nullptr;
    QLabel *m_emptyIcon = nullptr;
    bool m_darkMode = false;
    QStringList m_allVideos;
    QWidget *m_toastWidget = nullptr;
    QLabel *m_toastLabel = nullptr;
    QTimer *m_toastTimer = nullptr;
    QTimer *m_filterDebounce = nullptr;
    QMap<QString, QPixmap> m_thumbnailCache;
    QStringList m_thumbnailLRU;
    QStringList m_pendingThumbnails;
    QHash<QString, QListWidgetItem *> m_itemMap;
    QHash<QString, QLabel *> m_thumbnailLabels;
    QIcon m_placeholderIcon;
    int m_thumbnailGeneration = 0;
    static constexpr int kMaxThumbnails = 50;
};
