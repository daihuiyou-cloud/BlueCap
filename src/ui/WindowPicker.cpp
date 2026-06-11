#include "WindowPicker.h"
#include "utils/WindowDragHelper.h"
#include "theme/ThemeColors.h"
#include "widgets/ActionButton.h"
#include "widgets/PaintedLineEdit.h"
#include "widgets/StyledListWidget.h"
#include "utils/win32/WindowEnumerator.h"

#include <QAbstractButton>
#include "utils/win32/Win32Icon.h"
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
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

    auto *surfaceLayout = new QVBoxLayout(m_surface);
    surfaceLayout->setSpacing(12);
    surfaceLayout->setContentsMargins(20, 16, 20, 16);

    m_titleBar = new QWidget(m_surface);
    m_titleBar->setFixedHeight(48);
    auto *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);

    auto *header = new QLabel(QStringLiteral("选择窗口"), m_titleBar);
    header->setObjectName(QStringLiteral("pageHeader"));

    m_closeBtn = new QPushButton(QStringLiteral("✕"), m_titleBar);
    m_closeBtn->setFixedSize(30, 30);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setFlat(true);

    titleLayout->addWidget(header);
    titleLayout->addStretch();
    titleLayout->addWidget(m_closeBtn);
    surfaceLayout->addWidget(m_titleBar);

    auto *searchRow = new QHBoxLayout;
    searchRow->setSpacing(8);
    m_filterEdit = new PaintedLineEdit(m_surface);
    m_filterEdit->setPlaceholderText(QStringLiteral("搜索窗口..."));
    searchRow->addWidget(m_filterEdit, 1);

    m_refreshBtn = new ActionButton(QStringLiteral("刷新"), ActionButton::Reset, m_surface);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    m_refreshBtn->setToolTip(QStringLiteral("重新扫描当前打开的窗口"));
    searchRow->addWidget(m_refreshBtn);
    surfaceLayout->addLayout(searchRow);

    m_list = new StyledListWidget(m_surface);
    m_list->setIconSize(QSize(24, 24));
    surfaceLayout->addWidget(m_list, 1);

    auto *btnRow = new QWidget(m_surface);
    auto *btnLayout = new QHBoxLayout(btnRow);
    btnLayout->setContentsMargins(0, 6, 0, 0);
    btnLayout->addStretch();
    m_cancelBtn = new ActionButton(QStringLiteral("取消"), ActionButton::Reset, btnRow);
    m_okBtn = new ActionButton(QStringLiteral("确定"), ActionButton::Browse, btnRow);
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_okBtn);
    surfaceLayout->addWidget(btnRow);

    root->addWidget(m_surface);

    m_windows = window_enumerator::enumerateWindows();
    populateList();

    m_filterEdit->setFocus();
    if (m_list->count() > 0) {
        m_list->setCurrentRow(0);
    }

    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    m_filterDebounce = new QTimer(this);
    m_filterDebounce->setSingleShot(true);
    m_filterDebounce->setInterval(150);
    connect(m_filterDebounce, &QTimer::timeout, this, [this] {
        populateList(m_filterPending);
    });

    auto *filterShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this);
    connect(filterShortcut, &QShortcut::activated, m_filterEdit, qOverload<>(&QWidget::setFocus));

    connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_filterPending = text;
        m_filterDebounce->start();
    });
    connect(m_filterEdit, &QLineEdit::returnPressed, this, &QDialog::accept);

    connect(m_refreshBtn, &QPushButton::clicked, this, &WindowPicker::refreshWindows);

    connect(m_list, &QListWidget::itemDoubleClicked, this, &QDialog::accept);
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void WindowPicker::refreshWindows()
{
    m_windows = window_enumerator::enumerateWindows();
    m_iconCache.clear();
    populateList(m_filterEdit->text());
    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
    m_filterEdit->setFocus();
}

void WindowPicker::setDarkMode(bool dark)
{
    m_darkMode = dark;
    if (auto *edit = qobject_cast<PaintedLineEdit *>(m_filterEdit))
        edit->setDarkMode(dark);
    if (auto *button = qobject_cast<ActionButton *>(m_refreshBtn))
        button->setDarkMode(dark);
    if (auto *cb = qobject_cast<ActionButton *>(m_cancelBtn))
        cb->setDarkMode(dark);
    if (auto *ob = qobject_cast<ActionButton *>(m_okBtn))
        ob->setDarkMode(dark);
    if (auto *list = qobject_cast<StyledListWidget *>(m_list))
        list->setDarkMode(dark);
    const auto &a = ThemeColors::forMode(dark).app;
    QPalette p = m_filterEdit->palette();
    p.setColor(QPalette::Highlight, a.selectionBg);
    p.setColor(QPalette::HighlightedText, a.selectionText);
    m_filterEdit->setPalette(p);
    update();
}

void WindowPicker::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &a = ThemeColors::forMode(m_darkMode).app;
    const QRectF body = rect().adjusted(14, 14, -14, -14);

    for (int i = 5; i >= 1; --i) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, m_darkMode ? 6 + i * 4 : 4 + i * 3));
        const qreal k = i * 1.5;
        painter.drawRoundedRect(body.adjusted(-k, -k + 2, k, k + 4), 16 + k, 16 + k);
    }

    QPainterPath path;
    path.addRoundedRect(body, 12, 12);
    painter.setPen(Qt::NoPen);
    painter.setBrush(a.dialogSurfaceBg);
    painter.drawPath(path);
}

void WindowPicker::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPoint pos = m_surface->mapFrom(this, event->pos());
        if (window_drag::handlePress(this, m_titleBar, pos, event, m_dragState))
            return;
    }
    QDialog::mousePressEvent(event);
}

void WindowPicker::mouseMoveEvent(QMouseEvent *event)
{
    if (window_drag::handleMove(this, event, m_dragState))
        return;
    QDialog::mouseMoveEvent(event);
}

void WindowPicker::mouseReleaseEvent(QMouseEvent *event)
{
    window_drag::handleRelease(event, m_dragState);
    QDialog::mouseReleaseEvent(event);
}

void WindowPicker::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
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
            HICON hIcon = nullptr;
            DWORD_PTR result = 0;
            if (SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL2, 0,
                                    SMTO_ABORTIFHUNG, 200, &result))
                hIcon = reinterpret_cast<HICON>(result);
            if (!hIcon && SendMessageTimeoutW(hwnd, WM_GETICON, ICON_BIG, 0,
                                              SMTO_ABORTIFHUNG, 200, &result))
                hIcon = reinterpret_cast<HICON>(result);
            if (!hIcon)
                hIcon = reinterpret_cast<HICON>(
                    GetClassLongPtrW(hwnd, GCLP_HICONSM));
            QPixmap px = win32::iconFromHICON(hIcon);
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
