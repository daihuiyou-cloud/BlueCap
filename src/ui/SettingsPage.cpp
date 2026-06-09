#include "SettingsPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
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
    root->setContentsMargins(18, 14, 18, 14);
    root->setSpacing(14);

    auto *header = new QLabel(QStringLiteral("设置"));
    header->setObjectName(QStringLiteral("pageHeader"));
    root->addWidget(header);

    auto *panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("settingsPanel"));
    panel->setAttribute(Qt::WA_StyledBackground, true);
    panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(24, 22, 24, 20);
    panelLayout->setSpacing(14);

    auto *sectionTitle = new QLabel(QStringLiteral("录制参数"), panel);
    sectionTitle->setObjectName(QStringLiteral("settingsSectionTitle"));
    panelLayout->addWidget(sectionTitle);

    auto *form = new QWidget(panel);
    form->setObjectName(QStringLiteral("settingsForm"));
    auto *layout = new QFormLayout(form);
    layout->setHorizontalSpacing(16);
    layout->setVerticalSpacing(10);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    const QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
        + QStringLiteral("/BlueCap");
    m_pathEdit = new QLineEdit(form);
    m_pathEdit->setFixedHeight(42);
    m_pathEdit->setMinimumWidth(360);
    m_pathEdit->setToolTip(QStringLiteral("录制文件的保存位置，点击「浏览」选择文件夹"));
    auto *browseBtn = new QPushButton(QStringLiteral("浏览..."), form);
    browseBtn->setObjectName(QStringLiteral("browseBtn"));
    browseBtn->setFixedSize(112, 42);
    browseBtn->setCursor(Qt::PointingHandCursor);
    auto *pathRow = new QWidget(form);
    auto *pathLayout = new QHBoxLayout(pathRow);
    pathLayout->setContentsMargins(0, 0, 0, 0);
    pathLayout->setSpacing(10);
    pathLayout->addWidget(m_pathEdit, 1);
    pathLayout->addWidget(browseBtn, 0, Qt::AlignRight);
    layout->addRow(QStringLiteral("保存路径"), pathRow);

    m_fpsSpin = new QSpinBox(form);
    m_fpsSpin->setFixedHeight(42);
    m_fpsSpin->setMinimumWidth(360);
    m_fpsSpin->setRange(15, 60);
    m_fpsSpin->setSuffix(QStringLiteral(" fps"));
    m_fpsSpin->setToolTip(QStringLiteral("帧率越高，视频越流畅但文件越大。屏幕录制推荐 30 fps。"));
    m_fpsSpin->setFocusPolicy(Qt::StrongFocus);
    layout->addRow(QStringLiteral("帧率"), m_fpsSpin);

    m_qualityCombo = new QComboBox(form);
    m_qualityCombo->setFixedHeight(42);
    m_qualityCombo->setMinimumWidth(360);
    m_qualityCombo->addItem(QStringLiteral(" ultrafast (低画质)"), QStringLiteral("ultrafast"));
    m_qualityCombo->addItem(QStringLiteral(" superfast"), QStringLiteral("superfast"));
    m_qualityCombo->addItem(QStringLiteral(" veryfast"), QStringLiteral("veryfast"));
    m_qualityCombo->addItem(QStringLiteral(" faster"), QStringLiteral("faster"));
    m_qualityCombo->addItem(QStringLiteral(" fast (推荐)"), QStringLiteral("fast"));
    m_qualityCombo->addItem(QStringLiteral(" medium"), QStringLiteral("medium"));
    m_qualityCombo->addItem(QStringLiteral(" slow (高画质)"), QStringLiteral("slow"));
    m_qualityCombo->setToolTip(QStringLiteral("画质越高，视频文件越大。推荐选择「fast」获得较好的平衡。"));
    m_qualityCombo->setFocusPolicy(Qt::StrongFocus);
    layout->addRow(QStringLiteral("画质"), m_qualityCombo);

    m_themeCombo = new QComboBox(form);
    m_themeCombo->setFixedHeight(42);
    m_themeCombo->setMinimumWidth(360);
    m_themeCombo->addItem(QStringLiteral("跟随系统"), ThemeSystem);
    m_themeCombo->addItem(QStringLiteral("浅色模式"), ThemeLight);
    m_themeCombo->addItem(QStringLiteral("深色模式"), ThemeDark);
    m_themeCombo->setToolTip(QStringLiteral("选择应用的外观主题，可跟随 Windows 系统设置"));
    m_themeCombo->setFocusPolicy(Qt::StrongFocus);
    layout->addRow(QStringLiteral("主题"), m_themeCombo);

    m_confirmStopCheck = new QCheckBox(QStringLiteral("停止时确认"), form);
    m_confirmStopCheck->setFixedHeight(42);
    m_confirmStopCheck->setToolTip(QStringLiteral("启用后，停止录制时会弹出确认对话框"));
    layout->addRow(QStringLiteral("停止确认"), m_confirmStopCheck);

    m_showCursorCheck = new QCheckBox(QStringLiteral("录制时显示"), form);
    m_showCursorCheck->setFixedHeight(42);
    m_showCursorCheck->setToolTip(QStringLiteral("录制时是否包含鼠标光标"));
    layout->addRow(QStringLiteral("显示光标"), m_showCursorCheck);

    m_startTimeoutSpin = new QSpinBox(form);
    m_startTimeoutSpin->setFixedHeight(42);
    m_startTimeoutSpin->setMinimumWidth(360);
    m_startTimeoutSpin->setRange(1, 30);
    m_startTimeoutSpin->setSuffix(QStringLiteral(" 秒"));
    m_startTimeoutSpin->setToolTip(QStringLiteral("录制程序启动等待时间，超时则取消录制"));
    m_startTimeoutSpin->setFocusPolicy(Qt::StrongFocus);
    layout->addRow(QStringLiteral("启动超时"), m_startTimeoutSpin);

    m_stopTimeoutSpin = new QSpinBox(form);
    m_stopTimeoutSpin->setFixedHeight(42);
    m_stopTimeoutSpin->setMinimumWidth(360);
    m_stopTimeoutSpin->setRange(1, 30);
    m_stopTimeoutSpin->setSuffix(QStringLiteral(" 秒"));
    m_stopTimeoutSpin->setToolTip(QStringLiteral("录制程序停止等待时间，超时将强制终止"));
    m_stopTimeoutSpin->setFocusPolicy(Qt::StrongFocus);
    layout->addRow(QStringLiteral("停止超时"), m_stopTimeoutSpin);

    panelLayout->addWidget(form);
    panelLayout->addSpacing(2);

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
    m_resetBtn->setFixedHeight(42);
    m_resetBtn->setCursor(Qt::PointingHandCursor);

    btnLayout->addStretch();
    btnLayout->addWidget(m_resetBtn);
    panelLayout->addWidget(btnRow);

    root->addWidget(panel, 0, Qt::AlignTop);
    root->addStretch();

    // Auto-save on any change (debounced to batch registry writes)
    auto scheduleApply = [this] { m_applyDebounce->start(); };
    connect(m_fpsSpin, qOverload<int>(&QSpinBox::valueChanged), this, scheduleApply);
    connect(m_qualityCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, scheduleApply);
    connect(m_themeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, scheduleApply);
    connect(m_confirmStopCheck, &QCheckBox::toggled, this, scheduleApply);
    connect(m_showCursorCheck, &QCheckBox::toggled, this, scheduleApply);
    connect(m_startTimeoutSpin, qOverload<int>(&QSpinBox::valueChanged), this, scheduleApply);
    connect(m_stopTimeoutSpin, qOverload<int>(&QSpinBox::valueChanged), this, scheduleApply);
    connect(m_pathEdit, &QLineEdit::editingFinished, this, [this] { applySettings(true); });

    connect(browseBtn, &QPushButton::clicked, this, &SettingsPage::browsePath);
    connect(m_resetBtn, &QPushButton::clicked, this, &SettingsPage::resetDefaults);
}

