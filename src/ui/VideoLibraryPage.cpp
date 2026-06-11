#include "VideoLibraryPage.h"
#include "utils/IconHelper.h"
#include "storage/ISettingsRepository.h"
#include "storage/IVideoLibrary.h"
#include "ffmpeg/FfmpegLocator.h"
#include "utils/win32/FileUtils.h"
#include "io/Format.h"
#include "ffmpeg/ThumbnailGenerator.h"
#include "theme/ThemeColors.h"
#include "utils/win32/Win32Icon.h"
#include "widgets/PageHeader.h"
#include "widgets/SearchLineEdit.h"
#include "widgets/StyledListWidget.h"
#include "widgets/VideoListItemWidget.h"
#include "widgets/ToastWidget.h"
#include "widgets/PaintedDialog.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QFileInfo>
#include <QImage>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPushButton>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace {

class EmptyStateWidget : public QWidget
{
public:
    using QWidget::QWidget;
    void setDarkMode(bool dark) { m_darkMode = dark; update(); }
    void setText(const QString &title, const QString &hint) {
        m_title = title; m_hint = hint; update();
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const auto &a = ThemeColors::forMode(m_darkMode).app;
        const auto &pc = ThemeColors::forMode(m_darkMode).placeholder;

        QPixmap icon = icon::renderSvg(QStringLiteral(":/icons/nav-record.svg"), pc.normal, 48);
        int iconX = (width() - 48) / 2;
        int iconY = height() / 2 - 80;
        p.drawPixmap(iconX, iconY, 48, 48, icon);

        QFont titleFont = p.font();
        titleFont.setPixelSize(22);
        titleFont.setBold(true);
        p.setFont(titleFont);
        p.setPen(a.placeholderTitle);
        p.drawText(QRect(0, iconY + 64, width(), 36), Qt::AlignCenter, m_title);

        QFont hintFont = p.font();
        hintFont.setPixelSize(14);
        hintFont.setBold(false);
        p.setFont(hintFont);
        p.setPen(a.placeholderSubtitle);
        p.drawText(QRect(0, iconY + 104, width(), 28), Qt::AlignCenter, m_hint);
    }
private:
    bool m_darkMode = false;
    QString m_title;
    QString m_hint;
};

}

VideoLibraryPage::VideoLibraryPage(IVideoLibrary *library, ISettingsRepository *settings, QWidget *parent)
    : QWidget(parent)
    , m_library(library)
    , m_settings(settings)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 20, 18, 20);
    root->setSpacing(10);

    auto *header = new PageHeader(QStringLiteral("视频库"), this);
    root->addWidget(header);

    m_filterEdit = new SearchLineEdit(this);
    m_filterEdit->setDarkMode(false);
    root->addWidget(m_filterEdit);

    m_stack = new QStackedWidget(this);

    m_list = new StyledListWidget(this);
    m_list->setDarkMode(false);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    m_stack->addWidget(m_list);

    auto *empty = new EmptyStateWidget(this);
    empty->setText(QStringLiteral("还没有录制文件"),
                   QStringLiteral("点击「开始录制」创建你的第一个屏幕录制吧"));
    empty->setDarkMode(false);
    m_emptyWidget = empty;
    m_stack->addWidget(m_emptyWidget);

    root->addWidget(m_stack, 1);

    m_toastWidget = new ToastWidget(this);

    m_filterDebounce = new QTimer(this);
    m_filterDebounce->setSingleShot(true);
    m_filterDebounce->setInterval(150);
    connect(m_filterDebounce, &QTimer::timeout, this, [this] {
        applyFilter();
    });

    connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString &) {
        m_filterDebounce->start();
    });
    connect(m_list, &QListWidget::currentItemChanged, this, [this] {
        for (int i = 0; i < m_list->count(); ++i) {
            auto *item = m_list->item(i);
            auto *w = qobject_cast<VideoListItemWidget *>(m_list->itemWidget(item));
            if (w) w->setSelected(item == m_list->currentItem());
        }
    });
    connect(m_list, &QListWidget::itemDoubleClicked, this, &VideoLibraryPage::openSelected);
    connect(m_list, &QListWidget::customContextMenuRequested, this, &VideoLibraryPage::showContextMenu);
    connect(m_library, &IVideoLibrary::recentVideosChanged, this, &VideoLibraryPage::refreshList);

    QString savePath = m_settings->value(QStringLiteral("settings/savePath")).toString();
    if (savePath.isEmpty())
        savePath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + QStringLiteral("/BlueCap");
    m_library->scanDirectory(savePath);
    refreshList(m_library->recentVideos());
}

