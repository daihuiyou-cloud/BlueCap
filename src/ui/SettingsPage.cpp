#include "SettingsPage.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    qRegisterMetaTypeStreamOperators<QStringList>("QStringList");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 16);
    root->setSpacing(12);

    auto *header = new QLabel(QStringLiteral("设置"));
    header->setObjectName(QStringLiteral("pageHeader"));
    root->addWidget(header);

    auto *form = new QWidget(this);
    auto *layout = new QFormLayout(form);
    layout->setSpacing(12);
    layout->setContentsMargins(0, 0, 0, 0);

    QSettings defaults;

    m_pathEdit = new QLineEdit(form);
    m_pathEdit->setText(defaults.value(QStringLiteral("settings/savePath"),
        QStringLiteral("Videos/BlueCap")).toString());
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

    root->addWidget(form);

    auto *applyBtn = new QPushButton(QStringLiteral("应用"), this);
    applyBtn->setObjectName(QStringLiteral("applyButton"));
    applyBtn->setFixedSize(120, 36);
    root->addWidget(applyBtn, 0, Qt::AlignRight);
    root->addStretch();

    connect(browseBtn, &QPushButton::clicked, this, &SettingsPage::browsePath);
    connect(applyBtn, &QPushButton::clicked, this, &SettingsPage::applySettings);
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

    emit frameRateChanged(m_fpsSpin->value());
    emit presetChanged(m_qualityCombo->currentData().toString());
    emit savePathChanged(m_pathEdit->text());
}