void SettingsPage::loadSettings()
{
    QSettings s;

    // Block signals to prevent debounced save from triggering during initial load
    m_pathEdit->blockSignals(true);
    m_fpsSpin->blockSignals(true);
    m_qualityCombo->blockSignals(true);
    m_themeCombo->blockSignals(true);
    m_confirmStopCheck->blockSignals(true);
    m_showCursorCheck->blockSignals(true);
    m_startTimeoutSpin->blockSignals(true);
    m_stopTimeoutSpin->blockSignals(true);

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

    m_pathEdit->blockSignals(false);
    m_fpsSpin->blockSignals(false);
    m_qualityCombo->blockSignals(false);
    m_themeCombo->blockSignals(false);
    m_confirmStopCheck->blockSignals(false);
    m_showCursorCheck->blockSignals(false);
    m_startTimeoutSpin->blockSignals(false);
    m_stopTimeoutSpin->blockSignals(false);

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

    applySettings(true);
}

void SettingsPage::applySettings(bool showFeedback)
{
    QSettings settings;

    // Always save non-path settings
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

    // Validate and save path separately
    QString path = m_pathEdit->text().trimmed();
    bool pathValid = true;

    if (path.isEmpty()) {
        showPathError(QStringLiteral("保存路径不能为空"));
        pathValid = false;
    } else {
        QDir dir(path);
        if (!dir.exists()) {
            if (!dir.mkpath(QStringLiteral("."))) {
                showPathError(QStringLiteral("无法创建目录，请检查权限"));
                pathValid = false;
            }
        } else if (!QFileInfo(path).isWritable()) {
            showPathError(QStringLiteral("路径不可写，请选择其他路径"));
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
            m_saveFeedback->setStyleSheet(QStringLiteral("color: #28965c; font-size: 13px; font-weight: 700; padding: 4px 8px;"));
            m_saveFeedback->setText(m_resetting ? QStringLiteral("✓ 已恢复默认设置") : QStringLiteral("✓ 已保存"));
            m_saveFeedback->setVisible(true);
            m_feedbackTimer->start(2000);
        }
        m_resetting = false;
    }
}

void SettingsPage::showPathError(const QString &msg)
{
    m_saveFeedback->setStyleSheet(QStringLiteral("color: #e0525e; font-size: 13px; font-weight: 700; padding: 4px 8px;"));
    m_saveFeedback->setText(msg);
    m_saveFeedback->setVisible(true);
    m_feedbackTimer->start(3000);
}