void VideoLibraryPage::refreshList(const QStringList &videos)
{
    m_allVideos = videos;
    m_toastWidget->dismissImmediately();
    m_itemMap.clear();

    for (auto it = m_thumbnailCache.begin(); it != m_thumbnailCache.end(); ) {
        if (!QFileInfo::exists(it.key()))
            it = m_thumbnailCache.erase(it);
        else
            ++it;
    }

    applyFilter();
}

void VideoLibraryPage::applyFilter()
{
    ++m_thumbnailGeneration;
    m_list->clear();
    m_pendingThumbnails.clear();
    m_itemMap.clear();
    QString filter = m_filterEdit->text().trimmed();

    QStringList matched;
    for (const auto &path : m_allVideos) {
        if (filter.isEmpty() || QFileInfo(path).fileName().contains(filter, Qt::CaseInsensitive)) {
            matched.append(path);
        }
    }

    m_filterCount = matched.size();

    if (!filter.isEmpty()) {
        m_filterEdit->setPlaceholderText(
            QStringLiteral("共 %1 个视频").arg(m_filterCount));
    } else {
        m_filterEdit->setPlaceholderText(QStringLiteral("搜索视频..."));
    }

    if (matched.isEmpty()) {
        auto *empty = static_cast<EmptyStateWidget *>(m_emptyWidget);
        if (empty) {
            QFont f = font();
            bool hasFilter = !filter.isEmpty();
            empty->setText(hasFilter
                ? QStringLiteral("没有找到匹配的视频")
                : QStringLiteral("还没有录制文件"),
                hasFilter
                ? QStringLiteral("试试其他关键词")
                : QStringLiteral("点击「开始录制」创建你的第一个屏幕录制吧"));
        }
        m_stack->setCurrentWidget(m_emptyWidget);
        return;
    }
    m_stack->setCurrentWidget(m_list);

    for (const auto &path : matched) {
        QFileInfo fi(path);
        auto *item = new QListWidgetItem(m_list);
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        item->setSizeHint(QSize(0, 92));
        m_itemMap.insert(path, item);

        const QString title = fi.fileName();
        const QString meta = fi.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm"))
            + QStringLiteral("  |  ") + format::fileSize(fi.size());
        const QString badge = fi.suffix().isEmpty() ? QStringLiteral("MP4") : fi.suffix().toUpper();
        const QPixmap cachedThumb = m_thumbnailCache.value(path);

        auto *widget = new VideoListItemWidget(path, title, meta, badge, cachedThumb, m_list);
        widget->setDarkMode(m_darkMode);
        connect(widget, &VideoListItemWidget::openRequested, this, [this](const QString &p) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(p));
        });
        m_list->setItemWidget(item, widget);

        if (m_thumbnailCache.contains(path)) {
            int idx = m_thumbnailLRU.indexOf(path);
            if (idx > 0)
                m_thumbnailLRU.move(idx, 0);
        } else if (!m_pendingThumbnails.contains(path)) {
            m_pendingThumbnails.append(path);
        }
    }

    if (m_list->currentItem()) {
        auto *w = qobject_cast<VideoListItemWidget *>(m_list->itemWidget(m_list->currentItem()));
        if (w) w->setSelected(true);
    }

    if (!m_pendingThumbnails.isEmpty())
        QTimer::singleShot(30, this, &VideoLibraryPage::processNextThumbnail);
}

void VideoLibraryPage::processNextThumbnail()
{
    if (m_pendingThumbnails.isEmpty())
        return;

    QString path = m_pendingThumbnails.takeFirst();
    const int generation = m_thumbnailGeneration;

    auto *watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher, path, generation]() {
        const QImage image = watcher->result();
        if (generation != m_thumbnailGeneration) {
            watcher->deleteLater();
            return;
        }
        if (!image.isNull()) {
            const QPixmap thumb = QPixmap::fromImage(image);
            if (m_thumbnailCache.size() >= kMaxThumbnails) {
                QString oldest = m_thumbnailLRU.takeLast();
                m_thumbnailCache.remove(oldest);
            }
            m_thumbnailCache[path] = thumb;
            m_thumbnailLRU.prepend(path);

            auto *item = m_itemMap.value(path);
            if (item) {
                auto *widget = qobject_cast<VideoListItemWidget *>(m_list->itemWidget(item));
                if (widget)
                    widget->setThumbnail(thumb);
            }
        }
        watcher->deleteLater();
        if (!m_pendingThumbnails.isEmpty())
            QTimer::singleShot(0, this, &VideoLibraryPage::processNextThumbnail);
    });
    const QString ffmpegPath = ffmpeg_locator::findFfmpegPath();
    watcher->setFuture(QtConcurrent::run([ffmpegPath, path]() {
        return thumbnail::fromVideo(ffmpegPath, path);
    }));
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

    if (PaintedDialog::question(this, QStringLiteral("删除确认"),
            QStringLiteral("确定要删除「%1」吗？\n文件将被移至回收站。").arg(fi.fileName()))) {
        m_toastWidget->dismissImmediately();
        m_itemMap.remove(path);

        bool movedToTrash = file_utils::moveToTrash(path);
        if (!movedToTrash) {
            QFile::remove(path);
        }

        m_library->removeVideo(path);

        showToast(movedToTrash
            ? QStringLiteral("已删除「%1」，可从回收站恢复").arg(fi.fileName())
            : QStringLiteral("已永久删除「%1」").arg(fi.fileName()));
    }
}

