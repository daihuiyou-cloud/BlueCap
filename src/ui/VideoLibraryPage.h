#pragma once

#include <QHash>
#include <QMap>
#include <QPixmap>
#include <QWidget>

class ISettingsRepository;
class IVideoLibrary;
class QListWidgetItem;
class QPushButton;
class QStackedWidget;
class QTimer;
class SearchLineEdit;
class StyledListWidget;
class ToastWidget;

class VideoLibraryPage : public QWidget
{
    Q_OBJECT

public:
    explicit VideoLibraryPage(IVideoLibrary *library, ISettingsRepository *settings, QWidget *parent = nullptr);
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

    IVideoLibrary *m_library = nullptr;
    ISettingsRepository *m_settings = nullptr;
    QStackedWidget *m_stack = nullptr;
    StyledListWidget *m_list = nullptr;
    QWidget *m_emptyWidget = nullptr;
    SearchLineEdit *m_filterEdit = nullptr;
    bool m_darkMode = false;
    int m_filterCount = 0;
    QStringList m_allVideos;
    ToastWidget *m_toastWidget = nullptr;
    QTimer *m_filterDebounce = nullptr;
    QMap<QString, QPixmap> m_thumbnailCache;
    QStringList m_thumbnailLRU;
    QStringList m_pendingThumbnails;
    QHash<QString, QListWidgetItem *> m_itemMap;
    int m_thumbnailGeneration = 0;
    static constexpr int kMaxThumbnails = 50;
};
