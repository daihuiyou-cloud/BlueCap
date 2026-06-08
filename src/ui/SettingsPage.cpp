#include "SettingsPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    qRegisterMetaTypeStreamOperators<QStringList>("QStringList");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 12);
    root->setSpacing(8);

    auto *header = new QLabel(QStringLiteral("设置"));
    header->setObjectName(QStringLiteral("pageHeader"));
    root->addWidget(header);

    auto *form = new QWidget(this);
    auto *layout = new QFormLayout(form);
    layout->setSpacing(12);
    layout->setContentsMargins(0, 0, 0, 0);

    const QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
        + QStringLiteral("/BlueCap");
    m_pathEdit = new QLineEdit(form);
    m_pathEdit->setToolTip(QStringLiteral("录制文件的保存位置，点击「浏览」选择文件夹"));
    auto *browseBtn = new QPushButton(QStringLiteral("浏览..."), form);
    browseBtn->setFixedWidth(80);
    auto *pathRow = new QWidget(form);
    auto *pathLayout = new QHBoxLayout(pathRow);
    pathLayout->setContentsMargins(0, 0, 0, 0);
    pathLayout->addWidget(m_pathEdit, 1);
    pathLayout->addWidget(browseBtn);
    layout->addRow(QStringLiteral("保存路径"), pathRow);

    m_fpsSpin = new QSpinBox(form);
    m_fpsSpin->setRange(15, 60);
    m_fpsSpin->setSuffix(QStringLiteral(" fps"));
    m_fpsSpin->setToolTip(QStringLiteral("帧率越高，视频越流畅但文件越大。屏幕录制推荐 30 fps。"));
    layout->addRow(QStringLiteral("帧率"), m_fpsSpin);

    m_qualityCombo = new QComboBox(form);
    m_qualityCombo->addItem(QStringLiteral(" ultrafast (低画质)"), QStringLiteral("ultrafast"));
    m_qualityCombo->addItem(QStringLiteral(" superfast"), QStringLiteral("superfast"));
    m_qualityCombo->addItem(QStringLiteral(" veryfast"), QStringLiteral("veryfast"));
    m_qualityCombo->addItem(QStringLiteral(" faster"), QStringLiteral("faster"));
    m_qualityCombo->addItem(QStringLiteral(" fast (推荐)"), QStringLiteral("fast"));
    m_qualityCombo->addItem(QStringLiteral(" medium"), QStringLiteral("medium"));
    m_qualityCombo->addItem(QStringLiteral(" slow (高画质)"), QStringLiteral("slow"));
    m_qualityCombo->setToolTip(QStringLiteral("画质越高，视频文件越大。推荐选择「fast」获得较好的平衡。"));
    layout->addRow(QStringLiteral("画质"), m_qualityCombo);

    m_confirmStopCheck = new QCheckBox(QStringLiteral("停止录制时确认"), form);
    m_confirmStopCheck->setToolTip(QStringLiteral("启用后，停止录制时会弹出确认对话框"));
    layout->addRow(QStringLiteral(""), m_confirmStopCheck);

    m_showCursorCheck = new QCheckBox(QStringLiteral("录制中显示鼠标"), form);
    m_showCursorCheck->setToolTip(QStringLiteral("录制时是否包含鼠标光标"));
    layout->addRow(QStringLiteral(""), m_showCursorCheck);

    m_startTimeoutSpin = new QSpinBox(form);
    m_startTimeoutSpin->setRange(1, 30);
    m_startTimeoutSpin->setSuffix(QStringLiteral(" 秒"));
    m_startTimeoutSpin->setToolTip(QStringLiteral("录制程序启动等待时间，超时则取消录制"));
    layout->addRow(QStringLiteral("启动超时"), m_startTimeoutSpin);

    m_stopTimeoutSpin = new QSpinBox(form);
    m_stopTimeoutSpin->setRange(1, 30);
    m_stopTimeoutSpin->setSuffix(QStringLiteral(" 秒"));
    m_stopTimeoutSpin->setToolTip(QStringLiteral("录制程序停止等待时间，超时将强制终止"));
    layout->addRow(QStringLiteral("停止超时"), m_stopTimeoutSpin);

    root->addWidget(form);

    m_saveFeedback = new QLabel(this);
    m_saveFeedback->setObjectName(QStringLiteral("saveFeedback"));
    m_saveFeedback->setAlignment(Qt::AlignCenter);
    m_saveFeedback->setVisible(false);
    root->addWidget(m_saveFeedback);

    m_feedbackTimer = new QTimer(this);
    m_feedbackTimer->setSingleShot(true);
    connect(m_feedbackTimer, &QTimer::timeout, m_saveFeedback, &QLabel::hide);

    auto *btnRow = new QWidget(this);
    auto *btnLayout = new QHBoxLayout(btnRow);
    btnLayout->setContentsMargins(0, 8, 0, 0);
    btnLayout->setSpacing(12);

    m_resetBtn = new QPushButton(QStringLiteral("恢复默认"), this);
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    m_resetBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; border: 1px solid rgba(207, 216, 232, 0.95); "
        "border-radius: 6px; padding: 8px 20px; font-size: 13px; color: #53617a; } "
        "QPushButton:hover { background: rgba(232, 242, 255, 0.66); }"));

    m_applyBtn = new QPushButton(QStringLiteral("应用"), this);
    m_applyBtn->setCursor(Qt::PointingHandCursor);
    m_applyBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #0967f2; border: none; border-radius: 6px; "
        "padding: 8px 28px; font-size: 13px; font-weight: 700; color: #ffffff; } "
        "QPushButton:hover { background: #075ce0; }"));

    btnLayout->addStretch();
    btnLayout->addWidget(m_resetBtn);
    btnLayout->addWidget(m_applyBtn);
    root->addWidget(btnRow);

    root->addStretch();

    loadSettings();

    connect(browseBtn, &QPushButton::clicked, this, &SettingsPage::browsePath);
    connect(m_applyBtn, &QPushButton::clicked, this, &SettingsPage::applySettings);
    connect(m_resetBtn, &QPushButton::clicked, this, &SettingsPage::resetDefaults);
}

