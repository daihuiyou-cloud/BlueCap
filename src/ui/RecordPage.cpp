#include "RecordPage.h"
#include "paint/PaintMetrics.h"
#include "paint/PaintPrimitives.h"
#include "theme/ThemeColors.h"

#include "ModeSwitch.h"
#include "RecordButton.h"
#include "RegionSelector.h"
#include "WindowPicker.h"
#include "recorder/IRecorderService.h"
#include "storage/IVideoLibrary.h"
#include "utils/Format.h"
#include "widgets/AudioToggleCard.h"
#include "widgets/PaintedComboBox.h"
#include "widgets/PaintedDialog.h"


#include <QComboBox>
#include <QDateTime>
#include <QPalette>
#include <QCursor>
#include <QDesktopServices>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QShortcut>
#include <QSizePolicy>
#include <QUrl>
#include <QPainter>
#include <QTimer>
#include <QVBoxLayout>

RecordPage::RecordPage(IRecorderService *recorder, IVideoLibrary *library, QWidget *parent)
    : QWidget(parent)
    , m_recorder(recorder)
    , m_library(library)
    , m_palette(paint::theme(false))
{
    setAttribute(Qt::WA_StyledBackground, false);

    m_recordingTimer = new QTimer(this);
    connect(m_recordingTimer, &QTimer::timeout, this, &RecordPage::updateElapsedTime);

    m_countdownTimer = new QTimer(this);
    connect(m_countdownTimer, &QTimer::timeout, this, [this] {
        m_countdownValue--;
        if (m_countdownValue <= 0) {
            m_countdownTimer->stop();
            m_countdownLabel->setVisible(false);
            m_titleLabel->setVisible(true);
            doStartRecording();
            return;
        }
        m_countdownLabel->setText(QString::number(m_countdownValue));
    });

    m_stopProgressTimer = new QTimer(this);
    connect(m_stopProgressTimer, &QTimer::timeout, this, &RecordPage::updateStopProgress);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(34, 24, 34, 24);
    root->setSpacing(0);

    m_modeSwitch = new ModeSwitch(this);
    root->addWidget(m_modeSwitch);
    root->addSpacing(18);

    connect(m_modeSwitch, &ModeSwitch::modeChanged, this, [this](RecordMode mode) {
        if (!m_recorder->isRecording()) {
            updateStatusForMode(mode);
            updateScreenCombo();
        }
    });

    m_screenCombo = new PaintedComboBox(this);
    m_screenCombo->setVisible(false);
    m_screenCombo->setFixedHeight(32);
    m_screenCombo->setMinimumWidth(280);
    root->addWidget(m_screenCombo, 0, Qt::AlignHCenter);
    root->addSpacing(10);

    m_controlPanel = new QWidget(this);
    m_controlPanel->setMinimumHeight(220);
    m_controlPanel->setMaximumHeight(270);
    m_controlPanel->setMinimumWidth(540);

    setMinimumWidth(620);

    m_micCard = new AudioToggleCard(QStringLiteral("麦克风"), AudioToggleCard::Microphone, m_controlPanel);
    m_systemAudioCard = new AudioToggleCard(QStringLiteral("系统声音"), AudioToggleCard::SystemAudio, m_controlPanel);
    m_recordButton = new RecordButton(m_controlPanel);

    m_titleLabel = new QLabel(QStringLiteral("开始录制"), this);
    m_titleLabel->setParent(m_controlPanel);
    m_titleLabel->setText(QStringLiteral("开始录制"));
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_countdownLabel = new QLabel(this);
    m_countdownLabel->setParent(m_controlPanel);
    m_countdownLabel->setAlignment(Qt::AlignCenter);
    m_countdownLabel->setVisible(false);

    m_hotkeyLabel = new QLabel(QStringLiteral("Ctrl + Shift + R"), this);
    m_hotkeyLabel->setParent(m_controlPanel);
    m_hotkeyLabel->setAlignment(Qt::AlignCenter);

    m_statusLabel = new QLabel(QStringLiteral("点击开始后录制整个屏幕，视频会保存到 BlueCap 文件夹"), this);
    m_statusLabel->setParent(m_controlPanel);
    m_statusLabel->setText(QStringLiteral("点击开始后录制整个屏幕，视频会保存到 BlueCap 文件夹"));
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setCursor(Qt::PointingHandCursor);
    m_statusLabel->setToolTip(QStringLiteral("点击打开保存位置"));
    m_statusLabel->installEventFilter(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);


    m_stopStatusLabel = new QLabel(QStringLiteral("正在结束录制并写入视频文件..."), this);
    m_stopStatusLabel->setParent(m_controlPanel);
    m_stopStatusLabel->setText(QStringLiteral("正在结束录制并写入视频文件..."));
    m_stopStatusLabel->setAlignment(Qt::AlignCenter);
    QFont stopFont = m_stopStatusLabel->font();
    stopFont.setPixelSize(14);
    stopFont.setBold(true);
    m_stopStatusLabel->setFont(stopFont);
    m_stopStatusLabel->setVisible(false);

    m_stopProgress = new QProgressBar(this);
    m_stopProgress->setParent(m_controlPanel);
    m_stopProgress->setRange(0, 0);
    m_stopProgress->setFixedWidth(320);
    m_stopProgress->setFixedHeight(6);
    m_stopProgress->setTextVisible(false);
    m_stopProgress->setVisible(false);
    root->addWidget(m_controlPanel);

    auto *escapeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    escapeShortcut->setContext(Qt::ApplicationShortcut);
    connect(escapeShortcut, &QShortcut::activated, this, [this] {
        if (m_recorder->isRecording()) {
            toggleRecording();
        } else if (m_countdownTimer->isActive()) {
            m_countdownTimer->stop();
            m_countdownLabel->setVisible(false);
            m_titleLabel->setVisible(true);
            updateStatusForMode(m_modeSwitch->currentMode());
        }
    });

    updateSectionHeights();
    layoutControlPanel();

    connect(m_recordButton, &QAbstractButton::clicked, this, &RecordPage::toggleRecording);
    connect(m_recorder, &IRecorderService::recordingChanged, this, &RecordPage::handleRecordingChanged);
    connect(m_recorder, &IRecorderService::videoSaved, this, &RecordPage::handleVideoSaved);
    connect(m_recorder, &IRecorderService::errorOccurred, this, &RecordPage::handleError);
    updateStatusForMode(m_modeSwitch->currentMode());
    updateLabelColors();
}

