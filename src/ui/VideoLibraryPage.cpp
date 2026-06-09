#include "VideoLibraryPage.h"
#include "IconHelper.h"
#include "storage/VideoLibrary.h"
#include "utils/Format.h"
#include "utils/ThemeColors.h"
#include "utils/Win32Icon.h"

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QImage>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include <windows.h>
#include <shellapi.h>
#include <shobjidl.h>

namespace {

QString findBundledFfmpeg()
{
    const QString bundlePath = QCoreApplication::applicationDirPath()
        + QStringLiteral("/3rd/ffmpeg/ffmpeg.exe");
    if (QFileInfo::exists(bundlePath))
        return bundlePath;
    return QStringLiteral("ffmpeg.exe");
}

QPixmap thumbnailViaFfmpeg(const QString &filePath)
{
    const QString ffmpeg = findBundledFfmpeg();
    if (!QFileInfo::exists(ffmpeg))
        return {};

    const QString tmpFile = QDir::temp().absoluteFilePath(
        QStringLiteral("bluecap_thumb_%1.bmp").arg(
            QDateTime::currentMSecsSinceEpoch()));

    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(ffmpeg, {
        QStringLiteral("-ss"), QStringLiteral("00:00:01"),
        QStringLiteral("-i"), filePath,
        QStringLiteral("-vframes"), QStringLiteral("1"),
        QStringLiteral("-f"), QStringLiteral("image2pipe"),
        QStringLiteral("-vcodec"), QStringLiteral("bmp"),
        QStringLiteral("-")
    });
    if (!proc.waitForFinished(15000) || proc.exitCode() != 0)
        return {};

    QPixmap px;
    px.loadFromData(proc.readAllStandardOutput(), "BMP");
    return px;
}

QPixmap fetchThumbnailRaw(const QString &filePath)
{
    QPixmap result;
    IShellItemImageFactory *factory = nullptr;
    HRESULT hr = SHCreateItemFromParsingName(
        reinterpret_cast<const wchar_t *>(filePath.utf16()),
        nullptr,
        IID_PPV_ARGS(&factory));

    if (SUCCEEDED(hr) && factory) {
        HBITMAP hBitmap = nullptr;
        hr = factory->GetImage({ 320, 180 }, 0, &hBitmap);
        if (SUCCEEDED(hr) && hBitmap) {
            result = win32::hBitmapToPixmap(hBitmap);
            DeleteObject(hBitmap);
        }
        factory->Release();
    }

    if (result.isNull())
        result = thumbnailViaFfmpeg(filePath);

    return result;
}

bool moveToTrash(const QString &filePath)
{
    // Double-null-terminated wide string for SHFileOperationW
    auto *wPath = reinterpret_cast<const wchar_t *>(filePath.utf16());
    // The QString is already null-terminated; we need double null
    std::wstring doubleNull(wPath, filePath.size() + 1);

    SHFILEOPSTRUCTW fos = {};
    fos.wFunc = FO_DELETE;
    fos.pFrom = doubleNull.c_str();
    fos.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    return SHFileOperationW(&fos) == 0;
}

}

