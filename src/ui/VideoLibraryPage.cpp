#include "VideoLibraryPage.h"
#include "../storage/VideoLibrary.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QUrl>
#include <QVBoxLayout>

VideoLibraryPage::VideoLibraryPage(VideoLibrary *library, QWidget *parent)
    : QWidget(parent)
    , m_library(library)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 12);
    root->setSpacing(8);

    auto *header = new QLabel(QStringLiteral("视频库"));
    header->setObjectName(QStringLiteral("pageHeader"));
    root->addWidget(header);

    m_list = new QListWidget(this);
    m_list->setAlternatingRowColors(true);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    root->addWidget(m_list, 1);

    connect(m_list, &QListWidget::itemDoubleClicked, this, &VideoLibraryPage::openSelected);
    connect(m_list, &QListWidget::customContextMenuRequested, this, &VideoLibraryPage::showContextMenu);
    connect(m_library, &VideoLibrary::recentVideosChanged, this, &VideoLibraryPage::refreshList);

    refreshList(m_library->recentVideos());
}

void VideoLibraryPage::refreshList(const QStringList &videos)
{
    m_list->clear();
    for (const auto &path : videos) {
        QFileInfo fi(path);
        auto *item = new QListWidgetItem(fi.fileName(), m_list);
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
    }
}

void VideoLibraryPage::openSelected()
{
    auto *item = m_list->currentItem();
    if (!item) return;

    const QString path = item->data(Qt::UserRole).toString();
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void VideoLibraryPage::deleteSelected()
{
    auto *item = m_list->currentItem();
    if (!item) return;

    const QString path = item->data(Qt::UserRole).toString();
    QFileInfo fi(path);

    auto result = QMessageBox::question(this, QStringLiteral("删除确认"),
        QStringLiteral("确定要删除「%1」吗？").arg(fi.fileName()),
        QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        QFile::remove(path);

        QStringList videos = m_library->recentVideos();
        videos.removeAll(path);
        for (int i = 0; i < videos.size();) {
            if (!QFileInfo::exists(videos[i])) {
                videos.removeAt(i);
            } else {
                ++i;
            }
        }
        // Re-add through library API
        m_library->clearAndReplace(videos);
    }
}

void VideoLibraryPage::showContextMenu(const QPoint &pos)
{
    auto *item = m_list->itemAt(pos);
    if (!item) return;

    QMenu menu(this);
    menu.addAction(QStringLiteral("打开"), this, &VideoLibraryPage::openSelected);
    menu.addAction(QStringLiteral("打开文件位置"), this, [this, item]() {
        const QString path = item->data(Qt::UserRole).toString();
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).path()));
    });
    menu.addSeparator();
    menu.addAction(QStringLiteral("删除"), this, &VideoLibraryPage::deleteSelected);
    menu.exec(m_list->mapToGlobal(pos));
}