void RecordPage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF panel = QRectF(rect());
    paint::drawCard(painter, panel, m_palette.panelBg, m_palette.panelBorder, paint::Metrics::pagePanelRadius);
    paint::drawVerticalSheen(painter, panel, paint::Metrics::pagePanelRadius, m_darkMode);

    if (m_controlPanel) {
        QRectF control = QRectF(m_controlPanel->geometry());
        paint::drawCard(painter, control, m_palette.cardBg, m_palette.cardBorder, paint::Metrics::cardRadius);
        paint::drawVerticalSheen(painter, control, paint::Metrics::cardRadius, m_darkMode);
    }

}

void RecordPage::resizeEvent(QResizeEvent *event)
{
    updateSectionHeights();
    layoutControlPanel();
    QWidget::resizeEvent(event);
}

void RecordPage::updateSectionHeights()
{
    if (!m_controlPanel)
        return;

    const int margins = 24 + 24;
    const int modeHeight = m_modeSwitch ? m_modeSwitch->height() : paint::Metrics::modeCardHeight;
    const int screenHeight = m_screenCombo && m_screenCombo->isVisible() ? m_screenCombo->height() + 10 : 0;
    const int fixedGaps = 18;
    int available = height() - margins - modeHeight - screenHeight - fixedGaps;
    if (available <= 0)
        available = 430;

    int controlHeight = qBound(220, available, 400);
    m_controlPanel->setFixedHeight(controlHeight);
}

