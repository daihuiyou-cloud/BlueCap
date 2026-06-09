#include "SettingsPage.h"
#include "paint/PaintMetrics.h"
#include "theme/ThemeColors.h"
#include "storage/ISettingsRepository.h"
#include "widgets/PageHeader.h"
#include "widgets/SettingsPanel.h"
#include "widgets/SettingsSeparator.h"
#include "widgets/SaveFeedbackLabel.h"
#include "widgets/ActionButton.h"
#include "widgets/PaintedCheckBox.h"
#include "widgets/PaintedComboBox.h"
#include "widgets/PaintedLineEdit.h"
#include "widgets/PaintedSpinBox.h"
#include "widgets/PaintedScrollBar.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>

namespace {
const QLatin1String kSettingsPrefix("settings/");

QFormLayout *makeFormLayout()
{
    auto *layout = new QFormLayout;
    layout->setHorizontalSpacing(18);
    layout->setVerticalSpacing(12);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    return layout;
}
}

SettingsPage::SettingsPage(ISettingsRepository *settings, QWidget *parent)
    : QWidget(parent)
    , m_settings(settings)
{
    setAutoFillBackground(false);
    setAttribute(Qt::WA_StyledBackground, false);
    setAttribute(Qt::WA_TranslucentBackground);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(22, 16, 24, 18);
    root->setSpacing(12);

    auto *header = new PageHeader(QStringLiteral("设置"), this);
    root->addWidget(header);

    auto *panel = new SettingsPanel(this);
    panel->setDarkMode(m_darkMode);

    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(24, 22, 24, 22);
    panelLayout->setSpacing(18);

    const QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
        + QStringLiteral("/BlueCap");

    {
        auto *form = new QWidget(panel);
        form->setAutoFillBackground(false);
        auto *layout = makeFormLayout();
        form->setLayout(layout);

        m_pathEdit = new PaintedLineEdit(form);
        m_pathEdit->setFixedHeight(paint::Metrics::inputHeight);
        m_pathEdit->setMinimumWidth(paint::Metrics::inputMinWidth);
        m_pathEdit->setPlaceholderText(defaultPath);
        m_pathEdit->setToolTip(QStringLiteral("录制文件的保存位置，点击“浏览...”选择文件夹"));
        auto *browseBtn = new ActionButton(QStringLiteral("浏览..."), ActionButton::Browse, form);
        browseBtn->setToolTip(QStringLiteral("浏览选择文件夹"));
        auto *pathRow = new QWidget(form);
        pathRow->setAutoFillBackground(false);
        auto *pathLayout = new QHBoxLayout(pathRow);
        pathLayout->setContentsMargins(0, 0, 0, 0);
        pathLayout->setSpacing(10);
        pathLayout->addWidget(m_pathEdit, 1);
        pathLayout->addWidget(browseBtn, 0, Qt::AlignRight);
        layout->addRow(QStringLiteral("保存路径"), pathRow);

        m_fpsSpin = new PaintedSpinBox(form);
        m_fpsSpin->setFixedHeight(paint::Metrics::inputHeight);
        m_fpsSpin->setMinimumWidth(paint::Metrics::inputMinWidth);
        m_fpsSpin->setRange(15, 60);
        m_fpsSpin->setSuffix(QStringLiteral(" fps"));
        m_fpsSpin->setToolTip(QStringLiteral("帧率越高，视频越流畅但文件越大。屏幕录制推荐 30 fps。"));
        m_fpsSpin->installEventFilter(this);
        layout->addRow(QStringLiteral("帧率"), m_fpsSpin);

        m_qualityCombo = new PaintedComboBox(form);
        m_qualityCombo->setFixedHeight(paint::Metrics::inputHeight);
        m_qualityCombo->setMinimumWidth(paint::Metrics::inputMinWidth);
        m_qualityCombo->setMaxVisibleItems(7);
        m_qualityCombo->addItem(QStringLiteral("ultrafast (低画质)"), QStringLiteral("ultrafast"));
        m_qualityCombo->addItem(QStringLiteral("superfast"), QStringLiteral("superfast"));
        m_qualityCombo->addItem(QStringLiteral("veryfast"), QStringLiteral("veryfast"));
        m_qualityCombo->addItem(QStringLiteral("faster"), QStringLiteral("faster"));
        m_qualityCombo->addItem(QStringLiteral("fast (推荐)"), QStringLiteral("fast"));
        m_qualityCombo->addItem(QStringLiteral("medium"), QStringLiteral("medium"));
        m_qualityCombo->addItem(QStringLiteral("slow (高画质)"), QStringLiteral("slow"));
        m_qualityCombo->setToolTip(QStringLiteral("画质越高，视频文件越大。推荐选择 fast 获得较好的平衡。"));
        m_qualityCombo->installEventFilter(this);
        layout->addRow(QStringLiteral("画质"), m_qualityCombo);

        m_showCursorCheck = new PaintedCheckBox(QStringLiteral("录制时显示"), form);
        m_showCursorCheck->setFixedHeight(paint::Metrics::inputHeight);
        m_showCursorCheck->setToolTip(QStringLiteral("录制时是否包含鼠标光标"));
        layout->addRow(QStringLiteral("显示光标"), m_showCursorCheck);

        panelLayout->addWidget(createSection(QStringLiteral("录制设置"), form));

        connect(browseBtn, &QPushButton::clicked, this, &SettingsPage::browsePath);
    }

    {
        auto *form = new QWidget(panel);
        form->setAutoFillBackground(false);
        auto *layout = makeFormLayout();
        form->setLayout(layout);

        m_themeCombo = new PaintedComboBox(form);
        m_themeCombo->setFixedHeight(paint::Metrics::inputHeight);
        m_themeCombo->setMinimumWidth(paint::Metrics::inputMinWidth);
        m_themeCombo->addItem(QStringLiteral("跟随系统"), ThemeSystem);
        m_themeCombo->addItem(QStringLiteral("浅色模式"), ThemeLight);
        m_themeCombo->addItem(QStringLiteral("深色模式"), ThemeDark);
        m_themeCombo->setToolTip(QStringLiteral("选择应用的外观主题，可跟随 Windows 系统设置"));
        m_themeCombo->installEventFilter(this);
        layout->addRow(QStringLiteral("主题"), m_themeCombo);

        m_confirmStopCheck = new PaintedCheckBox(QStringLiteral("停止时确认"), form);
        m_confirmStopCheck->setFixedHeight(paint::Metrics::inputHeight);
        m_confirmStopCheck->setToolTip(QStringLiteral("启用后，停止录制时会弹出确认对话框"));
        layout->addRow(QStringLiteral("停止确认"), m_confirmStopCheck);

        panelLayout->addWidget(createSection(QStringLiteral("界面设置"), form));
    }

    {
        auto *form = new QWidget(panel);
        form->setAutoFillBackground(false);
        auto *layout = makeFormLayout();
        form->setLayout(layout);

        m_startTimeoutSpin = new PaintedSpinBox(form);
        m_startTimeoutSpin->setFixedHeight(paint::Metrics::inputHeight);
        m_startTimeoutSpin->setMinimumWidth(paint::Metrics::inputMinWidth);
        m_startTimeoutSpin->setRange(1, 30);
        m_startTimeoutSpin->setSuffix(QStringLiteral(" 秒"));
        m_startTimeoutSpin->setToolTip(QStringLiteral("录制程序启动等待时间，超时则取消录制"));
        m_startTimeoutSpin->installEventFilter(this);
        layout->addRow(QStringLiteral("启动超时"), m_startTimeoutSpin);

        m_stopTimeoutSpin = new PaintedSpinBox(form);
        m_stopTimeoutSpin->setFixedHeight(paint::Metrics::inputHeight);
        m_stopTimeoutSpin->setMinimumWidth(paint::Metrics::inputMinWidth);
        m_stopTimeoutSpin->setRange(1, 30);
        m_stopTimeoutSpin->setSuffix(QStringLiteral(" 秒"));
        m_stopTimeoutSpin->setToolTip(QStringLiteral("录制程序停止等待时间，超时将强制终止"));
        m_stopTimeoutSpin->installEventFilter(this);
        layout->addRow(QStringLiteral("停止超时"), m_stopTimeoutSpin);

        panelLayout->addWidget(createSection(QStringLiteral("高级设置"), form));
    }

    panelLayout->addSpacing(8);
    auto *fbContainer = new QWidget(panel);
    fbContainer->setAutoFillBackground(false);
    fbContainer->setFixedHeight(36);
    auto *fbLayout = new QVBoxLayout(fbContainer);
    fbLayout->setContentsMargins(0, 0, 0, 0);
    m_saveFeedback = new SaveFeedbackLabel(fbContainer);
    m_saveFeedback->setVisible(false);
    fbLayout->addWidget(m_saveFeedback);
    panelLayout->addWidget(fbContainer);

    m_feedbackTimer = new QTimer(this);
    m_feedbackTimer->setSingleShot(true);
    connect(m_feedbackTimer, &QTimer::timeout, m_saveFeedback, &QLabel::hide);

    m_applyDebounce = new QTimer(this);
    m_applyDebounce->setSingleShot(true);
    m_applyDebounce->setInterval(300);
    connect(m_applyDebounce, &QTimer::timeout, this, [this] { applySettings(true); });

    auto *btnRow = new QWidget(panel);
    btnRow->setAutoFillBackground(false);
    auto *btnLayout = new QHBoxLayout(btnRow);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(12);

    m_resetBtn = new ActionButton(QStringLiteral("恢复默认"), ActionButton::Reset, panel);

    btnLayout->addStretch();
    btnLayout->addWidget(m_resetBtn);
    panelLayout->addWidget(btnRow);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scrollArea->setAutoFillBackground(false);
    scrollArea->setAttribute(Qt::WA_StyledBackground, false);
    scrollArea->setAttribute(Qt::WA_TranslucentBackground);
    scrollArea->viewport()->setAutoFillBackground(false);
    scrollArea->viewport()->setAttribute(Qt::WA_StyledBackground, false);
    scrollArea->viewport()->setAttribute(Qt::WA_TranslucentBackground);
    QPalette scrollPalette = scrollArea->palette();
    scrollPalette.setColor(QPalette::Window, Qt::transparent);
    scrollPalette.setColor(QPalette::Base, Qt::transparent);
    scrollArea->setPalette(scrollPalette);
    scrollArea->viewport()->setPalette(scrollPalette);
    scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea, QScrollArea QWidget, QScrollArea QViewport {"
        " background: transparent;"
        " border: none;"
        "}"));
    auto *scrollBar = new PaintedScrollBar(scrollArea);
    scrollBar->setFixedWidth(10);
    scrollArea->setVerticalScrollBar(scrollBar);

    auto *scrollContent = new QWidget(scrollArea);
    scrollContent->setAutoFillBackground(false);
    scrollContent->setAttribute(Qt::WA_StyledBackground, false);
    scrollContent->setAttribute(Qt::WA_TranslucentBackground);
    scrollContent->setPalette(scrollPalette);
    auto *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(0);
    scrollLayout->addWidget(panel);
    scrollLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    root->addWidget(scrollArea, 1);

    auto scheduleApply = [this] { m_applyDebounce->start(); };
    connect(m_fpsSpin, qOverload<int>(&QSpinBox::valueChanged), this, scheduleApply);
    connect(m_qualityCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, scheduleApply);
    connect(m_themeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, scheduleApply);
    connect(m_confirmStopCheck, &QCheckBox::toggled, this, scheduleApply);
    connect(m_showCursorCheck, &QCheckBox::toggled, this, scheduleApply);
    connect(m_startTimeoutSpin, qOverload<int>(&QSpinBox::valueChanged), this, scheduleApply);
    connect(m_stopTimeoutSpin, qOverload<int>(&QSpinBox::valueChanged), this, scheduleApply);
    connect(m_pathEdit, &QLineEdit::editingFinished, this, [this] { applySettings(true); });

    connect(m_resetBtn, &QPushButton::clicked, this, &SettingsPage::resetDefaults);
}

