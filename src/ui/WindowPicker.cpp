#include "WindowPicker.h"
#include "recorder/RecorderController.h"

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QPainter>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QShortcut>
#include <QVBoxLayout>

#include <windows.h>

WindowPicker::WindowPicker(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(476, 376);
    resize(530, 470);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);

    m_surface = new QWidget(this);
    m_surface->setObjectName(QStringLiteral("dialogSurface"));
    m_surface->setAttribute(Qt::WA_StyledBackground, true);

    auto *surfaceLayout = new QVBoxLayout(m_surface);
    surfaceLayout->setSpacing(12);
    surfaceLayout->setContentsMargins(20, 0, 20, 16);

    m_titleBar = new QWidget(m_surface);
    m_titleBar->setObjectName(QStringLiteral("dialogTitleBar"));
    m_titleBar->setFixedHeight(48);
    auto *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);

    auto *header = new QLabel(QStringLiteral("选择窗口"), m_titleBar);
    header->setObjectName(QStringLiteral("pageHeader"));

    m_closeBtn = new QPushButton(QStringLiteral("✕"), m_titleBar);
    m_closeBtn->setObjectName(QStringLiteral("closeButton"));
    m_closeBtn->setFixedSize(30, 30);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setFlat(true);

    titleLayout->addWidget(header);
    titleLayout->addStretch();
    titleLayout->addWidget(m_closeBtn);
    surfaceLayout->addWidget(m_titleBar);

    auto *searchRow = new QHBoxLayout;
    searchRow->setSpacing(8);
    m_filterEdit = new QLineEdit(m_surface);
    m_filterEdit->setPlaceholderText(QStringLiteral("搜索窗口..."));
    searchRow->addWidget(m_filterEdit, 1);

    m_refreshBtn = new QPushButton(QStringLiteral("刷新"), m_surface);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    m_refreshBtn->setToolTip(QStringLiteral("重新扫描当前打开的窗口"));
    searchRow->addWidget(m_refreshBtn);
    surfaceLayout->addLayout(searchRow);

    m_list = new QListWidget(m_surface);
    m_list->setIconSize(QSize(24, 24));
    surfaceLayout->addWidget(m_list, 1);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, m_surface);
    surfaceLayout->addWidget(buttonBox);

    root->addWidget(m_surface);

    auto *shadow = new QGraphicsDropShadowEffect(m_surface);
    shadow->setBlurRadius(20);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 50));
    m_surface->setGraphicsEffect(shadow);

    m_windows = RecorderController::enumerateWindows();
    populateList();

    m_filterEdit->setFocus();
    if (m_list->count() > 0) {
        m_list->setCurrentRow(0);
    }

    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    auto *filterShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this);
    connect(filterShortcut, &QShortcut::activated, m_filterEdit, qOverload<>(&QWidget::setFocus));

    connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        populateList(text);
    });

    connect(m_refreshBtn, &QPushButton::clicked, this, &WindowPicker::refreshWindows);

    connect(m_list, &QListWidget::itemDoubleClicked, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void WindowPicker::refreshWindows()
{
    m_windows = RecorderController::enumerateWindows();
    m_iconCache.clear();
    populateList(m_filterEdit->text());
    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
    m_filterEdit->setFocus();
}

void WindowPicker::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Clip away shadow margin area for the background
    const QRectF body = rect().adjusted(14, 14, -14, -14);
    painter.setClipRect(body);
    painter.fillRect(body, Qt::transparent);
}

void WindowPicker::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPoint pos = m_surface->mapFrom(this, event->pos());
        if (m_titleBar->geometry().contains(pos)) {
            m_dragging = true;
            m_dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
            return;
        }
    }
    QDialog::mousePressEvent(event);
}

void WindowPicker::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void WindowPicker::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    QDialog::mouseReleaseEvent(event);
}

void WindowPicker::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

namespace {

QPixmap iconFromHICON(HICON hIcon)
{
    if (!hIcon) return {};
    ICONINFO ii = {};
    if (!GetIconInfo(hIcon, &ii))
        return {};
    BITMAP bm = {};
    if (!GetObject(ii.hbmColor, sizeof(bm), &bm)) {
        DeleteObject(ii.hbmColor);
        DeleteObject(ii.hbmMask);
        return {};
    }
    QImage img(bm.bmWidth, bm.bmHeight, QImage::Format_ARGB32_Premultiplied);
    if (img.isNull()) {
        DeleteObject(ii.hbmColor);
        DeleteObject(ii.hbmMask);
        return {};
    }
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = bm.bmWidth;
    bi.bmiHeader.biHeight = -bm.bmHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    HDC hdc = GetDC(nullptr);
    int lines = GetDIBits(hdc, ii.hbmColor, 0, bm.bmHeight, img.bits(), &bi, DIB_RGB_COLORS);
    ReleaseDC(nullptr, hdc);
    DeleteObject(ii.hbmColor);
    DeleteObject(ii.hbmMask);
    return lines > 0 ? QPixmap::fromImage(img) : QPixmap();
}

}

void WindowPicker::populateList(const QString &filter)
{
    m_list->clear();
    for (const auto &entry : m_windows) {
        const QString &display = entry.displayName;
        if (!filter.isEmpty() && !display.contains(filter, Qt::CaseInsensitive))
            continue;

        auto *item = new QListWidgetItem(display, m_list);
        item->setData(Qt::UserRole, entry.title);
        item->setToolTip(display);

        auto iconIt = m_iconCache.constFind(entry.hwnd);
        if (iconIt != m_iconCache.constEnd()) {
            item->setIcon(QIcon(iconIt.value()));
        } else {
            HWND hwnd = reinterpret_cast<HWND>(entry.hwnd);
            HICON hIcon = reinterpret_cast<HICON>(
                SendMessageW(hwnd, WM_GETICON, ICON_SMALL2, 0));
            if (!hIcon)
                hIcon = reinterpret_cast<HICON>(
                    GetClassLongPtrW(hwnd, GCLP_HICONSM));
            QPixmap px = iconFromHICON(hIcon);
            if (!px.isNull()) {
                m_iconCache.insert(entry.hwnd, px);
                item->setIcon(QIcon(px));
            }
        }
    }
    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
}

QString WindowPicker::selectedWindow() const
{
    auto *item = m_list->currentItem();
    if (!item) return {};
    return item->data(Qt::UserRole).toString();
}