void RecordPage::layoutControlPanel()
{
    if (!m_controlPanel || !m_recordButton)
        return;

    const QRect r = m_controlPanel->rect().adjusted(26, 14, -26, -14);
    const int centerX = r.center().x();
    const int top = r.top();
    const int buttonSize = paint::Metrics::recordButtonSize;
    const int minCardGap = 8;

    m_recordButton->setGeometry(centerX - buttonSize / 2, top + 26, buttonSize, buttonSize);

    const int cardY = top + 94;
    int sideInset = qMax(42, r.width() / 10);
    if (m_micCard && m_systemAudioCard) {
        const int maxInset = qMax(0, r.width() / 2 - buttonSize / 2 - minCardGap - m_micCard->width());
        sideInset = qMin(sideInset, maxInset);
    }
    if (m_micCard)
        m_micCard->move(r.left() + sideInset, cardY);
    if (m_systemAudioCard)
        m_systemAudioCard->move(r.right() - sideInset - m_systemAudioCard->width(), cardY);

    const int titleY = top + 146;
    const int titleW = qBound(160, r.width() - 200, 300);
    m_titleLabel->setGeometry(centerX - titleW / 2, titleY, titleW, 34);
    m_countdownLabel->setGeometry(m_titleLabel->geometry());
    const int hotkeyW = qMin(340, r.width() - 40);
    m_hotkeyLabel->setGeometry(centerX - hotkeyW / 2, titleY + 38, hotkeyW, 28);

    const int statusY = qMin(r.bottom() - 24, titleY + 70);
    m_statusLabel->setGeometry(r.left() + 34, statusY, r.width() - 68, 22);
    m_stopStatusLabel->setGeometry(r.left() + 34, statusY - 26, r.width() - 68, 22);
    m_stopProgress->setGeometry(centerX - 160, statusY, 320, 6);
}

void RecordPage::setConfirmStop(bool confirm)
{
    m_confirmStop = confirm;
}

void RecordPage::updateLabelColors()
{
    const auto &a = ThemeColors::forMode(m_darkMode).app;
    QFont titleFont = m_titleLabel->font();
    titleFont.setPixelSize(21);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    QPalette tp = m_titleLabel->palette();
    tp.setColor(QPalette::WindowText, a.recordTitleText);
    m_titleLabel->setPalette(tp);

    QFont hotkeyFont = m_hotkeyLabel->font();
    hotkeyFont.setPixelSize(16);
    hotkeyFont.setBold(true);
    m_hotkeyLabel->setFont(hotkeyFont);
    QPalette hp = m_hotkeyLabel->palette();
    hp.setColor(QPalette::WindowText, a.recordHotkeyText);
    m_hotkeyLabel->setPalette(hp);

    QFont statusFont = m_statusLabel->font();
    statusFont.setPixelSize(13);
    m_statusLabel->setFont(statusFont);
    QPalette sp = m_statusLabel->palette();
    sp.setColor(QPalette::WindowText, a.recordStatusText);
    m_statusLabel->setPalette(sp);

    QPalette ssp = m_stopStatusLabel->palette();
    ssp.setColor(QPalette::WindowText, a.stopStatusText);
    m_stopStatusLabel->setPalette(ssp);
}

void RecordPage::setDarkMode(bool dark)
{
    m_darkMode = dark;
    m_modeSwitch->setDarkMode(dark);
    m_recordButton->setDarkMode(dark);
    if (m_micCard)
        m_micCard->setDarkMode(dark);
    if (auto *combo = qobject_cast<PaintedComboBox *>(m_screenCombo))
        combo->setDarkMode(dark);
    if (m_systemAudioCard)
        m_systemAudioCard->setDarkMode(dark);
    m_palette = paint::theme(dark);
    updateLabelColors();
    update();
}

void RecordPage::updateScreenCombo()
{
    m_screenCombo->clear();
    const auto screens = QGuiApplication::screens();
    if (screens.size() < 2 || m_modeSwitch->currentMode() != RecordMode::FullScreen) {
        m_screenCombo->setVisible(false);
        updateSectionHeights();
        layoutControlPanel();
        return;
    }
    m_screenCombo->setVisible(true);
    int selectedIdx = 0;
    QScreen *activeScreen = QGuiApplication::screenAt(QCursor::pos());
    for (int i = 0; i < screens.size(); ++i) {
        QRect g = screens[i]->geometry();
        m_screenCombo->addItem(QStringLiteral("屏幕 %1 (%2x%3)").arg(i + 1).arg(g.width()).arg(g.height()));
        if (screens[i] == activeScreen)
            selectedIdx = i;
    }
    m_screenCombo->setCurrentIndex(selectedIdx);
    updateSectionHeights();
    layoutControlPanel();
}

QScreen *RecordPage::selectedScreen() const
{
    if (!m_screenCombo->isVisible())
        return nullptr;
    const auto screens = QGuiApplication::screens();
    int idx = m_screenCombo->currentIndex();
    if (idx >= 0 && idx < screens.size())
        return screens[idx];
    return nullptr;
}

void RecordPage::startQuickRecording()
{
    if (m_recorder->isRecording()) return;
    if (m_countdownTimer->isActive()) return;
    m_recorder->startFullScreenRecording(nullptr);
}