QFrame *SettingsPage::createSection(const QString &title, QWidget *form)
{
    auto *section = new QFrame(this);
    section->setAutoFillBackground(false);
    auto *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto *titleLabel = new QLabel(title, section);
    QFont titleFont = titleLabel->font();
    titleFont.setPixelSize(15);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    m_sectionTitles.append(titleLabel);

    layout->addWidget(titleLabel);

    auto *separator = new SettingsSeparator(section);
    layout->addWidget(separator);

    layout->addWidget(form);

    return section;
}

bool SettingsPage::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Wheel
        && (qobject_cast<QSpinBox *>(watched) || qobject_cast<QComboBox *>(watched))) {
        event->ignore();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void SettingsPage::blockSignalsAll(bool block)
{
    m_pathEdit->blockSignals(block);
    m_fpsSpin->blockSignals(block);
    m_qualityCombo->blockSignals(block);
    m_themeCombo->blockSignals(block);
    m_confirmStopCheck->blockSignals(block);
    m_showCursorCheck->blockSignals(block);
    m_startTimeoutSpin->blockSignals(block);
    m_stopTimeoutSpin->blockSignals(block);
}

void SettingsPage::loadSettings()
{
    blockSignalsAll(true);

    m_lastValidPath = m_settings->value(kSettingsPrefix + QLatin1String("savePath"),
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + QStringLiteral("/BlueCap")).toString();
    m_pathEdit->setText(m_lastValidPath);

    m_fpsSpin->setValue(m_settings->value(kSettingsPrefix + QLatin1String("frameRate"), 30).toInt());

    const QString savedPreset = m_settings->value(
        kSettingsPrefix + QLatin1String("preset"), QStringLiteral("fast")).toString();
    int idx = m_qualityCombo->findData(savedPreset);
    if (idx >= 0) m_qualityCombo->setCurrentIndex(idx);

    int themeIdx = m_themeCombo->findData(m_settings->value(kSettingsPrefix + QLatin1String("theme"), ThemeSystem).toInt());
    if (themeIdx >= 0) m_themeCombo->setCurrentIndex(themeIdx);

    m_confirmStopCheck->setChecked(m_settings->value(kSettingsPrefix + QLatin1String("confirmStop"), false).toBool());
    m_showCursorCheck->setChecked(m_settings->value(kSettingsPrefix + QLatin1String("showCursor"), true).toBool());
    m_startTimeoutSpin->setValue(m_settings->value(kSettingsPrefix + QLatin1String("startTimeout"), 5).toInt());
    m_stopTimeoutSpin->setValue(m_settings->value(kSettingsPrefix + QLatin1String("stopTimeout"), 5).toInt());

    blockSignalsAll(false);

    emit frameRateChanged(m_fpsSpin->value());
    emit presetChanged(m_qualityCombo->currentData().toString());
    emit savePathChanged(m_pathEdit->text().trimmed());
    emit confirmStopChanged(m_confirmStopCheck->isChecked());
    emit showCursorChanged(m_showCursorCheck->isChecked());
    emit startTimeoutChanged(m_startTimeoutSpin->value() * 1000);
    emit stopTimeoutChanged(m_stopTimeoutSpin->value() * 1000);
    emit themeChanged(m_themeCombo->currentData().toInt());
}

void SettingsPage::setDarkMode(bool dark)
{
    m_darkMode = dark;
    const auto &a = ThemeColors::forMode(dark).app;

    for (auto *w : findChildren<QWidget *>()) {
        if (auto *h = qobject_cast<PageHeader *>(w)) { h->setDarkMode(dark); }
        else if (auto *pp = qobject_cast<SettingsPanel *>(w)) { pp->setDarkMode(dark); }
        else if (auto *ss = qobject_cast<SettingsSeparator *>(w)) { ss->setDarkMode(dark); }
        else if (auto *sf = qobject_cast<SaveFeedbackLabel *>(w)) { sf->setDarkMode(dark); }
        else if (auto *ab = qobject_cast<ActionButton *>(w)) { ab->setDarkMode(dark); }
        else if (auto *le = qobject_cast<PaintedLineEdit *>(w)) { le->setDarkMode(dark); }
        else if (auto *cb = qobject_cast<PaintedComboBox *>(w)) { cb->setDarkMode(dark); }
        else if (auto *sb = qobject_cast<PaintedSpinBox *>(w)) { sb->setDarkMode(dark); }
        else if (auto *ck = qobject_cast<PaintedCheckBox *>(w)) { ck->setDarkMode(dark); }
        else if (auto *sc = qobject_cast<PaintedScrollBar *>(w)) { sc->setDarkMode(dark); }
    }

    for (auto *label : findChildren<QLabel *>()) {
        if (qobject_cast<PageHeader *>(label) || qobject_cast<SaveFeedbackLabel *>(label))
            continue;
        QPalette p = label->palette();
        p.setColor(QPalette::WindowText,
                   m_sectionTitles.contains(label) ? a.settingsSectionTitle : a.settingsFormLabel);
        label->setPalette(p);
    }
}

void SettingsPage::browsePath()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        QStringLiteral("选择保存路径"), m_pathEdit->text());
    if (!dir.isEmpty()) {
        m_pathEdit->setText(dir);
        applySettings(true);
    }
}

