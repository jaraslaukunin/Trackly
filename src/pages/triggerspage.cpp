// Triggers/Wellbeing page implementation for MainWindow
// Redesigned — beautiful mood selector, sleep slider, chip toggles

#include "mainwindow.h"
#include "appcore.h"
#include "utils.h"
#include "widgets/toastwidget.h"
#include "page_includes.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QTextEdit>
#include <QVBoxLayout>

// ─── tiny helpers ───────────────────────────────────────────────────────────

static QPushButton* makeChip(const QString& text, const QString& emoji,
                              QWidget* parent)
{
    auto* btn = new QPushButton(QString("%1  %2").arg(emoji, text), parent);
    btn->setCheckable(true);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(QString(
        "QPushButton {"
        "  background: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 20px;"
        "  padding: 8px 16px;"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "  color: %3;"
        "}"
        "QPushButton:checked {"
        "  background: %4;"
        "  border-color: %4;"
        "  color: #FFFFFF;"
        "}"
        "QPushButton:hover:!checked {"
        "  background: %5;"
        "}"
    ).arg(
        "#F1ECFF", "#E2D8FF", "#5C45D3",   // light normal
        "#7C5CFA",                           // checked
        "#E8E1FB"                            // hover
    ));
    return btn;
}

static QPushButton* makeMoodBtn(const QString& emoji, int value,
                                 QWidget* parent)
{
    auto* btn = new QPushButton(emoji, parent);
    btn->setCheckable(true);
    btn->setFixedSize(56, 56);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setProperty("moodValue", value);
    btn->setStyleSheet(QString(
        "QPushButton {"
        "  font-size: 28px;"
        "  border: 2px solid transparent;"
        "  border-radius: 28px;"
        "  background: transparent;"
        "}"
        "QPushButton:checked {"
        "  border-color: #7C5CFA;"
        "  background: #F1ECFF;"
        "}"
        "QPushButton:hover:!checked {"
        "  background: #F8F4FF;"
        "}"
    ));
    return btn;
}

// ─── Page creation ──────────────────────────────────────────────────────────

