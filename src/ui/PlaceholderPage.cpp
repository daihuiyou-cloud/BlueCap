#include "PlaceholderPage.h"

#include <QLabel>
#include <QVBoxLayout>

PlaceholderPage::PlaceholderPage(const QString &title, const QString &subtitle, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(72, 64, 72, 64);
    layout->setSpacing(16);

    auto *titleLabel = new QLabel(title);
    titleLabel->setObjectName(QStringLiteral("placeholderTitle"));

    auto *subtitleLabel = new QLabel(subtitle);
    subtitleLabel->setObjectName(QStringLiteral("placeholderSubtitle"));
    subtitleLabel->setWordWrap(true);

    layout->addStretch();
    layout->addWidget(titleLabel, 0, Qt::AlignHCenter);
    layout->addWidget(subtitleLabel, 0, Qt::AlignHCenter);
    layout->addStretch();

}
