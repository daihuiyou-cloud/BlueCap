#pragma once

#include <QWidget>

class PlaceholderPage : public QWidget
{
    Q_OBJECT

public:
    explicit PlaceholderPage(const QString &title, const QString &subtitle, QWidget *parent = nullptr);
};
