#include "SettingsPage.h"

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
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>

namespace {
const QLatin1String kSettingsPrefix("settings/");
}

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("settingsPage"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 22, 30, 24);
    root->setSpacing(18);

    auto *header = new QLabel(QStringLiteral("设置"));
    header->setObjectName(QStringLiteral("pageHeader"));
    root->addWidget(header);

    auto *panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("settingsPanel"));
    panel->setAttribute(Qt::WA_StyledBackground, true);
    panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    panel->setMaximumWidth(860);

    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(36, 28, 36, 28);
    panelLayout->setSpacing(22);

    const QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
        + QStringLiteral("/BlueCap");

    // ── Section 1: 录制设置 ──
    {
        auto *form = new QWidget(panel);
        form->setObjectName(QStringLiteral("settingsForm"));
        auto *layout = new QFormLayout(form);
        layout->setHorizontalSpacing(22);
        layout->setVerticalSpacing(14);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        m_pathEdit = new QLineEdit(form);
        m_pathEdit->setFixedHeight(46);
        m_pathEdit->setMinimumWidth(360);
        m_pathEdit->setPlaceholderText(defaultPath);
        m_pathEdit->setToolTip(QStringLiteral("录制文件的保存位置，点击「浏览」选择文件夹"));
        auto *browseBtn = new QPushButton(QStringLiteral("浏览..."), form);
        browseBtn->setObjectName(QStringLiteral("browseBtn"));
        browseBtn->setFixedSize(118, 46);
        browseBtn->setCursor(Qt::PointingHandCursor);
        auto *pathRow = new QWidget(form);
        auto *pathLayout = new QHBoxLayout(pathRow);
        pathLayout->setContentsMargins(0, 0, 0, 0);
        pathLayout->setSpacing(12);
        pathLayout->addWidget(m_pathEdit, 1);
        pathLayout->addWidget(browseBtn, 0, Qt::AlignRight);
        layout->addRow(QStringLiteral("保存路径"), pathRow);

        m_fpsSpin = new QSpinBox(form);
        m_fpsSpin->setFixedHeight(46);
        m_fpsSpin->setMinimumWidth(360);
        m_fpsSpin->setRange(15, 60);
        m_fpsSpin->setSuffix(QStringLiteral(" fps"));
        m_fpsSpin->setToolTip(QStringLiteral("帧率越高，视频越流畅但文件越大。屏幕录制推荐 30 fps。"));
        m_fpsSpin->installEventFilter(this);
        layout->addRow(QStringLiteral("帧率"), m_fpsSpin);

        m_qualityCombo = new QComboBox(form);
        m_qualityCombo->setFixedHeight(46);
        m_qualityCombo->setMinimumWidth(360);
        m_qualityCombo->setMaxVisibleItems(7);
        m_qualityCombo->addItem(QStringLiteral("ultrafast (低画质)"), QStringLiteral("ultrafast"));
        m_qualityCombo->addItem(QStringLiteral("superfast"), QStringLiteral("superfast"));
        m_qualityCombo->addItem(QStringLiteral("veryfast"), QStringLiteral("veryfast"));
        m_qualityCombo->addItem(QStringLiteral("faster"), QStringLiteral("faster"));
        m_qualityCombo->addItem(QStringLiteral("fast (推荐)"), QStringLiteral("fast"));
        m_qualityCombo->addItem(QStringLiteral("medium"), QStringLiteral("medium"));
        m_qualityCombo->addItem(QStringLiteral("slow (高画质)"), QStringLiteral("slow"));
        m_qualityCombo->setToolTip(QStringLiteral("画质越高，视频文件越大。推荐选择「fast」获得较好的平衡。"));
        m_qualityCombo->installEventFilter(this);
        layout->addRow(QStringLiteral("画质"), m_qualityCombo);

        m_showCursorCheck = new QCheckBox(QStringLiteral("录制时显示"), form);
        m_showCursorCheck->setFixedHeight(46);
        m_showCursorCheck->setToolTip(QStringLiteral("录制时是否包含鼠标光标"));
        layout->addRow(QStringLiteral("显示光标"), m_showCursorCheck);

        panelLayout->addWidget(createSection(QStringLiteral("录制设置"), form));

        connect(browseBtn, &QPushButton::clicked, this, &SettingsPage::browsePath);
    }

    // ── Section 2: 界面设置 ──
    {
        auto *form = new QWidget(panel);
        form->setObjectName(QStringLiteral("settingsForm"));
        auto *layout = new QFormLayout(form);
        layout->setHorizontalSpacing(22);
        layout->setVerticalSpacing(14);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        m_themeCombo = new QComboBox(form);
        m_themeCombo->setFixedHeight(46);
        m_themeCombo->setMinimumWidth(360);
        m_themeCombo->addItem(QStringLiteral("跟随系统"), ThemeSystem);
        m_themeCombo->addItem(QStringLiteral("浅色模式"), ThemeLight);
        m_themeCombo->addItem(QStringLiteral("深色模式"), ThemeDark);
        m_themeCombo->setToolTip(QStringLiteral("选择应用的外观主题，可跟随 Windows 系统设置"));
        m_themeCombo->installEventFilter(this);
        layout->addRow(QStringLiteral("主题"), m_themeCombo);

        m_confirmStopCheck = new QCheckBox(QStringLiteral("停止时确认"), form);
        m_confirmStopCheck->setFixedHeight(46);
        m_confirmStopCheck->setToolTip(QStringLiteral("启用后，停止录制时会弹出确认对话框"));
        layout->addRow(QStringLiteral("停止确认"), m_confirmStopCheck);

        panelLayout->addWidget(createSection(QStringLiteral("界面设置"), form));
    }

    // ── Section 3: 高级设置 ──
    {
        auto *form = new QWidget(panel);
        form->setObjectName(QStringLiteral("settingsForm"));
        auto *layout = new QFormLayout(form);
        layout->setHorizontalSpacing(22);
        layout->setVerticalSpacing(14);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        m_startTimeoutSpin = new QSpinBox(form);
        m_startTimeoutSpin->setFixedHeight(46);
        m_startTimeoutSpin->setMinimumWidth(360);
        m_startTimeoutSpin->setRange(1, 30);
        m_startTimeoutSpin->setSuffix(QStringLiteral(" 秒"));
        m_startTimeoutSpin->setToolTip(QStringLiteral("录制程序启动等待时间，超时则取消录制"));
        m_startTimeoutSpin->installEventFilter(this);
        layout->addRow(QStringLiteral("启动超时"), m_startTimeoutSpin);

        m_stopTimeoutSpin = new QSpinBox(form);
        m_stopTimeoutSpin->setFixedHeight(46);
        m_stopTimeoutSpin->setMinimumWidth(360);
        m_stopTimeoutSpin->setRange(1, 30);
        m_stopTimeoutSpin->setSuffix(QStringLiteral(" 秒"));
        m_stopTimeoutSpin->setToolTip(QStringLiteral("录制程序停止等待时间，超时将强制终止"));
        m_stopTimeoutSpin->installEventFilter(this);
        layout->addRow(QStringLiteral("停止超时"), m_stopTimeoutSpin);

        panelLayout->addWidget(createSection(QStringLiteral("高级设置"), form));
    }

    // ── Feedback & Footer ──
    panelLayout->addSpacing(8);

    m_saveFeedback = new QLabel(panel);
    m_saveFeedback->setObjectName(QStringLiteral("saveFeedback"));
    m_saveFeedback->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_saveFeedback->setVisible(false);
    panelLayout->addWidget(m_saveFeedback);

    m_feedbackTimer = new QTimer(this);
    m_feedbackTimer->setSingleShot(true);
    connect(m_feedbackTimer, &QTimer::timeout, m_saveFeedback, &QLabel::hide);

    m_applyDebounce = new QTimer(this);
    m_applyDebounce->setSingleShot(true);
    m_applyDebounce->setInterval(300);
    connect(m_applyDebounce, &QTimer::timeout, this, [this] { applySettings(true); });

    auto *btnRow = new QWidget(panel);
    btnRow->setObjectName(QStringLiteral("settingsFooter"));
    auto *btnLayout = new QHBoxLayout(btnRow);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(12);

    m_resetBtn = new QPushButton(QStringLiteral("恢复默认"), panel);
    m_resetBtn->setObjectName(QStringLiteral("resetBtn"));
    m_resetBtn->setFixedHeight(44);
    m_resetBtn->setCursor(Qt::PointingHandCursor);

    btnLayout->addStretch();
    btnLayout->addWidget(m_resetBtn);
    panelLayout->addWidget(btnRow);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setObjectName(QStringLiteral("settingsScrollArea"));
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scrollArea->viewport()->setAutoFillBackground(false);

    auto *scrollContent = new QWidget(scrollArea);
    scrollContent->setObjectName(QStringLiteral("settingsScrollContent"));
    auto *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(0);
    scrollLayout->addWidget(panel, 0, Qt::AlignHCenter | Qt::AlignTop);
    scrollLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    root->addWidget(scrollArea, 1);

    // Auto-save on any change (debounced)
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
    section->setObjectName(QStringLiteral("settingsSection"));
    auto *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto *titleLabel = new QLabel(title, section);
    titleLabel->setObjectName(QStringLiteral("settingsSectionTitle"));
    layout->addWidget(titleLabel);

    auto *separator = new QFrame(section);
    separator->setObjectName(QStringLiteral("settingsSeparator"));
    separator->setFixedHeight(1);
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
    QSettings s;

    blockSignalsAll(true);

    m_lastValidPath = s.value(kSettingsPrefix + QLatin1String("savePath"),
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + QStringLiteral("/BlueCap")).toString();
    m_pathEdit->setText(m_lastValidPath);

    m_fpsSpin->setValue(s.value(kSettingsPrefix + QLatin1String("frameRate"), 30).toInt());

    const QString savedPreset = s.value(
        kSettingsPrefix + QLatin1String("preset"), QStringLiteral("fast")).toString();
    int idx = m_qualityCombo->findData(savedPreset);
    if (idx >= 0) m_qualityCombo->setCurrentIndex(idx);

    int themeIdx = m_themeCombo->findData(s.value(kSettingsPrefix + QLatin1String("theme"), ThemeSystem).toInt());
    if (themeIdx >= 0) m_themeCombo->setCurrentIndex(themeIdx);

    m_confirmStopCheck->setChecked(s.value(kSettingsPrefix + QLatin1String("confirmStop"), false).toBool());
    m_showCursorCheck->setChecked(s.value(kSettingsPrefix + QLatin1String("showCursor"), true).toBool());
    m_startTimeoutSpin->setValue(s.value(kSettingsPrefix + QLatin1String("startTimeout"), 5).toInt());
    m_stopTimeoutSpin->setValue(s.value(kSettingsPrefix + QLatin1String("stopTimeout"), 5).toInt());

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
    QSettings settings;

    settings.setValue(kSettingsPrefix + QLatin1String("frameRate"), m_fpsSpin->value());
    settings.setValue(kSettingsPrefix + QLatin1String("preset"), m_qualityCombo->currentData().toString());
    settings.setValue(kSettingsPrefix + QLatin1String("confirmStop"), m_confirmStopCheck->isChecked());
    settings.setValue(kSettingsPrefix + QLatin1String("showCursor"), m_showCursorCheck->isChecked());
    settings.setValue(kSettingsPrefix + QLatin1String("startTimeout"), m_startTimeoutSpin->value());
    settings.setValue(kSettingsPrefix + QLatin1String("stopTimeout"), m_stopTimeoutSpin->value());
    settings.setValue(kSettingsPrefix + QLatin1String("theme"), m_themeCombo->currentData().toInt());

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
        settings.setValue(kSettingsPrefix + QLatin1String("savePath"), path);
        emit savePathChanged(path);
    } else {
        m_pathEdit->setText(m_lastValidPath);
    }

    if (showFeedback) {
        if (pathValid) {
            m_saveFeedback->setStyleSheet(QString());
            m_saveFeedback->setProperty("success", true);
        } else {
            m_saveFeedback->setProperty("success", false);
        }
        m_saveFeedback->style()->unpolish(m_saveFeedback);
        m_saveFeedback->style()->polish(m_saveFeedback);
        m_saveFeedback->setText(pathValid
            ? (m_resetting ? QStringLiteral("已恢复默认设置") : QStringLiteral("已保存"))
            : m_saveFeedback->text());
        m_saveFeedback->setVisible(true);
        m_feedbackTimer->start(pathValid ? 2000 : 3000);
        m_resetting = false;
    }
}
