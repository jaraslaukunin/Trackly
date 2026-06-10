#pragma once

#include <QFrame>
#include <QWidget>
#include <QString>

enum class ToastKind {
    Info,
    Success,
    Error
};

class ToastWidget : public QFrame
{
public:
    ToastWidget(const QString& text, ToastKind kind, QWidget* parent = nullptr);
};

void showToast(QWidget* central, const QString& text, ToastKind kind, int timeoutMs = 2600);
