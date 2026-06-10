#include "BlueCapStyle.h"
#include "theme/Theme.h"
#include "theme/ThemeColors.h"
#include "theme/ThemePreference.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QStyleOption>
#include <QStyleOptionButton>
#include <QStyleOptionComplex>
#include <QStyleOptionComboBox>
#include <QStyleOptionFrame>
#include <QStyleOptionHeader>
#include <QStyleOptionMenuItem>
#include <QStyleOptionProgressBar>
#include <QStyleOptionSpinBox>
#include <QStyleOptionTab>
#include <QStyleOptionToolButton>
#include <QToolTip>

bool BlueCapStyle::s_darkMode = false;

BlueCapStyle::BlueCapStyle()
    : QProxyStyle()
{
}

void BlueCapStyle::setDarkMode(bool dark)
{
    s_darkMode = dark;
}

void BlueCapStyle::applyTheme(int preference, QApplication *app)
{
    if (!app) app = qApp;
    if (!app) return;
    const int th = theme::resolve(preference);
    bool dark = (th == ThemeDark);
    const auto &a = ThemeColors::forMode(dark).app;
    QPalette pal = app->palette();
    pal.setColor(QPalette::WindowText, a.defaultText);
    app->setPalette(pal);

    auto *style = qobject_cast<BlueCapStyle *>(app->style());
    if (style) {
        style->setDarkMode(dark);
        app->setStyleSheet(QString());
        app->style()->unpolish(app);
        app->style()->polish(app);
    }
}

const BlueCapStyle *BlueCapStyle::styleInstance()
{
    return qobject_cast<const BlueCapStyle *>(QApplication::style());
}

const ThemeAppearance &BlueCapStyle::styleColors()
{
    static const auto light = ThemeColors::light();
    static const auto dark = ThemeColors::dark();
    return s_darkMode ? dark.app : light.app;
}

