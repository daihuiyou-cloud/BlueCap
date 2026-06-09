#include "ToastWidget.h"
#include "theme/ThemeColors.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QTimer>

ToastWidget::ToastWidget(QWidget *parent)
    : QWidget(parent)
{
    setVisible(false);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 10, 16, 10);
    m_label = new QLabel(this);
    QFont font = m_label->font();
    font.setPixelSize(13);
    font.setBold(true);
    m_label->setFont(font);
    m_label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    layout->addWidget(m_label, 1);
    applyTextColor();

    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, &ToastWidget::fadeOut);

    m_fadeAnim = new QPropertyAnimation(this, "toastOpacity", this);
    m_fadeAnim->setDuration(200);
    connect(m_fadeAnim, &QPropertyAnimation::finished, this, [this] {
        if (m_animatingHide) {
            m_animatingHide = false;
            QWidget::setVisible(false);
        }
    });
}

void ToastWidget::setToastOpacity(qreal opacity)
{
    m_toastOpacity = opacity;
    update();
}

void ToastWidget::showMessage(const QString &message, int durationMs)
{
    m_label->setText(message);
    if (durationMs <= 0) durationMs = 5000;

    m_hideTimer->stop();
    m_fadeAnim->stop();
    m_animatingHide = false;

    positionAtBottom();
    raise();
    m_toastOpacity = 0.0;
    QWidget::setVisible(true);
    fadeIn();
    m_hideTimer->start(durationMs);
}

void ToastWidget::dismissImmediately()
{
    m_hideTimer->stop();
    m_fadeAnim->stop();
    m_animatingHide = false;
    QWidget::setVisible(false);
}

void ToastWidget::fadeIn()
{
    m_fadeAnim->setStartValue(m_toastOpacity);
    m_fadeAnim->setEndValue(1.0);
    m_fadeAnim->setDirection(QAbstractAnimation::Forward);
    m_fadeAnim->start();
}

void ToastWidget::fadeOut()
{
    m_animatingHide = true;
    m_fadeAnim->setDirection(QAbstractAnimation::Backward);
    m_fadeAnim->start();
}

void ToastWidget::positionAtBottom()
{
    if (!parentWidget()) return;

    adjustSize();
    int pw = parentWidget()->width();
    int ph = parentWidget()->height();
    int w = qMin(width() + 32, pw - 40);
    int h = height();
    move((pw - w) / 2, ph - h - 20);
    resize(w, h);
}

void ToastWidget::setDarkMode(bool dark)
{
    m_darkMode = dark;
    applyTextColor();
    update();
}

void ToastWidget::applyTextColor()
{
    const auto &a = ThemeColors::forMode(m_darkMode).app;
    QPalette p = m_label->palette();
    p.setColor(QPalette::WindowText, a.toastText);
    m_label->setPalette(p);
}

bool ToastWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ParentChange) {
        if (parentWidget())
            parentWidget()->installEventFilter(this);
        positionAtBottom();
    }
    return QWidget::event(event);
}

bool ToastWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        if (isVisible())
            positionAtBottom();
    }
    return QWidget::eventFilter(watched, event);
}

void ToastWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(m_toastOpacity);

    const auto &a = ThemeColors::forMode(m_darkMode).app;
    QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath path;
    path.addRoundedRect(r, 8, 8);

    painter.setPen(Qt::NoPen);
    painter.setBrush(a.toastBg);
    painter.drawPath(path);
}