void SettingsPage::resetDefaults()
{
    m_resetting = true;

    blockSignalsAll(true);
    m_pathEdit->setText(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + QStringLiteral("/BlueCap"));
    m_fpsSpin->setValue(30);
    int idx = m_qualityCombo->findData(QStringLiteral("fast"));
    if (idx >= 0) m_qualityCombo->setCurrentIndex(idx);
    m_confirmStopCheck->setChecked(false);
    m_showCursorCheck->setChecked(true);
    m_startTimeoutSpin->setValue(5);
    m_stopTimeoutSpin->setValue(5);
    int themeIdx = m_themeCombo->findData(ThemeSystem);
    if (themeIdx >= 0) m_themeCombo->setCurrentIndex(themeIdx);
    blockSignalsAll(false);

    applySettings(true);
}

void SettingsPage::applySettings(bool showFeedback)
{
    m_settings->setValue(kSettingsPrefix + QLatin1String("frameRate"), m_fpsSpin->value());
    m_settings->setValue(kSettingsPrefix + QLatin1String("preset"), m_qualityCombo->currentData().toString());
    m_settings->setValue(kSettingsPrefix + QLatin1String("confirmStop"), m_confirmStopCheck->isChecked());
    m_settings->setValue(kSettingsPrefix + QLatin1String("showCursor"), m_showCursorCheck->isChecked());
    m_settings->setValue(kSettingsPrefix + QLatin1String("startTimeout"), m_startTimeoutSpin->value());
    m_settings->setValue(kSettingsPrefix + QLatin1String("stopTimeout"), m_stopTimeoutSpin->value());
    m_settings->setValue(kSettingsPrefix + QLatin1String("theme"), m_themeCombo->currentData().toInt());

    emit frameRateChanged(m_fpsSpin->value());
    emit presetChanged(m_qualityCombo->currentData().toString());
    emit themeChanged(m_themeCombo->currentData().toInt());
    emit confirmStopChanged(m_confirmStopCheck->isChecked());
    emit showCursorChanged(m_showCursorCheck->isChecked());
    emit startTimeoutChanged(m_startTimeoutSpin->value() * 1000);
    emit stopTimeoutChanged(m_stopTimeoutSpin->value() * 1000);

    QString path = m_pathEdit->text().trimmed();
    bool pathValid = true;

    if (path.isEmpty()) {
        m_saveFeedback->setText(QStringLiteral("保存路径不能为空"));
        pathValid = false;
    } else {
        QDir dir(path);
        if (!dir.exists()) {
            if (!dir.mkpath(QStringLiteral("."))) {
                m_saveFeedback->setText(QStringLiteral("无法创建目录，请检查权限"));
                pathValid = false;
            }
        } else if (!QFileInfo(path).isWritable()) {
            m_saveFeedback->setText(QStringLiteral("路径不可写，请选择其他路径"));
            pathValid = false;
        }
    }

    if (pathValid) {
        m_lastValidPath = path;
        m_settings->setValue(kSettingsPrefix + QLatin1String("savePath"), path);
        emit savePathChanged(path);
    } else {
        m_pathEdit->setText(m_lastValidPath);
    }

    if (showFeedback) {
        m_saveFeedback->setSuccess(pathValid);
        m_saveFeedback->setText(pathValid
            ? (m_resetting ? QStringLiteral("已恢复默认设置") : QStringLiteral("已保存"))
            : m_saveFeedback->text());
        m_saveFeedback->setVisible(true);
        m_feedbackTimer->start(pathValid ? 2000 : 3000);
        m_resetting = false;
    }
}