VideoLibraryPage::VideoLibraryPage(VideoLibrary *library, QWidget *parent)
    : QWidget(parent)
    , m_library(library)
{
    setObjectName(QStringLiteral("videoLibraryPage"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 16, 18, 14);
    root->setSpacing(10);

    auto *header = new QLabel(QStringLiteral("视频库"));
    header->setObjectName(QStringLiteral("pageHeader"));
    root->addWidget(header);

    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setObjectName(QStringLiteral("videoLibrarySearch"));
    m_filterEdit->setPlaceholderText(QStringLiteral("搜索视频..."));
    m_filterEdit->setClearButtonEnabled(true);
    m_filterEdit->setFixedHeight(40);
    root->addWidget(m_filterEdit);

    m_stack = new QStackedWidget(this);
    m_stack->setObjectName(QStringLiteral("videoLibraryStack"));

    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("videoLibraryList"));
    m_list->setAlternatingRowColors(false);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setSpacing(6);
    m_stack->addWidget(m_list);

    m_emptyWidget = new QWidget(this);
    auto *emptyLayout = new QVBoxLayout(m_emptyWidget);
    emptyLayout->setSpacing(16);
    m_emptyIcon = new QLabel(m_emptyWidget);
    m_emptyIcon->setPixmap(icon::renderSvg(
        QStringLiteral(":/icons/nav-record.svg"), ThemeColors::forMode(false).placeholder.normal, 48));
    m_emptyIcon->setAlignment(Qt::AlignCenter);
    auto *emptyTitle = new QLabel(QStringLiteral("还没有录制文件"), m_emptyWidget);
    emptyTitle->setObjectName(QStringLiteral("placeholderTitle"));
    emptyTitle->setAlignment(Qt::AlignCenter);
    auto *emptyHint = new QLabel(QStringLiteral("点击「开始录制」创建你的第一个屏幕录制吧"), m_emptyWidget);
    emptyHint->setObjectName(QStringLiteral("placeholderSubtitle"));
    emptyHint->setAlignment(Qt::AlignCenter);
    emptyLayout->addStretch();
    emptyLayout->addWidget(m_emptyIcon);
    emptyLayout->addWidget(emptyTitle);
    emptyLayout->addWidget(emptyHint);
    emptyLayout->addStretch();
    m_stack->addWidget(m_emptyWidget);

    root->addWidget(m_stack, 1);

    m_toastWidget = new QWidget(this);
    m_toastWidget->setVisible(false);
    m_toastWidget->setObjectName(QStringLiteral("toastWidget"));
    auto *toastLayout = new QHBoxLayout(m_toastWidget);
    toastLayout->setContentsMargins(16, 10, 16, 10);
    m_toastLabel = new QLabel(m_toastWidget);
    m_toastLabel->setObjectName(QStringLiteral("toastLabel"));
    toastLayout->addWidget(m_toastLabel, 1);
    root->addWidget(m_toastWidget);

    m_toastTimer = new QTimer(this);
    m_toastTimer->setSingleShot(true);
    connect(m_toastTimer, &QTimer::timeout, m_toastWidget, &QWidget::hide);

    m_filterDebounce = new QTimer(this);
    m_filterDebounce->setSingleShot(true);
    m_filterDebounce->setInterval(150);
    connect(m_filterDebounce, &QTimer::timeout, this, [this] {
        applyFilter();
    });

    const auto &pc = ThemeColors::forMode(false).placeholder;
    m_placeholderIcon = icon::coloredIcon(QStringLiteral(":/icons/nav-record.svg"), 24,
        pc.normal, pc.active, pc.disabled);

    connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString &) {
        m_filterDebounce->start();
    });
    connect(m_list, &QListWidget::itemDoubleClicked, this, &VideoLibraryPage::openSelected);
    connect(m_list, &QListWidget::customContextMenuRequested, this, &VideoLibraryPage::showContextMenu);
    connect(m_library, &VideoLibrary::recentVideosChanged, this, &VideoLibraryPage::refreshList);

    refreshList(m_library->recentVideos());
}

void VideoLibraryPage::refreshList(const QStringList &videos)
{
    m_allVideos = videos;
    m_toastWidget->setVisible(false);
    m_toastTimer->stop();
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
    m_list->clear();
    m_pendingThumbnails.clear();
    QString filter = m_filterEdit->text().trimmed();

    QStringList matched;
    for (const auto &path : m_allVideos) {
        if (filter.isEmpty() || QFileInfo(path).fileName().contains(filter, Qt::CaseInsensitive)) {
            matched.append(path);
        }
    }

    if (matched.isEmpty()) {
        m_stack->setCurrentWidget(m_emptyWidget);
        return;
    }
    m_stack->setCurrentWidget(m_list);

    m_list->setIconSize(QSize(104, 58));

    for (const auto &path : matched) {
        QFileInfo fi(path);

        QString text = fi.fileName() + QStringLiteral("\n")
            + fi.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm"))
            + QStringLiteral("  |  ") + format::fileSize(fi.size());

        auto *item = new QListWidgetItem(m_placeholderIcon, text, m_list);
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        item->setSizeHint(QSize(0, 82));
        m_itemMap.insert(path, item);

        // Cache hit — set icon immediately
        if (m_thumbnailCache.contains(path)) {
            item->setIcon(QIcon(m_thumbnailCache[path]));
        } else {
            m_pendingThumbnails.append(path);
        }
    }

    if (!m_pendingThumbnails.isEmpty())
        QTimer::singleShot(30, this, &VideoLibraryPage::processNextThumbnail);
}