QWidget* MainWindow::createTriggersPage()
{
    auto* page = new QWidget;
    auto* root = new QHBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(20);

    // ═══ LEFT: form ═══════════════════════════════════════════════════════
    auto* form = new QWidget(page);
    form->setFixedWidth(380);
    auto* fl = new QVBoxLayout(form);
    fl->setContentsMargins(0, 0, 0, 0);
    fl->setSpacing(14);

    // ── Mood card ──────────────────────────────────────────────────────
    auto* moodCard = new QFrame(form);
    moodCard->setObjectName("SectionCard");
    auto* moodLay = new QVBoxLayout(moodCard);
    moodLay->setContentsMargins(20, 20, 20, 20);
    moodLay->setSpacing(12);

    auto* moodTitle = new QLabel(tr("😌  Настроение"), moodCard);
    moodTitle->setObjectName("SectionTitle");

    auto* moodRow = new QHBoxLayout;
    moodRow->setSpacing(8);
    moodRow->addStretch();

    QList<QPushButton*> moodBtns;
    const QStringList moodEmojis = {"😫", "😟", "😐", "😊", "🤩"};
    for (int i = 0; i < moodEmojis.size(); ++i) {
        auto* btn = makeMoodBtn(moodEmojis[i], i + 1, moodCard);
        if (i == 2) btn->setChecked(true);
        moodBtns << btn;
        moodRow->addWidget(btn);
    }
    moodRow->addStretch();

    // exclusive group
    for (auto* b : moodBtns) {
        QObject::connect(b, &QPushButton::clicked, moodCard, [moodBtns, b]() {
            for (auto* other : moodBtns)
                other->setChecked(other == b);
        });
    }

    m_moodScore = new QComboBox(moodCard); // hidden, for data storage
    m_moodScore->hide();
    m_moodScore->addItems({"1","2","3","4","5"});
    m_moodScore->setCurrentIndex(2);
    // sync emoji buttons → combo
    for (auto* b : moodBtns) {
        QObject::connect(b, &QPushButton::clicked, moodCard, [this, b]() {
            m_moodScore->setCurrentIndex(b->property("moodValue").toInt() - 1);
        });
    }

    m_triggerDate = new QDateTimeEdit(QDateTime::currentDateTime(), moodCard);
    m_triggerDate->setCalendarPopup(true);
    m_triggerDate->setDisplayFormat("dd.MM.yyyy  HH:mm");

    moodLay->addWidget(moodTitle);
    moodLay->addLayout(moodRow);
    moodLay->addWidget(m_triggerDate);

    // ── Sleep card ─────────────────────────────────────────────────────
    auto* sleepCard = new QFrame(form);
    sleepCard->setObjectName("SectionCard");
    auto* sleepLay = new QVBoxLayout(sleepCard);
    sleepLay->setContentsMargins(20, 20, 20, 20);
    sleepLay->setSpacing(10);

    auto* sleepTitle = new QLabel(tr("😴  Сон"), sleepCard);
    sleepTitle->setObjectName("SectionTitle");

    auto* sleepVal = new QLabel(tr("8.0 ч"), sleepCard);
    sleepVal->setObjectName("SummaryValue");

    m_sleepHours = new QDoubleSpinBox(sleepCard);
    m_sleepHours->setRange(0, 24);
    m_sleepHours->setValue(8.0);
    m_sleepHours->setSingleStep(0.5);
    m_sleepHours->setDecimals(1);
    m_sleepHours->setPrefix(tr("Часов: "));
    m_sleepHours->hide();

    auto* sleepSlider = new QSlider(Qt::Horizontal, sleepCard);
    sleepSlider->setRange(0, 48); // 0–24 in 0.5 steps
    sleepSlider->setValue(16);    // 8.0
    sleepSlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "  height: 6px; border-radius: 3px; background: #E4DCF8;"
        "}"
        "QSlider::handle:horizontal {"
        "  width: 18px; height: 18px; margin: -7px 0;"
        "  border-radius: 9px; background: #7C5CFA;"
        "}");

    QObject::connect(sleepSlider, &QSlider::valueChanged, sleepCard,
                     [sleepVal, this](int v) {
        double h = v / 2.0;
        sleepVal->setText(QString("%1 ч").arg(h, 0, 'f', 1));
        m_sleepHours->setValue(h);
    });

    sleepLay->addWidget(sleepTitle);
    sleepLay->addWidget(sleepVal);
    sleepLay->addWidget(sleepSlider);

    // ── Triggers card ──────────────────────────────────────────────────
    auto* trigCard = new QFrame(form);
    trigCard->setObjectName("SectionCard");
    auto* trigLay = new QVBoxLayout(trigCard);
    trigLay->setContentsMargins(20, 20, 20, 20);
    trigLay->setSpacing(10);

    auto* trigTitle = new QLabel(tr("⚡  Триггеры"), trigCard);
    trigTitle->setObjectName("SectionTitle");

    // Chip layout with wrapping
    auto* chipHost = new QWidget(trigCard);
    auto* chipGrid = new QGridLayout(chipHost);
    chipGrid->setContentsMargins(0, 0, 0, 0);
    chipGrid->setSpacing(8);

    struct ChipDef { QString text; QString emoji; };
    const QList<ChipDef> chips = {
        {"Недосып",    "😵"},
        {"Стресс",     "😰"},
        {"Пропуск лек.", "💊"},
        {"Яркий свет", "💡"},
        {"Громкий шум","🔊"},
        {"Алкоголь",   "🍷"},
        {"Погода",     "🌡️"},
        {"Еда",        "🍕"},
        {"Экран",      "📱"},
        {"Другое",     "➕"},
    };

    QList<QPushButton*> chipBtns;
    for (int i = 0; i < chips.size(); ++i) {
        auto* btn = makeChip(chips[i].text, chips[i].emoji, chipHost);
        chipBtns << btn;
        chipGrid->addWidget(btn, i / 4, i % 4);
    }

    trigLay->addWidget(trigTitle);
    trigLay->addWidget(chipHost);

    // ── Title & Note ───────────────────────────────────────────────────
    m_triggerTitle = new QLineEdit(form);
    m_triggerTitle->setPlaceholderText(tr("Короткое название записи"));

    m_triggerNote = new QTextEdit(form);
    m_triggerNote->setPlaceholderText(tr("Как прошёл день? Что чувствовал?"));
    m_triggerNote->setFixedHeight(80);

    // ── Save button ────────────────────────────────────────────────────
    auto* saveBtn = new QPushButton(tr("💾  Сохранить"), form);
    saveBtn->setObjectName("PrimaryButton");
    saveBtn->setMinimumHeight(48);
    saveBtn->setCursor(Qt::PointingHandCursor);

    // ── Assemble form ──────────────────────────────────────────────────
    fl->addWidget(moodCard);
    fl->addWidget(sleepCard);
    fl->addWidget(trigCard);
    fl->addWidget(m_triggerTitle);
    fl->addWidget(m_triggerNote);
    fl->addWidget(saveBtn);
    fl->addStretch();

    // scroll form
    auto* formScroll = new QScrollArea(page);
    formScroll->setWidgetResizable(true);
    formScroll->setFrameShape(QFrame::NoFrame);
    formScroll->setWidget(form);
    formScroll->setFixedWidth(400);

    // ═══ RIGHT: history ════════════════════════════════════════════════════
    m_triggerListHost = new QWidget(page);
    m_triggerListHost->setObjectName("TriggerListHost");
    m_triggerListLayout = new QVBoxLayout(m_triggerListHost);
    m_triggerListLayout->setContentsMargins(0, 0, 0, 0);
    m_triggerListLayout->setSpacing(10);

    auto* placeholder = new QLabel(
        tr("Здесь появится история твоего самочувствия."),
        m_triggerListHost);
    placeholder->setObjectName("MutedLabel");
    placeholder->setWordWrap(true);
    placeholder->setAlignment(Qt::AlignCenter);
    m_triggerListLayout->addStretch();
    m_triggerListLayout->addWidget(placeholder);
    m_triggerListLayout->addStretch();

    auto* histScroll = new QScrollArea(page);
    histScroll->setWidgetResizable(true);
    histScroll->setFrameShape(QFrame::NoFrame);
    histScroll->setWidget(m_triggerListHost);

    root->addWidget(formScroll);
    root->addWidget(histScroll, 1);

    // ── Wire save ──────────────────────────────────────────────────────
    QObject::connect(saveBtn, &QPushButton::clicked, page, [this, chipBtns]() {
        QStringList selected;
        for (auto* cb : chipBtns) {
            if (cb->isChecked())
                selected << cb->text().section("  ", 1, 1).trimmed();
        }
        submitTrigger(); // sends trigger_title, sleep_hours, trigger_kind, etc.
    });

    return page;
}

