#include "VideoLibraryPage.h"
#include "../storage/VideoLibrary.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
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
#include <QPushButton>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include <windows.h>
#include <shellapi.h>
#include <shobjidl.h>

namespace {

bool moveToTrash(const QString &filePath)
{
    const QString nullPath = filePath + QChar('\0');
    std::wstring wPath = nullPath.toStdWString();

    SHFILEOPSTRUCTW fos = {};
    fos.wFunc = FO_DELETE;
    fos.pFrom = wPath.c_str();
    fos.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    return SHFileOperationW(&fos) == 0;
}

}

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

    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(QStringLiteral("搜索视频..."));
    m_filterEdit->setClearButtonEnabled(true);
    root->addWidget(m_filterEdit);

    m_stack = new QStackedWidget(this);

    m_list = new QListWidget(this);
    m_list->setAlternatingRowColors(true);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    m_stack->addWidget(m_list);

    m_emptyWidget = new QWidget(this);
    auto *emptyLayout = new QVBoxLayout(m_emptyWidget);
    emptyLayout->setSpacing(16);
    auto *emptyIcon = new QLabel(m_emptyWidget);
    emptyIcon->setPixmap(QIcon(QStringLiteral(":/icons/nav-record.svg")).pixmap(48, 48));
    emptyIcon->setAlignment(Qt::AlignCenter);
    auto *emptyTitle = new QLabel(QStringLiteral("还没有录制文件"), m_emptyWidget);
    emptyTitle->setObjectName(QStringLiteral("placeholderTitle"));
    emptyTitle->setAlignment(Qt::AlignCenter);
    auto *emptyHint = new QLabel(QStringLiteral("点击「开始录制」创建你的第一个屏幕录制吧"), m_emptyWidget);
    emptyHint->setObjectName(QStringLiteral("placeholderSubtitle"));
    emptyHint->setAlignment(Qt::AlignCenter);
    emptyLayout->addStretch();
    emptyLayout->addWidget(emptyIcon);
    emptyLayout->addWidget(emptyTitle);
    emptyLayout->addWidget(emptyHint);
    emptyLayout->addStretch();
    m_stack->addWidget(m_emptyWidget);

    root->addWidget(m_stack, 1);

    m_toastWidget = new QWidget(this);
    m_toastWidget->setVisible(false);
    m_toastWidget->setStyleSheet(QStringLiteral(
        "background: #1a2638; border-radius: 8px; padding: 0px;"));
    auto *toastLayout = new QHBoxLayout(m_toastWidget);
    toastLayout->setContentsMargins(16, 10, 16, 10);
    m_toastLabel = new QLabel(m_toastWidget);
    m_toastLabel->setStyleSheet(QStringLiteral(
        "color: #ffffff; font-size: 13px; font-weight: 600; background: transparent;"));
    m_toastUndoBtn = new QPushButton(QStringLiteral("撤销"), m_toastWidget);
    m_toastUndoBtn->setStyleSheet(QStringLiteral(
        "QPushButton { color: #78bdff; font-size: 13px; font-weight: 700; "
        "background: transparent; border: 1px solid #78bdff; border-radius: 4px; "
        "padding: 4px 12px; } "
        "QPushButton:hover { background: rgba(120, 189, 255, 0.15); }"));
    m_toastUndoBtn->setCursor(Qt::PointingHandCursor);
    toastLayout->addWidget(m_toastLabel, 1);
    toastLayout->addWidget(m_toastUndoBtn);
    root->addWidget(m_toastWidget);

    m_toastTimer = new QTimer(this);
    m_toastTimer->setSingleShot(true);
    connect(m_toastTimer, &QTimer::timeout, this, &VideoLibraryPage::cleanupUndo);
    connect(m_toastTimer, &QTimer::timeout, m_toastWidget, &QWidget::hide);
    connect(m_toastUndoBtn, &QPushButton::clicked, this, &VideoLibraryPage::undoDelete);

    connect(m_filterEdit, &QLineEdit::textChanged, this, &VideoLibraryPage::applyFilter);
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

    m_list->setIconSize(QSize(120, 68));

    for (const auto &path : matched) {
        QFileInfo fi(path);
        qint64 size = fi.size();
        QString sizeStr;
        if (size < 1024 * 1024)
            sizeStr = QStringLiteral("%1 KB").arg(size / 1024);
        else
            sizeStr = QStringLiteral("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);

        QString text = fi.fileName() + QStringLiteral("\n")
            + fi.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm"))
            + QStringLiteral("  |  ") + sizeStr;

        QPixmap thumb = getVideoThumbnail(path);
        QIcon icon;
        if (!thumb.isNull())
            icon = QIcon(thumb);
        auto *item = new QListWidgetItem(icon, text, m_list);
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        item->setSizeHint(QSize(0, 80));
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
        QStringLiteral("确定要删除「%1」吗？\n文件将被移至回收站。").arg(fi.fileName()),
        QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        cleanupUndo();

        if (fi.exists()) {
            QString undoDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                + QStringLiteral("/BlueCap/undo");
            QDir().mkpath(undoDir);
            m_undoBackupPath = undoDir + QStringLiteral("/") + fi.fileName();
            QFile::copy(path, m_undoBackupPath);
            m_undoOriginalPath = path;
        }

        if (!moveToTrash(path)) {
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

        showToast(QStringLiteral("已删除「%1」").arg(fi.fileName()));
    }
}

void VideoLibraryPage::showToast(const QString &message)
{
    m_toastLabel->setText(message);
    m_toastWidget->setVisible(true);
    m_toastTimer->start(5000);
}

void VideoLibraryPage::undoDelete()
{
    if (m_undoBackupPath.isEmpty() || m_undoOriginalPath.isEmpty())
        return;

    QFileInfo backup(m_undoBackupPath);
    if (!backup.exists()) {
        QMessageBox::information(this, QStringLiteral("撤销失败"),
            QStringLiteral("备份文件已不存在，无法恢复。"));
        cleanupUndo();
        return;
    }

    if (QFileInfo::exists(m_undoOriginalPath)) {
        QMessageBox::information(this, QStringLiteral("撤销失败"),
            QStringLiteral("原文件已存在，无法恢复。"));
        cleanupUndo();
        return;
    }

    QDir().mkpath(QFileInfo(m_undoOriginalPath).path());
    if (QFile::copy(m_undoBackupPath, m_undoOriginalPath)) {
        m_library->addRecentVideo(m_undoOriginalPath);
        m_toastLabel->setText(QStringLiteral("已撤销删除"));
        m_toastUndoBtn->setVisible(false);
        m_toastTimer->start(2000);
    } else {
        QMessageBox::warning(this, QStringLiteral("撤销失败"),
            QStringLiteral("无法恢复文件，请检查磁盘空间或权限。"));
    }
    cleanupUndo();
}

void VideoLibraryPage::cleanupUndo()
{
    if (!m_undoBackupPath.isEmpty()) {
        QFile::remove(m_undoBackupPath);
        m_undoBackupPath.clear();
    }
    m_undoOriginalPath.clear();
    m_toastUndoBtn->setVisible(true);
}

static QPixmap hBitmapToPixmap(HBITMAP hBitmap)
{
    BITMAP bm;
    if (!GetObject(hBitmap, sizeof(bm), &bm) || !bm.bmWidth || !bm.bmHeight)
        return {};

    QImage img(bm.bmWidth, bm.bmHeight, QImage::Format_ARGB32_Premultiplied);
    if (img.isNull()) return {};

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = bm.bmWidth;
    bi.bmiHeader.biHeight = -bm.bmHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(nullptr);
    int lines = GetDIBits(hdc, hBitmap, 0, bm.bmHeight, img.bits(), &bi, DIB_RGB_COLORS);
    ReleaseDC(nullptr, hdc);

    return lines > 0 ? QPixmap::fromImage(img) : QPixmap();
}

QPixmap VideoLibraryPage::getVideoThumbnail(const QString &filePath)
{
    if (m_thumbnailCache.contains(filePath))
        return m_thumbnailCache[filePath];

    QPixmap fallback;
    IShellItemImageFactory *factory = nullptr;
    HRESULT hr = SHCreateItemFromParsingName(
        reinterpret_cast<const wchar_t *>(filePath.utf16()),
        nullptr,
        IID_PPV_ARGS(&factory));

    if (SUCCEEDED(hr) && factory) {
        HBITMAP hBitmap = nullptr;
        hr = factory->GetImage({ 320, 180 }, 0, &hBitmap);
        if (SUCCEEDED(hr) && hBitmap) {
            fallback = hBitmapToPixmap(hBitmap);
            DeleteObject(hBitmap);
        }
        factory->Release();
    }

    if (!fallback.isNull()) {
        m_thumbnailCache[filePath] = fallback;
    }
    return fallback;
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
