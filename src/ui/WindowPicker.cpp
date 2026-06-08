#include "WindowPicker.h"
#include "../recorder/RecorderController.h"

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QShortcut>
#include <QVBoxLayout>

WindowPicker::WindowPicker(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("选择窗口"));
    setMinimumSize(420, 320);
    resize(480, 400);

    auto *root = new QVBoxLayout(this);
    root->setSpacing(12);

    auto *header = new QLabel(QStringLiteral("请选择要录制的窗口："), this);
    header->setObjectName(QStringLiteral("pageHeader"));
    root->addWidget(header);

    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(QStringLiteral("搜索窗口..."));
    root->addWidget(m_filterEdit);

    m_list = new QListWidget(this);
    m_list->setAlternatingRowColors(true);
    root->addWidget(m_list, 1);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttonBox);

    m_windows = RecorderController::enumerateWindows();
    for (const auto &w : m_windows) {
        auto *item = new QListWidgetItem(w, m_list);
        item->setToolTip(w);
    }

    m_filterEdit->setFocus();
    if (m_list->count() > 0) {
        m_list->setCurrentRow(0);
    }

    auto *filterShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this);
    connect(filterShortcut, &QShortcut::activated, m_filterEdit, qOverload<>(&QWidget::setFocus));

    connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_list->clear();
        for (const auto &w : m_windows) {
            if (text.isEmpty() || w.contains(text, Qt::CaseInsensitive)) {
                auto *item = new QListWidgetItem(w, m_list);
                item->setToolTip(w);
            }
        }
        if (m_list->count() > 0)
            m_list->setCurrentRow(0);
    });

    connect(m_list, &QListWidget::itemDoubleClicked, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString WindowPicker::selectedWindow() const
{
    auto *item = m_list->currentItem();
    return item ? item->text() : QString();
}
