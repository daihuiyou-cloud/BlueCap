#include "PaintedDialog.h"
#include "theme/Theme.h"

#include "paint/PaintPrimitives.h"
#include "widgets/ActionButton.h"
#include "widgets/PaintedLineEdit.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

namespace {
bool dialogIsDarkMode()
{
    QSettings s(QStringLiteral("BlueCap"), QStringLiteral("BlueCap"));
    int pref = s.value(QStringLiteral("settings/theme"), ThemeSystem).toInt();
    return theme::resolve(pref) == ThemeDark;
}
}

PaintedDialog::PaintedDialog(const QString &title, const QString &message, QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setMinimumWidth(380);
    m_palette = paint::theme(false);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(0);

    auto *surface = new QWidget(this);
    auto *layout = new QVBoxLayout(surface);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(14);

    m_titleLabel = new QLabel(title, surface);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPixelSize(18);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setObjectName(QStringLiteral("dialogTitle"));

    auto *messageLabel = new QLabel(message, surface);
    messageLabel->setWordWrap(true);
    QFont msgFont = messageLabel->font();
    msgFont.setPixelSize(13);
    messageLabel->setFont(msgFont);
    messageLabel->setObjectName(QStringLiteral("dialogMessage"));

    layout->addWidget(m_titleLabel);
    layout->addWidget(messageLabel);
    root->addWidget(surface);
}

void PaintedDialog::setDarkMode(bool dark)
{
    m_darkMode = dark;
    m_palette = paint::theme(dark);
    for (auto *label : findChildren<QLabel *>()) {
        QPalette p = label->palette();
        p.setColor(QPalette::WindowText,
                   label->objectName() == QStringLiteral("dialogTitle") ? m_palette.text : m_palette.mutedText);
        label->setPalette(p);
    }
    for (auto *button : findChildren<ActionButton *>())
        button->setDarkMode(dark);
    for (auto *edit : findChildren<PaintedLineEdit *>())
        edit->setDarkMode(dark);
    update();
}

void PaintedDialog::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF shadow = QRectF(rect()).adjusted(18, 18, -18, -18);
    for (int i = 6; i > 0; --i) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, m_darkMode ? 7 + i * 5 : 5 + i * 4));
        p.drawRoundedRect(shadow.adjusted(-i * 1.5, -i, i * 1.5, i * 2), 16 + i, 16 + i);
    }
    paint::drawCard(p, shadow, m_palette.panelBg, m_palette.panelBorder, 14);
    paint::drawVerticalSheen(p, shadow, 14, m_darkMode);
}

void PaintedDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_titleLabel) {
        QWidget *titleParent = m_titleLabel->parentWidget();
        QPoint pos = titleParent ? titleParent->mapFrom(this, event->pos()) : event->pos();
        if (window_drag::handlePress(this, m_titleLabel, pos, event, m_dragState))
            return;
    }
    QDialog::mousePressEvent(event);
}

void PaintedDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (window_drag::handleMove(this, event, m_dragState))
        return;
    QDialog::mouseMoveEvent(event);
}

void PaintedDialog::mouseReleaseEvent(QMouseEvent *event)
{
    window_drag::handleRelease(event, m_dragState);
    QDialog::mouseReleaseEvent(event);
}

bool PaintedDialog::question(QWidget *parent, const QString &title, const QString &message,
                             const QString &yesText, const QString &noText)
{
    PaintedDialog dialog(title, message, parent);
    auto *surface = dialog.findChild<QWidget *>();
    auto *layout = qobject_cast<QVBoxLayout *>(surface->layout());
    auto *buttons = new QWidget(surface);
    auto *buttonLayout = new QHBoxLayout(buttons);
    buttonLayout->setContentsMargins(0, 6, 0, 0);
    buttonLayout->addStretch();
    auto *no = new ActionButton(noText, ActionButton::Reset, buttons);
    auto *yes = new ActionButton(yesText, ActionButton::Browse, buttons);
    buttonLayout->addWidget(no);
    buttonLayout->addWidget(yes);
    layout->addWidget(buttons);
    QObject::connect(no, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(yes, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.setDarkMode(dialogIsDarkMode());
    return dialog.exec() == QDialog::Accepted;
}

void PaintedDialog::warning(QWidget *parent, const QString &title, const QString &message)
{
    PaintedDialog dialog(title, message, parent);
    auto *surface = dialog.findChild<QWidget *>();
    auto *layout = qobject_cast<QVBoxLayout *>(surface->layout());
    auto *buttons = new QWidget(surface);
    auto *buttonLayout = new QHBoxLayout(buttons);
    buttonLayout->setContentsMargins(0, 6, 0, 0);
    buttonLayout->addStretch();
    auto *ok = new ActionButton(QStringLiteral("确定"), ActionButton::Browse, buttons);
    buttonLayout->addWidget(ok);
    layout->addWidget(buttons);
    QObject::connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.setDarkMode(dialogIsDarkMode());
    dialog.exec();
}

QString PaintedDialog::getText(QWidget *parent, const QString &title, const QString &label,
                               const QString &text, bool *ok)
{
    PaintedDialog dialog(title, label, parent);
    auto *surface = dialog.findChild<QWidget *>();
    auto *layout = qobject_cast<QVBoxLayout *>(surface->layout());
    auto *edit = new PaintedLineEdit(surface);
    edit->setFixedHeight(42);
    edit->setText(text);
    layout->addWidget(edit);
    auto *buttons = new QWidget(surface);
    auto *buttonLayout = new QHBoxLayout(buttons);
    buttonLayout->setContentsMargins(0, 6, 0, 0);
    buttonLayout->addStretch();
    auto *cancel = new ActionButton(QStringLiteral("取消"), ActionButton::Reset, buttons);
    auto *confirm = new ActionButton(QStringLiteral("确定"), ActionButton::Browse, buttons);
    buttonLayout->addWidget(cancel);
    buttonLayout->addWidget(confirm);
    layout->addWidget(buttons);
    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(confirm, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.setDarkMode(dialogIsDarkMode());
    edit->setFocus();
    const bool accepted = dialog.exec() == QDialog::Accepted;
    if (ok)
        *ok = accepted;
    return accepted ? edit->text() : QString();
}
