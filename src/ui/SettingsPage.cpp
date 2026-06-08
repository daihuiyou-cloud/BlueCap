#include "SettingsPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
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

    QSettings defaults;

    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
        + QStringLiteral("/BlueCap");
    m_pathEdit = new QLineEdit(form);
    m_pathEdit->setText(defaults.value(QStringLiteral("settings/savePath"), defaultPath).toString());
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
    m_fpsSpin->setValue(defaults.value(QStringLiteral("settings/frameRate"), 30).toInt());
    m_fpsSpin->setSuffix(QStringLiteral(" fps"));
    layout->addRow(QStringLiteral("帧率"), m_fpsSpin);

    m_qualityCombo = new QComboBox(form);
    m_qualityCombo->addItem(QStringLiteral(" ultrafast (低画质)"), QStringLiteral("ultrafast"));
    m_qualityCombo->addItem(QStringLiteral(" superfast"), QStringLiteral("superfast"));
    m_qualityCombo->addItem(QStringLiteral(" veryfast"), QStringLiteral("veryfast"));
    m_qualityCombo->addItem(QStringLiteral(" faster"), QStringLiteral("faster"));
    m_qualityCombo->addItem(QStringLiteral(" fast (推荐)"), QStringLiteral("fast"));
    m_qualityCombo->addItem(QStringLiteral(" medium"), QStringLiteral("medium"));
    m_qualityCombo->addItem(QStringLiteral(" slow (高画质)"), QStringLiteral("slow"));
    const QString savedPreset = defaults.value(
        QStringLiteral("settings/preset"), QStringLiteral("ultrafast")).toString();
    int idx = m_qualityCombo->findData(savedPreset);
    if (idx >= 0) m_qualityCombo->setCurrentIndex(idx);
    layout->addRow(QStringLiteral("画质"), m_qualityCombo);

    m_confirmStopCheck = new QCheckBox(QStringLiteral("停止录制时确认"), form);
    m_confirmStopCheck->setChecked(defaults.value(QStringLiteral("settings/confirmStop"), false).toBool());
    layout->addRow(QStringLiteral(""), m_confirmStopCheck);

    m_startTimeoutSpin = new QSpinBox(form);
    m_startTimeoutSpin->setRange(1, 30);
    m_startTimeoutSpin->setValue(defaults.value(QStringLiteral("settings/startTimeout"), 5).toInt());
    m_startTimeoutSpin->setSuffix(QStringLiteral(" 秒"));
    layout->addRow(QStringLiteral("启动超时"), m_startTimeoutSpin);

    m_stopTimeoutSpin = new QSpinBox(form);
    m_stopTimeoutSpin->setRange(1, 30);
    m_stopTimeoutSpin->setValue(defaults.value(QStringLiteral("settings/stopTimeout"), 5).toInt());
    m_stopTimeoutSpin->setSuffix(QStringLiteral(" 秒"));
    layout->addRow(QStringLiteral("停止超时"), m_stopTimeoutSpin);

    root->addWidget(form);
    root->addStretch();

    connect(browseBtn, &QPushButton::clicked, this, &SettingsPage::browsePath);

    connect(m_pathEdit, &QLineEdit::textChanged, this, &SettingsPage::applySettings);
    connect(m_fpsSpin, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPage::applySettings);
    connect(m_qualityCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] {
        applySettings();
    });
    connect(m_confirmStopCheck, &QCheckBox::toggled, this, &SettingsPage::applySettings);
    connect(m_startTimeoutSpin, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPage::applySettings);
    connect(m_stopTimeoutSpin, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsPage::applySettings);
}

void SettingsPage::browsePath()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        QStringLiteral("选择保存路径"), m_pathEdit->text());
    if (!dir.isEmpty()) {
        m_pathEdit->setText(dir);
    }
}

void SettingsPage::applySettings()
{
    QSettings settings;
    settings.setValue(QStringLiteral("settings/savePath"), m_pathEdit->text());
    settings.setValue(QStringLiteral("settings/frameRate"), m_fpsSpin->value());
    settings.setValue(QStringLiteral("settings/preset"), m_qualityCombo->currentData().toString());
    settings.setValue(QStringLiteral("settings/confirmStop"), m_confirmStopCheck->isChecked());
    settings.setValue(QStringLiteral("settings/startTimeout"), m_startTimeoutSpin->value());
    settings.setValue(QStringLiteral("settings/stopTimeout"), m_stopTimeoutSpin->value());

    emit frameRateChanged(m_fpsSpin->value());
    emit presetChanged(m_qualityCombo->currentData().toString());
    emit savePathChanged(m_pathEdit->text());
    emit confirmStopChanged(m_confirmStopCheck->isChecked());
    emit startTimeoutChanged(m_startTimeoutSpin->value() * 1000);
    emit stopTimeoutChanged(m_stopTimeoutSpin->value() * 1000);
}