// ─── Submit ─────────────────────────────────────────────────────────────────

void MainWindow::submitTriggerWithDetails(const QStringList& /*symptoms*/,
                                          const QStringList& /*triggers*/)
{
    // kept for compatibility, delegates to submitTrigger
    submitTrigger();
}

void MainWindow::submitTrigger()
{
    const QString title = m_triggerTitle ? m_triggerTitle->text().trimmed()
                                         : QString();
    const QString note  = m_triggerNote ? m_triggerNote->toPlainText().trimmed()
                                        : QString();

    if (title.isEmpty()) {
        showToast(m_central,
                  tr("Введи название записи"),
                  ToastKind::Error, 2600);
        return;
    }

    // collect checked chips as triggers
    QStringList chips;
    // find all chip buttons in the page
    auto chipBtns = m_triggerNote->parentWidget()
                        ->findChildren<QPushButton*>();
    for (auto* btn : chipBtns) {
        if (btn->isChecked() && btn->isCheckable()
            && !btn->text().startsWith("😫") && !btn->text().startsWith("😟")
            && !btn->text().startsWith("😐") && !btn->text().startsWith("😊")
            && !btn->text().startsWith("🤩"))
        {
            QString t = btn->text().section("  ", 1, 1).trimmed();
            if (!t.isEmpty()) chips << t;
        }
    }

    QJsonArray triggerArray;
    for (const auto& t : chips) {
        triggerArray.append(t);
    }

    const int mood = m_moodScore ? (m_moodScore->currentIndex() + 1) : 3;
    const double sleepH = m_sleepHours ? m_sleepHours->value() : 8.0;

    QJsonObject payload{
        {"user_id",       m_core->currentUserId()},
        {"logged_at",     m_triggerDate->dateTime().toString(Qt::ISODate)},
        {"mood",          mood},
        {"sleep_hours",   sleepH},
        {"sleep_quality", 3},
        {"trigger_kind",  QStringLiteral("Общее")},
        {"trigger_title", title},
        {"triggers",      triggerArray},
        {"note",          note},
    };

    m_core->createTrigger(payload);

    // reset form
    m_triggerDate->setDateTime(QDateTime::currentDateTime());
    m_triggerTitle->clear();
    m_triggerNote->clear();

    showToast(m_central, tr("Запись сохранена! 🎉"), ToastKind::Success, 2000);
}

// ─── List refresh ───────────────────────────────────────────────────────────