void VideoLibraryPage::setDarkMode(bool dark)
{
    m_darkMode = dark;
    m_filterEdit->setDarkMode(dark);
    m_list->setDarkMode(dark);
    m_toastWidget->setDarkMode(dark);
    if (auto *empty = static_cast<EmptyStateWidget *>(m_emptyWidget))
        empty->setDarkMode(dark);

    applyFilter();
}

void VideoLibraryPage::showToast(const QString &message)
{
    m_toastWidget->showMessage(message, 5000);
}

void VideoLibraryPage::renameSelected()
{
    auto *item = m_list->currentItem();
    if (!item) return;

    const QString oldPath = item->data(Qt::UserRole).toString();
    QFileInfo fi(oldPath);
    const QString oldName = fi.fileName();

    bool ok = false;
    QString newName = PaintedDialog::getText(this, QStringLiteral("重命名"),
        QStringLiteral("新文件名："), oldName, &ok);
    if (!ok || newName.isEmpty() || newName == oldName)
        return;

    if (!file_utils::isValidFileName(newName)) {
        PaintedDialog::warning(this, QStringLiteral("重命名失败"),
            QStringLiteral("文件名包含非法字符。"));
        return;
    }

    if (!newName.contains(QLatin1Char('.'))) {
        newName += fi.suffix().isEmpty() ? QStringLiteral(".mp4") : QStringLiteral(".") + fi.suffix();
    }

    const QString newPath = fi.dir().filePath(newName);
    if (QFileInfo::exists(newPath)) {
        PaintedDialog::warning(this, QStringLiteral("重命名失败"),
            QStringLiteral("目标文件名已存在。"));
        return;
    }

    if (!QFile::rename(oldPath, newPath)) {
        PaintedDialog::warning(this, QStringLiteral("重命名失败"),
            QStringLiteral("无法重命名文件，请检查文件是否被占用。"));
        return;
    }

    m_thumbnailCache.remove(oldPath);
    m_itemMap.remove(oldPath);

    QStringList videos = m_library->recentVideos();
    int idx = videos.indexOf(oldPath);
    if (idx >= 0) {
        videos[idx] = newPath;
        m_library->clearAndReplace(videos);
    }

    showToast(QStringLiteral("已重命名为「%1」").arg(newName));
}

void VideoLibraryPage::showContextMenu(const QPoint &pos)
{
    auto *item = m_list->itemAt(pos);
    if (!item) return;

    const auto &a = ThemeColors::forMode(m_darkMode).app;
    QMenu menu(this);
    QPalette mp = menu.palette();
    mp.setColor(QPalette::Window, a.menuBg);
    mp.setColor(QPalette::WindowText, a.menuItemText);
    mp.setColor(QPalette::Highlight, a.menuItemSelectedBg);
    mp.setColor(QPalette::HighlightedText, a.menuItemSelectedText);
    menu.setPalette(mp);
    menu.setStyleSheet(QStringLiteral(
        "QMenu { border: 1px solid %1; border-radius: 8px; padding: 5px; }"
        "QMenu::separator { height: 1px; background: %2; margin: 4px 8px; }"
        "QMenu::item { padding: 7px 24px; border-radius: 6px; }"
        "QMenu::item:selected { background: %3; color: %4; }"
    ).arg(a.menuBorder.name(QColor::HexArgb))
     .arg(a.menuSeparator.name(QColor::HexArgb))
     .arg(a.menuItemSelectedBg.name(QColor::HexArgb))
     .arg(a.menuItemSelectedText.name(QColor::HexArgb)));

    menu.addAction(QStringLiteral("打开"), this, &VideoLibraryPage::openSelected);
    menu.addAction(QStringLiteral("打开文件位置"), this, [this, item]() {
        const QString path = item->data(Qt::UserRole).toString();
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).path()));
    });
    menu.addSeparator();
    menu.addAction(QStringLiteral("重命名"), this, &VideoLibraryPage::renameSelected);
    menu.addAction(QStringLiteral("删除"), this, &VideoLibraryPage::deleteSelected);
    menu.exec(m_list->mapToGlobal(pos));
}