void BlueCapStyle::drawPrimitive(PrimitiveElement element,
                                 const QStyleOption *option,
                                 QPainter *painter,
                                 const QWidget *widget) const
{
    const auto &clr = styleColors();
    painter->setRenderHint(QPainter::Antialiasing);

    switch (element) {
    case PE_PanelButtonCommand: {
        auto *btnOpt = qstyleoption_cast<const QStyleOptionButton *>(option);
        bool hover = btnOpt && (btnOpt->state & State_MouseOver);
        bool pressed = btnOpt && (btnOpt->state & State_Sunken);
        bool disabled = !(option->state & State_Enabled);
        bool focused = btnOpt && (btnOpt->state & State_HasFocus);
        bool isBottomIcon = widget && widget->property("bluecapRole").toString() == QStringLiteral("bottomIcon");

        QRect r = option->rect;
        QPainterPath path;
        path.addRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), 6, 6);

        QColor bg = disabled ? (isBottomIcon ? clr.actionButtonDisabledBg : clr.qpushBtnDisabledBg)
                   : pressed ? (isBottomIcon ? clr.bottomButtonPressedBg : clr.qpushBtnPressedBg)
                   : hover   ? (isBottomIcon ? clr.bottomButtonHoverBg : clr.qpushBtnHoverBg)
                   :           (isBottomIcon ? clr.bottomButtonBg : clr.qpushBtnBg);
        QColor border = isBottomIcon
            ? (hover && !disabled ? clr.bottomButtonHoverBorder : clr.bottomButtonBorder)
            : clr.qpushBtnBorder;
        painter->setPen(QPen(border, 1));
        painter->setBrush(bg);
        painter->drawPath(path);

        if (focused && !pressed) {
            QPen focusPen(clr.inputFocusBorder, 1.5);
            painter->setPen(focusPen);
            painter->setBrush(Qt::NoBrush);
            QPainterPath focusPath;
            focusPath.addRoundedRect(QRectF(r).adjusted(0.75, 0.75, -0.75, -0.75), 6, 6);
            painter->drawPath(focusPath);
        }
        return;
    }
    case PE_FrameLineEdit: {
        auto *fOpt = qstyleoption_cast<const QStyleOptionFrame *>(option);
        bool hover = fOpt && (fOpt->state & State_MouseOver);
        bool focus = fOpt && (fOpt->state & State_HasFocus);
        bool disabled = !(option->state & State_Enabled);

        QRect r = option->rect;
        QPainterPath path;
        path.addRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), 6, 6);

        QColor bg = disabled ? clr.inputDisabledBg
                   : focus   ? clr.inputFocusBg
                   : hover   ? clr.inputHoverBg
                   :           clr.inputBg;
        QColor border = disabled ? clr.inputDisabledBg
                        : focus  ? clr.inputFocusBorder
                        : hover  ? clr.inputHoverBorder
                        :          clr.inputBorder;

        painter->setPen(QPen(border, 1));
        painter->setBrush(bg);
        painter->drawPath(path);
        return;
    }
    case PE_IndicatorCheckBox: {
        auto *cbOpt = qstyleoption_cast<const QStyleOptionButton *>(option);
        bool checked = cbOpt && (cbOpt->state & State_On);
        bool hover = cbOpt && (cbOpt->state & State_MouseOver);
        bool disabled = !(option->state & State_Enabled);

        QRect r = option->rect;
        QPainterPath path;
        path.addRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), 5, 5);

        if (checked) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(clr.checkboxIndicatorChecked);
            painter->drawPath(path);

            painter->setPen(QPen(Qt::white, 2));
            painter->setBrush(Qt::NoBrush);
            qreal cx = r.center().x();
            qreal cy = r.center().y();
            QPainterPath check;
            check.moveTo(cx - 5, cy);
            check.lineTo(cx - 2, cy + 3);
            check.lineTo(cx + 4, cy - 3);
            painter->drawPath(check);
        } else {
            QColor bg = disabled ? clr.checkboxIndicatorDisabledBg : clr.checkboxIndicatorBg;
            QColor border = disabled ? clr.checkboxIndicatorDisabledBorder
                           : hover   ? clr.checkboxIndicatorHoverBorder
                           :           clr.checkboxIndicatorBorder;
            painter->setPen(QPen(border, 1));
            painter->setBrush(bg);
            painter->drawPath(path);
        }
        return;
    }
    case PE_IndicatorItemViewItemCheck:
        drawPrimitive(PE_IndicatorCheckBox, option, painter, widget);
        return;
    case PE_PanelMenu: {
        QRect r = option->rect;
        painter->setPen(QPen(clr.menuBorder, 1));
        painter->setBrush(clr.menuBg);
        painter->drawRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);
        return;
    }
    case PE_FrameMenu: {
        return;
    }
    case PE_PanelItemViewItem: {
        bool selected = option->state & State_Selected;
        if (selected) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(clr.menuItemSelectedBg);
            painter->drawRoundedRect(QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
        }
        return;
    }
    case PE_FrameDefaultButton:
        return;
    case PE_PanelButtonTool: {
        auto *tbOpt = qstyleoption_cast<const QStyleOptionToolButton *>(option);
        bool hover = tbOpt && (tbOpt->state & State_MouseOver);
        bool pressed = tbOpt && (tbOpt->state & State_Sunken);
        bool checked = tbOpt && (tbOpt->state & State_On);
        bool disabled = !(option->state & State_Enabled);
        if (!hover && !pressed && !checked)
            return;
        QColor bg = disabled ? Qt::transparent
                   : (pressed || checked) ? clr.modeButtonCheckedBg
                   :                          clr.qpushBtnHoverBg;
        QRect r = option->rect;
        painter->setPen(Qt::NoPen);
        painter->setBrush(bg);
        painter->drawRoundedRect(QRectF(r), 6, 6);
        return;
    }
    case PE_FrameTabWidget:
        return;
    case PE_Frame:
        return;
    case PE_Widget:
        return;
    default:
        break;
    }

    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void BlueCapStyle::drawControl(ControlElement element,
                               const QStyleOption *option,
                               QPainter *painter,
                               const QWidget *widget) const
{
    const auto &clr = styleColors();
    painter->setRenderHint(QPainter::Antialiasing);

    switch (element) {
    case CE_PushButtonLabel: {
        auto *btnOpt = qstyleoption_cast<const QStyleOptionButton *>(option);
        if (!btnOpt) break;
        bool disabled = !(option->state & State_Enabled);
        QColor textColor = disabled ? clr.qpushBtnDisabledText : clr.qpushBtnText;
        painter->setPen(textColor);
        QRect textRect = subElementRect(SE_PushButtonContents, option, widget);
        QString text = btnOpt->text;
        if (!text.isEmpty()) {
            painter->drawText(textRect, Qt::AlignCenter, text);
        }
        if (btnOpt->icon.isNull())
            return;
        QIcon::Mode mode = disabled ? QIcon::Disabled : QIcon::Normal;
        QPixmap pix = btnOpt->icon.pixmap(btnOpt->iconSize, mode);
        if (!pix.isNull()) {
            int iconW = btnOpt->iconSize.width();
            QRect iconRect(textRect.left() + 2,
                           textRect.center().y() - iconW / 2,
                           iconW, iconW);
            painter->drawPixmap(iconRect, pix);
        }
        return;
    }
    case CE_CheckBoxLabel:
    case CE_RadioButtonLabel: {
        auto *btnOpt = qstyleoption_cast<const QStyleOptionButton *>(option);
        if (!btnOpt) break;
        bool disabled = !(option->state & State_Enabled);
        painter->setPen(disabled ? clr.checkboxDisabledText : clr.checkboxText);
        QFont font = painter->font();
        font.setPixelSize(14);
        painter->setFont(font);
        QRect textRect = proxy()->subElementRect(SE_CheckBoxContents, option, widget);
        if (textRect.isNull())
            textRect = option->rect.adjusted(22, 0, -4, 0);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, btnOpt->text);
        return;
    }
    case CE_ProgressBarGroove: {
        QRect r = option->rect;
        QPainterPath path;
        path.addRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), 3, 3);
        painter->setPen(QPen(clr.progressBorder, 1));
        painter->setBrush(clr.progressBg);
        painter->drawPath(path);
        return;
    }
    case CE_ProgressBarContents: {
        auto *pbOpt = qstyleoption_cast<const QStyleOptionProgressBar *>(option);
        if (!pbOpt) break;
        qreal progress = 0;
        if (pbOpt->maximum > pbOpt->minimum)
            progress = qreal(pbOpt->progress - pbOpt->minimum) / (pbOpt->maximum - pbOpt->minimum);
        QRect r = option->rect.adjusted(1, 1, -1, -1);
        int chunkW = qMax(0, qRound(r.width() * progress));
        if (chunkW <= 0) return;
        QRect chunkRect(r.left(), r.top(), chunkW, r.height());
        QPainterPath path;
        path.addRoundedRect(QRectF(chunkRect), 2, 2);
        QLinearGradient grad(chunkRect.topLeft(), chunkRect.topRight());
        grad.setColorAt(0.0, clr.progressChunkFrom);
        grad.setColorAt(1.0, clr.progressChunkTo);
        painter->setPen(Qt::NoPen);
        painter->setBrush(grad);
        painter->drawPath(path);
        return;
    }
    case CE_MenuItem: {
        auto *mOpt = qstyleoption_cast<const QStyleOptionMenuItem *>(option);
        if (!mOpt) break;
        bool selected = mOpt->state & State_Selected;
        bool disabled = !(mOpt->state & State_Enabled);

        QRect r = mOpt->rect;
        if (selected && !disabled) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(clr.menuItemSelectedBg);
            painter->drawRoundedRect(QRectF(r).adjusted(1, 0, -1, 0), 4, 4);
        }
        if (disabled && !selected) {
            painter->setPen(clr.menuItemText);
        } else if (selected) {
            painter->setPen(clr.menuItemSelectedText);
        } else {
            painter->setPen(clr.menuItemText);
        }
        QFont font = mOpt->font;
        painter->setFont(font);
        int leftMargin = (!mOpt->icon.isNull() || mOpt->checkType != QStyleOptionMenuItem::NotCheckable) ? 36 : 24;
        QRect textRect = r.adjusted(leftMargin, 0, -12, 0);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, mOpt->text);
        return;
    }
    case CE_MenuHMargin:
    case CE_MenuVMargin:
    case CE_MenuEmptyArea:
        return;
    case CE_ScrollBarSlider: {
        auto *sbOpt = qstyleoption_cast<const QStyleOptionSlider *>(option);
        bool hover = sbOpt && (sbOpt->state & State_MouseOver);
        bool pressed = sbOpt && (sbOpt->state & State_Sunken);
        QRect r = option->rect;
        QColor handleColor = pressed ? clr.scrollbarHandlePressed
                             : hover  ? clr.scrollbarHandleHover
                             :          clr.scrollbarHandle;
        painter->setPen(Qt::NoPen);
        painter->setBrush(handleColor);
        if (r.width() > r.height()) {
            painter->drawRoundedRect(QRectF(r), 4, r.height() / 2);
        } else {
            painter->drawRoundedRect(QRectF(r), r.width() / 2, 4);
        }
        return;
    }
    case CE_ScrollBarAddPage:
    case CE_ScrollBarSubPage:
        return;
    case CE_ScrollBarAddLine:
    case CE_ScrollBarSubLine:
        return;
    case CE_HeaderEmptyArea:
        return;
    case CE_HeaderSection: {
        painter->setPen(Qt::NoPen);
        painter->setBrush(clr.menuBg);
        painter->drawRect(option->rect);
        return;
    }
    default:
        break;
    }

    QProxyStyle::drawControl(element, option, painter, widget);
}