void RecordPage::toggleRecording()
{
    if (m_recorder->isRecording()) {
        if (m_confirmStop) {
            if (!PaintedDialog::question(this, QStringLiteral("停止录制"),
                    QStringLiteral("确定要停止当前录制吗？")))
                return;
        }
        m_recordingTimer->stop();
        m_lastSavedPath.clear();
        m_stopOutputPath = m_recorder->currentOutputPath();
        m_stopStatusLabel->setText(QStringLiteral("正在结束录制并写入视频文件..."));
        m_stopStatusLabel->setVisible(true);
        m_stopProgress->setVisible(true);
        m_stopProgressTimer->start(500);
        m_recorder->stopRecording();
        return;
    }

    if (m_countdownTimer->isActive()) {
        m_countdownTimer->stop();
        m_countdownLabel->setVisible(false);
        m_titleLabel->setVisible(true);
        m_hiddenForRecording = false;
        updateStatusForMode(m_modeSwitch->currentMode());
        return;
    }

    switch (m_modeSwitch->currentMode()) {
    case RecordMode::FullScreen:
        emit requestWindowHide();
        m_hiddenForRecording = true;
        m_countdownValue = 3;
        m_titleLabel->setVisible(false);
        m_countdownLabel->setText(QStringLiteral("3"));
        m_countdownLabel->setVisible(true);
        m_statusLabel->setText(QStringLiteral("录制即将开始..."));
        m_countdownTimer->start(1000);
        break;
    case RecordMode::Region:
        emit requestWindowHide();
        m_hiddenForRecording = true;
        doStartRecording();
        break;
    case RecordMode::Window:
        emit requestWindowHide();
        m_hiddenForRecording = true;
        doStartRecording();
        break;
    }
}

void RecordPage::doStartRecording()
{
    m_recordButton->setEnabled(false);
    m_statusLabel->setText(QStringLiteral("正在启动录制程序..."));
    switch (m_modeSwitch->currentMode()) {
    case RecordMode::FullScreen:
        m_recorder->startFullScreenRecording(selectedScreen());
        break;
    case RecordMode::Region:
        startRegionSelection();
        break;
    case RecordMode::Window:
        pickWindow();
        break;
    }
}