void SettingsPage::loadSettings()
{
    QSettings s;

    m_pathEdit->setText(s.value(QStringLiteral("settings/savePath"),
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + QStringLiteral("/BlueCap")).toString());

    m_fpsSpin->setValue(s.value(QStringLiteral("settings/frameRate"), 30).toInt());

    const QString savedPreset = s.value(
        QStringLiteral("settings/preset"), QStringLiteral("fast")).toString();
    int idx = m_qualityCombo->findData(savedPreset);
    if (idx >= 0) m_qualityCombo->setCurrentIndex(idx);

    m_confirmStopCheck->setChecked(s.value(QStringLiteral("settings/confirmStop"), false).toBool());
    m_showCursorCheck->setChecked(s.value(QStringLiteral("settings/showCursor"), true).toBool());
    m_startTimeoutSpin->setValue(s.value(QStringLiteral("settings/startTimeout"), 5).toInt());
    m_stopTimeoutSpin->setValue(s.value(QStringLiteral("settings/stopTimeout"), 5).toInt());
}

void SettingsPage::browsePath()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        QStringLiteral("选择保存路径"), m_pathEdit->text());
    if (!dir.isEmpty()) {
        m_pathEdit->setText(dir);
    }
}

void SettingsPage::resetDefaults()
{
    m_pathEdit->setText(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + QStringLiteral("/BlueCap"));
    m_fpsSpin->setValue(30);
    int idx = m_qualityCombo->findData(QStringLiteral("fast"));
    if (idx >= 0) m_qualityCombo->setCurrentIndex(idx);
    m_confirmStopCheck->setChecked(false);
    m_showCursorCheck->setChecked(true);
    m_startTimeoutSpin->setValue(5);
    m_stopTimeoutSpin->setValue(5);

    applySettings();
    m_saveFeedback->setText(QStringLiteral("已恢复默认设置 ✓"));
}

void SettingsPage::applySettings()
{
    QString path = m_pathEdit->text().trimmed();
    if (path.isEmpty()) {
        m_saveFeedback->setText(QStringLiteral("保存路径不能为空"));
        m_saveFeedback->setStyleSheet(QStringLiteral("color: #e0525e; font-size: 13px; font-weight: 700; padding: 4px 8px;"));
        m_saveFeedback->setVisible(true);
        m_feedbackTimer->start(2000);
        return;
    }

    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    if (!dir.exists() || !QFileInfo(path).isWritable()) {
        m_saveFeedback->setText(QStringLiteral("路径不可写，请选择其他路径"));
        m_saveFeedback->setStyleSheet(QStringLiteral("color: #e0525e; font-size: 13px; font-weight: 700; padding: 4px 8px;"));
        m_saveFeedback->setVisible(true);
        m_feedbackTimer->start(2000);
        return;
    }

    QSettings settings;
    settings.setValue(QStringLiteral("settings/savePath"), path);
    settings.setValue(QStringLiteral("settings/frameRate"), m_fpsSpin->value());
    settings.setValue(QStringLiteral("settings/preset"), m_qualityCombo->currentData().toString());
    settings.setValue(QStringLiteral("settings/confirmStop"), m_confirmStopCheck->isChecked());
    settings.setValue(QStringLiteral("settings/showCursor"), m_showCursorCheck->isChecked());
    settings.setValue(QStringLiteral("settings/startTimeout"), m_startTimeoutSpin->value());
    settings.setValue(QStringLiteral("settings/stopTimeout"), m_stopTimeoutSpin->value());

    emit frameRateChanged(m_fpsSpin->value());
    emit presetChanged(m_qualityCombo->currentData().toString());
    emit savePathChanged(path);
    emit confirmStopChanged(m_confirmStopCheck->isChecked());
    emit showCursorChanged(m_showCursorCheck->isChecked());
    emit startTimeoutChanged(m_startTimeoutSpin->value() * 1000);
    emit stopTimeoutChanged(m_stopTimeoutSpin->value() * 1000);

    m_saveFeedback->setStyleSheet(QStringLiteral("color: #28965c; font-size: 13px; font-weight: 700; padding: 4px 8px;"));
    m_saveFeedback->setText(QStringLiteral("✓ 已保存"));
    m_saveFeedback->setVisible(true);
    m_feedbackTimer->start(2000);
}