void BlueCapStyle::drawComplexControl(ComplexControl control,
                                      const QStyleOptionComplex *option,
                                      QPainter *painter,
                                      const QWidget *widget) const
{
    const auto &clr = styleColors();
    painter->setRenderHint(QPainter::Antialiasing);

    switch (control) {
    case CC_SpinBox: {
        auto *sbOpt = qstyleoption_cast<const QStyleOptionSpinBox *>(option);
        if (!sbOpt) break;
        bool hover = sbOpt->state & State_MouseOver;
        bool focus = sbOpt->state & State_HasFocus;
        bool disabled = !(sbOpt->state & State_Enabled);

        QRect frameRect = sbOpt->rect;
        QPainterPath path;
        path.addRoundedRect(QRectF(frameRect).adjusted(0.5, 0.5, -0.5, -0.5), 6, 6);
        QColor bg = disabled ? clr.inputDisabledBg
                   : focus   ? clr.inputFocusBg
                   : hover   ? clr.inputHoverBg
                   :           clr.inputBg;
        painter->setPen(QPen(focus ? clr.inputFocusBorder : clr.inputBorder, 1));
        painter->setBrush(bg);
        painter->drawPath(path);

        QRect upRect = proxy()->subControlRect(CC_SpinBox, sbOpt, SC_SpinBoxUp, widget);
        QRect downRect = proxy()->subControlRect(CC_SpinBox, sbOpt, SC_SpinBoxDown, widget);

        bool upHover = sbOpt->activeSubControls == SC_SpinBoxUp && (sbOpt->state & State_MouseOver);
        bool downHover = sbOpt->activeSubControls == SC_SpinBoxDown && (sbOpt->state & State_MouseOver);
        bool upPressed = sbOpt->activeSubControls == SC_SpinBoxUp && (sbOpt->state & State_Sunken);
        bool downPressed = sbOpt->activeSubControls == SC_SpinBoxDown && (sbOpt->state & State_Sunken);

        auto drawSpinButton = [&](const QRect &btnRect, bool isHover, bool isPressed) {
            if (isPressed) {
                painter->setPen(Qt::NoPen);
                painter->setBrush(clr.spinButtonPressedBg);
                painter->drawRoundedRect(QRectF(btnRect), 3, 3);
            } else if (isHover) {
                painter->setPen(Qt::NoPen);
                painter->setBrush(clr.spinButtonHoverBg);
                painter->drawRoundedRect(QRectF(btnRect), 3, 3);
            }
        };

        drawSpinButton(upRect, upHover, upPressed);
        drawSpinButton(downRect, downHover, downPressed);

        auto drawArrow = [&](const QRect &btnRect, bool up) {
            QColor arrowColor = clr.inputText;
            painter->setPen(QPen(arrowColor, 1.8));
            painter->setBrush(Qt::NoBrush);
            qreal cx = btnRect.center().x();
            qreal cy = btnRect.center().y();
            if (up) {
                painter->drawLine(cx - 4, cy + 3, cx, cy - 3);
                painter->drawLine(cx + 4, cy + 3, cx, cy - 3);
            } else {
                painter->drawLine(cx - 4, cy - 3, cx, cy + 3);
                painter->drawLine(cx + 4, cy - 3, cx, cy + 3);
            }
        };

        drawArrow(upRect, true);
        drawArrow(downRect, false);
        return;
    }
    case CC_ComboBox: {
        auto *cbOpt = qstyleoption_cast<const QStyleOptionComboBox *>(option);
        if (!cbOpt) break;
        bool hover = cbOpt->state & State_MouseOver;
        bool focus = cbOpt->state & State_HasFocus;
        bool disabled = !(cbOpt->state & State_Enabled);

        QRect r = cbOpt->rect;
        QPainterPath path;
        path.addRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), 6, 6);
        QColor bg = disabled ? clr.inputDisabledBg
                   : focus   ? clr.inputFocusBg
                   : hover   ? clr.inputHoverBg
                   :           clr.inputBg;
        painter->setPen(QPen(focus ? clr.inputFocusBorder : clr.inputBorder, 1));
        painter->setBrush(bg);
        painter->drawPath(path);

        QRect arrowRect = proxy()->subControlRect(CC_ComboBox, cbOpt, SC_ComboBoxArrow, widget);
        QColor arrowColor = disabled ? clr.inputDisabledText : clr.inputText;
        painter->setPen(QPen(arrowColor, 2));
        painter->setBrush(Qt::NoBrush);
        qreal cx = arrowRect.center().x();
        qreal cy = arrowRect.center().y();
        painter->drawLine(cx - 4, cy - 3, cx, cy + 3);
        painter->drawLine(cx + 4, cy - 3, cx, cy + 3);
        return;
    }
    default:
        break;
    }

    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