void MainWindow::refreshTriggersList()
{
    if (!m_triggerListLayout || !m_triggerListHost)
        return;

    ::clearLayout(m_triggerListLayout);

    if (m_cachedTriggers.isEmpty()) {
        auto* empty = new QLabel(
            tr("Здесь появится история твоего самочувствия."),
            m_triggerListHost);
        empty->setObjectName("MutedLabel");
        empty->setWordWrap(true);
        empty->setAlignment(Qt::AlignCenter);
        m_triggerListLayout->addStretch();
        m_triggerListLayout->addWidget(empty);
        m_triggerListLayout->addStretch();
        return;
    }

    for (const auto& v : m_cachedTriggers) {
        const QJsonObject o = v.toObject();

        auto* card = new QFrame(m_triggerListHost);
        card->setObjectName("FeedCard");

        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(18, 16, 18, 16);
        cl->setSpacing(8);

        const int mood           = o.value("mood").toInt();
        const double sleepHours  = o.value("sleep_hours").toDouble();
        const QString title      = o.value("trigger_title").toString();
        const QString kind       = o.value("trigger_kind").toString();
        const QString noteText   = o.value("note").toString();
        const QString when       = formatTs(o.value("logged_at").toString());

        const QStringList moodEmojis = {"😫", "😟", "😐", "😊", "🤩"};
        const QString moodEmoji = (mood >= 1 && mood <= 5)
                                      ? moodEmojis[mood - 1] : QString("😐");

        // header row: mood + title + date
        auto* header = new QHBoxLayout;
        header->setSpacing(10);

        auto* moodLbl = new QLabel(
            QString("%1  %2/5").arg(moodEmoji).arg(mood), card);
        moodLbl->setObjectName("BadgeLabel");

        auto* titleLbl = new QLabel(
            title.isEmpty() ? kind : title, card);
        titleLbl->setObjectName("CardTitle");

        header->addWidget(moodLbl);
        header->addWidget(titleLbl, 1);

        auto* dateLbl = new QLabel(when, card);
        dateLbl->setObjectName("MutedLabel");
        header->addWidget(dateLbl);

        cl->addLayout(header);

        // sleep
        if (sleepHours > 0) {
            auto* sleepLbl = new QLabel(
                tr("😴  %1 ч").arg(sleepHours, 0, 'f', 1), card);
            sleepLbl->setObjectName("MutedLabel");
            cl->addWidget(sleepLbl);
        }

        // triggers tags
        const auto arrTags = o.value("triggers").toArray();
        if (!arrTags.isEmpty()) {
            QStringList tags;
            for (const auto& tv : arrTags) {
                const QString s = tv.toString().trimmed();
                if (!s.isEmpty()) tags << s;
            }
            if (!tags.isEmpty()) {
                auto* tagsLbl = new QLabel(
                    tr("⚡  %1").arg(tags.join("  ·  ")), card);
                tagsLbl->setObjectName("MutedLabel");
                tagsLbl->setWordWrap(true);
                cl->addWidget(tagsLbl);
            }
        }

        // note
        if (!noteText.isEmpty()) {
            auto* noteLbl = new QLabel(noteText, card);
            noteLbl->setWordWrap(true);
            cl->addWidget(noteLbl);
        }

        // delete
        auto* delBtn = new QPushButton(tr("Удалить"), card);
        delBtn->setObjectName("SecondaryButton");
        delBtn->setFixedWidth(100);
        QObject::connect(delBtn, &QPushButton::clicked, card, [this, o]() {
            const QString id = o.value("id").toString();
            if (!id.isEmpty()) m_core->deleteTrigger(id);
        });

        auto* botRow = new QHBoxLayout;
        botRow->addStretch();
        botRow->addWidget(delBtn);
        cl->addLayout(botRow);

        m_triggerListLayout->addWidget(card);
    }

    m_triggerListLayout->addStretch();
}

// ─── Delete confirmation ────────────────────────────────────────────────────

void MainWindow::deleteTrigger(const QJsonObject& trigger)
{
    const QString id = trigger.value("id").toString();
    if (id.isEmpty()) return;

    const QString title = trigger.value("trigger_title").toString();
    const auto res = QMessageBox::question(
        this, tr("Удаление"),
        tr("Удалить запись «%1»?").arg(title.isEmpty() ? tr("без названия") : title),
        QMessageBox::Yes | QMessageBox::No);

    if (res == QMessageBox::Yes)
        m_core->deleteTrigger(id);
}

// ─── Data fill ──────────────────────────────────────────────────────────────

void MainWindow::fillTriggers(const QJsonArray& arr)
{
    m_cachedTriggers = arr;
    refreshTriggersList();
    refreshAnalyticsPage();
}