void VideoLibraryPage::processNextThumbnail()
{
    if (m_pendingThumbnails.isEmpty())
        return;

    QString path = m_pendingThumbnails.takeFirst();

    auto *watcher = new QFutureWatcher<QPixmap>(this);
    connect(watcher, &QFutureWatcher<QPixmap>::finished, this, [this, watcher, path]() {
        QPixmap thumb = watcher->result();
        if (!thumb.isNull()) {
            if (m_thumbnailCache.size() >= kMaxThumbnails) {
                QString oldest = m_thumbnailLRU.takeLast();
                m_thumbnailCache.remove(oldest);
            }
            m_thumbnailCache[path] = thumb;
            m_thumbnailLRU.prepend(path);

            auto *item = m_itemMap.value(path);
            if (item)
                item->setIcon(QIcon(thumb));
        }
        watcher->deleteLater();
        if (!m_pendingThumbnails.isEmpty())
            QTimer::singleShot(0, this, &VideoLibraryPage::processNextThumbnail);
    });
    watcher->setFuture(QtConcurrent::run(fetchThumbnailRaw, path));
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
        QStringLiteral("确定要删除「%1」吗？\n文件将被移至回收站。").arg(fi.fileName()),
        QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_toastWidget->setVisible(false);
        m_toastTimer->stop();
        m_itemMap.remove(path);

        bool movedToTrash = moveToTrash(path);
        if (!movedToTrash) {
            QFile::remove(path);
        }

        QStringList videos = m_library->recentVideos();
        videos.removeAll(path);
        for (int i = 0; i < videos.size();) {
            if (!QFileInfo::exists(videos[i])) {
                videos.removeAt(i);
            } else {
                ++i;
            }
        }
        m_library->clearAndReplace(videos);

        showToast(movedToTrash
            ? QStringLiteral("已删除「%1」，可从回收站恢复").arg(fi.fileName())
            : QStringLiteral("已永久删除「%1」").arg(fi.fileName()));
    }
}

void VideoLibraryPage::setDarkMode(bool dark)
{
    m_darkMode = dark;
    const auto &pc = ThemeColors::forMode(dark).placeholder;
    m_emptyIcon->setPixmap(icon::renderSvg(
        QStringLiteral(":/icons/nav-record.svg"), pc.normal, 48));
    m_placeholderIcon = icon::coloredIcon(QStringLiteral(":/icons/nav-record.svg"), 24,
        pc.normal, pc.active, pc.disabled);
    applyFilter();
}

void VideoLibraryPage::showToast(const QString &message)
{
    m_toastLabel->setText(message);
    m_toastWidget->setVisible(true);
    m_toastTimer->start(5000);
}

void VideoLibraryPage::renameSelected()
{
    auto *item = m_list->currentItem();
    if (!item) return;

    const QString oldPath = item->data(Qt::UserRole).toString();
    QFileInfo fi(oldPath);
    const QString oldName = fi.fileName();

    bool ok = false;
    QString newName = QInputDialog::getText(this, QStringLiteral("重命名"),
        QStringLiteral("新文件名："), QLineEdit::Normal, oldName, &ok);
    if (!ok || newName.isEmpty() || newName == oldName)
        return;

    if (newName.contains(QLatin1Char('/')) || newName.contains(QLatin1Char('\\'))
        || newName.contains(QLatin1Char(':')) || newName.contains(QLatin1Char('*'))
        || newName.contains(QLatin1Char('?')) || newName.contains(QLatin1Char('"'))
        || newName.contains(QLatin1Char('<')) || newName.contains(QLatin1Char('>'))
        || newName.contains(QLatin1Char('|'))) {
        QMessageBox::warning(this, QStringLiteral("重命名失败"),
            QStringLiteral("文件名包含非法字符。"));
        return;
    }

    if (!newName.contains(QLatin1Char('.'))) {
        newName += fi.suffix().isEmpty() ? QStringLiteral(".mp4") : QStringLiteral(".") + fi.suffix();
    }

    const QString newPath = fi.dir().filePath(newName);
    if (QFileInfo::exists(newPath)) {
        QMessageBox::warning(this, QStringLiteral("重命名失败"),
            QStringLiteral("目标文件名已存在。"));
        return;
    }

    if (!QFile::rename(oldPath, newPath)) {
        QMessageBox::warning(this, QStringLiteral("重命名失败"),
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

    QMenu menu(this);
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
