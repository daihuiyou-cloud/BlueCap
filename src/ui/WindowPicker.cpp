#include "WindowPicker.h"
#include "recorder/RecorderController.h"

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QIcon>
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
    setWindowTitle(QStringLiteral("选择窗口"));
    setMinimumSize(420, 320);
    resize(480, 400);

    auto *root = new QVBoxLayout(this);
    root->setSpacing(12);

    auto *header = new QLabel(QStringLiteral("请选择要录制的窗口："), this);
    header->setObjectName(QStringLiteral("pageHeader"));
    root->addWidget(header);

    auto *searchRow = new QHBoxLayout;
    searchRow->setSpacing(8);
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(QStringLiteral("搜索窗口..."));
    searchRow->addWidget(m_filterEdit, 1);

    m_refreshBtn = new QPushButton(QStringLiteral("刷新"), this);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    m_refreshBtn->setToolTip(QStringLiteral("重新扫描当前打开的窗口"));
    searchRow->addWidget(m_refreshBtn);
    root->addLayout(searchRow);

    m_list = new QListWidget(this);
    m_list->setAlternatingRowColors(true);
    m_list->setIconSize(QSize(24, 24));
    root->addWidget(m_list, 1);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttonBox);

    m_windows = RecorderController::enumerateWindows();
    populateList();

    m_filterEdit->setFocus();
    if (m_list->count() > 0) {
        m_list->setCurrentRow(0);
    }

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
    populateList(m_filterEdit->text());
    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
    m_filterEdit->setFocus();
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

        // Try to retrieve the window icon
        HWND hwnd = reinterpret_cast<HWND>(entry.hwnd);
        HICON hIcon = reinterpret_cast<HICON>(
            SendMessageW(hwnd, WM_GETICON, ICON_SMALL2, 0));
        if (!hIcon)
            hIcon = reinterpret_cast<HICON>(
                GetClassLongPtrW(hwnd, GCLP_HICONSM));
        QPixmap px = iconFromHICON(hIcon);
        if (!px.isNull())
            item->setIcon(QIcon(px));
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
