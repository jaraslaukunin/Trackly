#include "toastwidget.h"

#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

// --- ToastWidget -----------------------------------------------------------

ToastWidget::ToastWidget(const QString& text, ToastKind kind, QWidget* parent)
    : QFrame(parent)
{
    setObjectName("ToastWidget");
    setAttribute(Qt::WA_DeleteOnClose);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(10);

    auto* dot = new QLabel(this);
    dot->setFixedSize(10, 10);
    dot->setStyleSheet(QString("border:none;border-radius:5px;background:%1;")
                           .arg(kind == ToastKind::Error ? "#D97C88"
                                : kind == ToastKind::Success ? "#8A79E8"
                                                             : "#9B8CC9"));

    auto* label = new QLabel(text, this);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QString bg;
    QString border;
    QString fg;

    const bool dark = qApp->styleSheet().contains("#17141E");

    if (kind == ToastKind::Error) {
        bg = dark ? "#3A2430" : "#FFF1F3";
        border = dark ? "#6F4150" : "#E9C7CF";
        fg = dark ? "#FFE8EC" : "#6F3240";
    } else if (kind == ToastKind::Success) {
        bg = dark ? "#2A233B" : "#F7F4FF";
        border = dark ? "#4B3D72" : "#DDD3FB";
        fg = dark ? "#F1EBFF" : "#43306E";
    } else {
        bg = dark ? "#282334" : "#FBFAFF";
        border = dark ? "#433B57" : "#E4DCF8";
        fg = dark ? "#F1ECFF" : "#34264D";
    }

    setStyleSheet(QString(
                      "QFrame#ToastWidget {"
                      "background:%1;"
                      "border:1px solid %2;"
                      "border-radius:16px;"
                      "}"
                      "QLabel {"
                      "background:transparent;"
                      "color:%3;"
                      "font-size:13px;"
                      "font-weight:600;"
                      "}"
                      "QToolButton {"
                      "background:transparent;"
                      "border:none;"
                      "color:%3;"
                      "font-size:16px;"
                      "font-weight:700;"
                      "padding:0 2px;"
                      "}")
                      .arg(bg, border, fg));

    auto* closeBtn = new QToolButton(this);
    closeBtn->setText(QStringLiteral("×"));
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setAutoRaise(true);
    closeBtn->setStyleSheet(QString(
                                "QToolButton {"
                                "background:transparent;"
                                "border:none;"
                                "color:%1;"
                                "font-size:16px;"
                                "font-weight:700;"
                                "padding:0 2px;"
                                "}"
                                "QToolButton:hover { opacity:0.8; }")
                                .arg(fg));

    layout->addWidget(dot, 0, Qt::AlignTop);
    layout->addWidget(label, 1);
    layout->addWidget(closeBtn, 0, Qt::AlignTop);

    auto* opacity = new QGraphicsOpacityEffect(this);
    opacity->setOpacity(0.0);
    setGraphicsEffect(opacity);

    auto* fadeIn = new QPropertyAnimation(opacity, "opacity", this);
    fadeIn->setDuration(160);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    QObject::connect(closeBtn, &QToolButton::clicked, this, [this, opacity]() {
        auto* fadeOut = new QPropertyAnimation(opacity, "opacity", this);
        fadeOut->setDuration(140);
        fadeOut->setStartValue(opacity->opacity());
        fadeOut->setEndValue(0.0);
        QObject::connect(fadeOut, &QPropertyAnimation::finished, this, &ToastWidget::close);
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

// --- Toast system ----------------------------------------------------------

static QWidget* s_toastHost = nullptr;
static QVBoxLayout* s_toastLayout = nullptr;

static void ensureToastHost(QWidget* central)
{
    if (s_toastHost) {
        if (s_toastHost->parentWidget() != central) {
            s_toastHost->setParent(central);
        }
        s_toastHost->show();
        s_toastHost->raise();
        return;
    }

    s_toastHost = new QWidget(central);
    s_toastHost->setObjectName("ToastHost");
    s_toastHost->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    s_toastHost->setAttribute(Qt::WA_NoSystemBackground, true);
    s_toastHost->setAttribute(Qt::WA_TranslucentBackground, true);
    s_toastHost->setStyleSheet("background: transparent;");
    s_toastHost->resize(360, 10);

    s_toastLayout = new QVBoxLayout(s_toastHost);
    s_toastLayout->setContentsMargins(0, 0, 0, 0);
    s_toastLayout->setSpacing(10);
    s_toastLayout->addStretch();

    s_toastHost->show();
    s_toastHost->raise();
}

void showToast(QWidget* central, const QString& text, ToastKind kind, int timeoutMs)
{
    if (!central) return;

    ensureToastHost(central);

    const int width = 360;
    const int right = 18;
    const int top = 18;

    // Перед созданием нового тоста — убираем старые
    if (s_toastLayout) {
        while (s_toastLayout->count() > 0) {
            QLayoutItem* item = s_toastLayout->takeAt(0);
            if (!item) break;
            if (QWidget* w = item->widget()) {
                w->close();
                w->deleteLater();
            }
            delete item;
        }
    }

    auto* toast = new ToastWidget(text, kind, s_toastHost);
    toast->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    s_toastLayout->insertWidget(0, toast);
    toast->show();

    int totalHeight = 0;
    for (int i = 0; i < s_toastLayout->count(); ++i) {
        auto* item = s_toastLayout->itemAt(i);
        if (item && item->widget() && item->widget()->isVisible()) {
            totalHeight += item->widget()->sizeHint().height();
            if (i > 0) totalHeight += s_toastLayout->spacing();
        }
    }

    s_toastHost->setGeometry(
        central->width() - width - right,
        top,
        width,
        qMax(10, totalHeight));
    s_toastHost->show();
    s_toastHost->raise();
    toast->raise();

    QTimer::singleShot(timeoutMs, toast, [toast]() {
        if (!toast) return;

        auto* effect = qobject_cast<QGraphicsOpacityEffect*>(toast->graphicsEffect());
        if (!effect) {
            toast->close();
            return;
        }

        auto* fadeOut = new QPropertyAnimation(effect, "opacity", toast);
        fadeOut->setDuration(180);
        fadeOut->setStartValue(effect->opacity());
        fadeOut->setEndValue(0.0);
        QObject::connect(fadeOut, &QPropertyAnimation::finished, toast, &QWidget::close);
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

void repositionToast(QWidget* central)
{
    if (!s_toastHost || !central) return;

    const int width = 360;
    const int right = 18;
    const int top = 18;
    s_toastHost->move(central->width() - width - right, top);
    s_toastHost->resize(width, s_toastHost->height());
    s_toastHost->raise();
}