QSize BlueCapStyle::sizeFromContents(ContentsType type,
                                     const QStyleOption *option,
                                     const QSize &size,
                                     const QWidget *widget) const
{
    QSize sz = QProxyStyle::sizeFromContents(type, option, size, widget);
    switch (type) {
    case CT_PushButton:
        if (auto *btn = qobject_cast<const QPushButton *>(widget)) {
            if (btn->text().isEmpty() && !btn->icon().isNull()) {
                sz += QSize(4, 4);
            } else {
                sz += QSize(12, 8);
            }
        }
        break;
    case CT_LineEdit:
        sz += QSize(8, 6);
        break;
    case CT_ComboBox:
        sz += QSize(12, 8);
        break;
    case CT_SpinBox:
        sz += QSize(8, 8);
        break;
    case CT_MenuItem:
        sz += QSize(8, 4);
        break;
    default:
        break;
    }
    return sz;
}

QRect BlueCapStyle::subControlRect(ComplexControl cc,
                                   const QStyleOptionComplex *opt,
                                   SubControl sc,
                                   const QWidget *widget) const
{
    QRect rect = QProxyStyle::subControlRect(cc, opt, sc, widget);
    if (cc == CC_SpinBox) {
        if (sc == SC_SpinBoxUp || sc == SC_SpinBoxDown) {
            rect.setWidth(24);
            if (sc == SC_SpinBoxUp)
                rect.moveRight(opt->rect.right() - 1);
            else
                rect.moveRight(opt->rect.right() - 1);
            rect.setHeight(opt->rect.height() / 2);
            if (sc == SC_SpinBoxDown)
                rect.moveTop(opt->rect.center().y());
        }
    }
    return rect;
}

int BlueCapStyle::pixelMetric(PixelMetric metric,
                              const QStyleOption *option,
                              const QWidget *widget) const
{
    switch (metric) {
    case PM_ScrollBarExtent:
        return 8;
    default:
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
}