void RecordPage::openSaveFolder()
{
    const QString path = m_recorder->currentSavePath();
    if (path.isEmpty())
        return;

    QDir().mkpath(path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void RecordPage::startRegionSelection()
{
    auto *selector = new RegionSelector;
    selector->setAttribute(Qt::WA_DeleteOnClose);
    m_regionCommitted = false;
    connect(selector, &RegionSelector::regionSelected, this,
        [this](const QRect &region) {
            m_regionCommitted = true;
            m_recorder->startRegionRecording(region);
        });
    connect(selector, &QObject::destroyed, this, [this] {
        if (!m_regionCommitted && m_hiddenForRecording) {
            m_hiddenForRecording = false;
            emit requestWindowShow();
        }
    });
    selector->show();
}

void RecordPage::pickWindow()
{
    auto *picker = new WindowPicker();
    picker->setDarkMode(m_darkMode);
    picker->setAttribute(Qt::WA_DeleteOnClose);
    picker->setWindowModality(Qt::WindowModal);
    connect(picker, &QDialog::accepted, this, [this, picker] {
        QString selected = picker->selectedWindow();
        if (!selected.isEmpty()) {
            m_recorder->startWindowRecording(selected);
        } else if (m_hiddenForRecording) {
            m_hiddenForRecording = false;
            emit requestWindowShow();
        }
    });
    connect(picker, &QDialog::rejected, this, [this] {
        if (m_hiddenForRecording) {
            m_hiddenForRecording = false;
            emit requestWindowShow();
        }
    });
    picker->show();
}

void RecordPage::handleRecordingChanged(bool recording)
{
    m_recordButton->setRecording(recording);
    m_recordButton->setEnabled(true);
    m_modeSwitch->setModeEnabled(!recording);

    if (recording) {
        m_recordingTimer->start(1000);
        m_elapsed.start();
        m_titleLabel->setText(QStringLiteral("正在录制 00:00"));
        m_statusLabel->setText(QStringLiteral("点击中间按钮或按 Esc 停止录制"));
        m_statusLabel->setToolTip(QStringLiteral("录制完成后可点击打开视频"));
        m_statusOpensSavedVideo = false;
    } else {
        m_recordingTimer->stop();
        m_stopProgressTimer->stop();
        m_stopStatusLabel->setText(QStringLiteral("正在结束录制并写入视频文件..."));
        m_stopStatusLabel->setVisible(false);
        m_stopProgress->setVisible(false);
        m_statusLabel->setVisible(true);
        m_titleLabel->setText(QStringLiteral("开始录制"));
        updateStatusForMode(m_modeSwitch->currentMode());

        if (m_hiddenForRecording) {
            m_hiddenForRecording = false;
            emit requestWindowShow();
        }
    }

    m_hotkeyLabel->setText(recording
        ? QStringLiteral("Esc 停止  |  Ctrl + Shift + R")
        : QStringLiteral("Ctrl + Shift + R"));
}

void RecordPage::handleVideoSaved(const QString &path)
{
    m_library->addRecentVideo(path);
    m_lastSavedPath = path;
    QFileInfo fi(path);
    m_statusLabel->setText(QStringLiteral("已保存：%1 (%2)  |  单击打开  |  右键打开文件夹").arg(fi.fileName(), format::fileSize(fi.size())));
    m_statusLabel->setToolTip(QStringLiteral("左键打开视频，右键打开所在文件夹"));
    m_statusOpensSavedVideo = true;
}

void RecordPage::handleError(const QString &message)
{
    m_recordButton->setRecording(false);
    m_recordButton->setEnabled(true);
    m_recordingTimer->stop();
    m_stopProgressTimer->stop();
    m_stopStatusLabel->setText(QStringLiteral("正在结束录制并写入视频文件..."));
    m_stopStatusLabel->setVisible(false);
    m_stopProgress->setVisible(false);
    m_countdownTimer->stop();
    m_countdownLabel->setVisible(false);
    m_titleLabel->setVisible(true);
    m_titleLabel->setText(QStringLiteral("开始录制"));
    m_statusLabel->setText(QStringLiteral("录制失败"));
    m_modeSwitch->setModeEnabled(true);
    updateStatusForMode(m_modeSwitch->currentMode());
    m_statusLabel->setText(QStringLiteral("录制启动失败，请尝试切换录制模式或检查安全软件拦截"));

    if (m_hiddenForRecording) {
        m_hiddenForRecording = false;
        window()->showNormal();
        window()->activateWindow();
    }

    PaintedDialog::warning(this, QStringLiteral("录制失败"), message);
}

void RecordPage::updateStatusForMode(RecordMode mode)
{
    m_statusOpensSavedVideo = false;
    m_statusLabel->setToolTip(QStringLiteral("点击打开保存位置"));

    switch (mode) {
    case RecordMode::FullScreen:
        m_statusLabel->setText(QStringLiteral("点击开始后录制整个屏幕，视频会保存到 BlueCap 文件夹"));
        break;
    case RecordMode::Region:
        m_statusLabel->setText(QStringLiteral("点击开始后拖动选择录制区域"));
        break;
    case RecordMode::Window:
        m_statusLabel->setText(QStringLiteral("点击开始后选择一个窗口"));
        break;
    }
}

void RecordPage::updateElapsedTime()
{
    qint64 secs = m_elapsed.elapsed() / 1000;
    int h = secs / 3600;
    int m = (secs % 3600) / 60;
    int s = secs % 60;
    if (h > 0) {
        m_titleLabel->setText(QStringLiteral("正在录制 %1:%2:%3")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0')));
    } else {
        m_titleLabel->setText(QStringLiteral("正在录制 %1:%2")
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0')));
    }
    emit elapsedUpdated(static_cast<int>(secs));
}

void RecordPage::updateStopProgress()
{
    QFileInfo fi(m_stopOutputPath);
    if (fi.exists()) {
        m_stopStatusLabel->setText(
            QStringLiteral("正在结束录制并写入视频文件... 已写入 %1").arg(format::fileSize(fi.size())));
    }
}

bool RecordPage::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            if (obj == m_statusLabel) {
                if (m_statusOpensSavedVideo && !m_lastSavedPath.isEmpty()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(m_lastSavedPath));
                } else {
                    openSaveFolder();
                }
                return true;
            }
        }
        if (me->button() == Qt::RightButton && obj == m_statusLabel && m_statusOpensSavedVideo) {
            openSaveFolder();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}
