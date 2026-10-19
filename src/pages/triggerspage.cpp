// Triggers/Wellbeing page v3
// Mood emoji · sleep 1-5 pill · 3-col chips · trash icon · reminders

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
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QStyle>
#include <QTextEdit>
#include <QTime>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

// ─── helpers ────────────────────────────────────────────────────────────────

static QPushButton* makeChip(const QString& text, QWidget* parent)
{
    auto* btn = new QPushButton(text, parent);
    btn->setCheckable(true);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(QString(
        "QPushButton {"
        "  background: %1; border: 1px solid %2; border-radius: 14px;"
        "  padding: 8px 14px; font-size: 12px; font-weight: 600; color: %3;"
        "  min-width: 80px;"
        "}"
        "QPushButton:checked { background: %4; border-color: %4; color: #FFFFFF; }"
        "QPushButton:hover:!checked { background: %5; }"
    ).arg("#F1ECFF", "#E2D8FF", "#5C45D3", "#7C5CFA", "#E8E1FB"));
    return btn;
}

static QPushButton* makePillBtn(const QString& text, int value, QWidget* parent)
{
    auto* btn = new QPushButton(text, parent);
    btn->setCheckable(true);
    btn->setFixedSize(48, 48);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setProperty("pillValue", value);
    btn->setStyleSheet(QString(
        "QPushButton {"
        "  font-size: 18px; font-weight: 700;"
        "  border: 2px solid transparent; border-radius: 24px;"
        "  background: transparent; color: #5D5379;"
        "}"
        "QPushButton:checked {"
        "  border-color: #7C5CFA; background: #F1ECFF; color: #7C5CFA;"
        "}"
        "QPushButton:hover:!checked { background: #F8F4FF; }"
    ));
    return btn;
}

static QPushButton* makeMoodBtn(const QString& emoji, int value, QWidget* parent)
{
    auto* btn = new QPushButton(emoji, parent);
    btn->setCheckable(true);
    btn->setFixedSize(52, 52);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setProperty("moodValue", value);
    btn->setStyleSheet(QString(
        "QPushButton { font-size: 26px; border: 2px solid transparent;"
        "  border-radius: 26px; background: transparent; }"
        "QPushButton:checked { border-color: #7C5CFA; background: #F1ECFF; }"
        "QPushButton:hover:!checked { background: #F8F4FF; }"
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
    form->setFixedWidth(420);
    auto* fl = new QVBoxLayout(form);
    fl->setContentsMargins(0, 0, 0, 0);
    fl->setSpacing(14);

    // ── Time of day (centered) ─────────────────────────────────────────
    auto* todCard = new QFrame(form);
    todCard->setObjectName("SectionCard");
    auto* todLay = new QHBoxLayout(todCard);
    todLay->setContentsMargins(12, 8, 12, 8);
    todLay->setSpacing(0);

    auto* morningBtn = new QToolButton(todCard);
    morningBtn->setText(tr("☀️  Утро"));
    morningBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    morningBtn->setCheckable(true);
    morningBtn->setChecked(true);
    morningBtn->setMinimumHeight(40);
    morningBtn->setStyleSheet(
        "QToolButton { font-size:14px; font-weight:700; border-radius:10px;"
        " padding:8px 28px; background:transparent; color:#5D5379; }"
        "QToolButton:checked { background:#7C5CFA; color:#FFFFFF; }");

    auto* eveningBtn = new QToolButton(todCard);
    eveningBtn->setText(tr("🌙  Вечер"));
    eveningBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    eveningBtn->setCheckable(true);
    eveningBtn->setMinimumHeight(40);
    eveningBtn->setStyleSheet(
        "QToolButton { font-size:14px; font-weight:700; border-radius:10px;"
        " padding:8px 28px; background:transparent; color:#5D5379; }"
        "QToolButton:checked { background:#7C5CFA; color:#FFFFFF; }");

    QObject::connect(morningBtn, &QToolButton::clicked, todCard, [=]() {
        morningBtn->setChecked(true); eveningBtn->setChecked(false);
    });
    QObject::connect(eveningBtn, &QToolButton::clicked, todCard, [=]() {
        eveningBtn->setChecked(true); morningBtn->setChecked(false);
    });

    todLay->addStretch();
    todLay->addWidget(morningBtn);
    todLay->addWidget(eveningBtn);
    todLay->addStretch();

    // ── Mood card ──────────────────────────────────────────────────────
    auto* moodCard = new QFrame(form);
    moodCard->setObjectName("SectionCard");
    auto* moodLay = new QVBoxLayout(moodCard);
    moodLay->setContentsMargins(20, 20, 20, 20);
    moodLay->setSpacing(12);

    auto* moodTitle = new QLabel(tr("Настроение"), moodCard);
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

    for (auto* b : moodBtns) {
        QObject::connect(b, &QPushButton::clicked, moodCard, [moodBtns, b]() {
            for (auto* other : moodBtns) other->setChecked(other == b);
        });
    }

    m_moodScore = new QComboBox(moodCard);
    m_moodScore->hide();
    m_moodScore->addItems({"1","2","3","4","5"});
    m_moodScore->setCurrentIndex(2);
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
    sleepLay->setSpacing(12);

    auto* sleepTitle = new QLabel(tr("Сон"), sleepCard);
    sleepTitle->setObjectName("SectionTitle");

    // hours slider
    auto* sleepVal = new QLabel(tr("8.0 ч"), sleepCard);
    sleepVal->setObjectName("SummaryValue");

    m_sleepHours = new QDoubleSpinBox(sleepCard);
    m_sleepHours->setRange(0, 24);
    m_sleepHours->setValue(8.0);
    m_sleepHours->setSingleStep(0.5);
    m_sleepHours->setDecimals(1);
    m_sleepHours->hide();

    auto* sleepSlider = new QSlider(Qt::Horizontal, sleepCard);
    sleepSlider->setRange(0, 48);
    sleepSlider->setValue(16);
    sleepSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 6px; border-radius: 3px; background: #E4DCF8; }"
        "QSlider::handle:horizontal { width: 18px; height: 18px; margin: -7px 0;"
        "  border-radius: 9px; background: #7C5CFA; }");

    QObject::connect(sleepSlider, &QSlider::valueChanged, sleepCard,
                     [sleepVal, this](int v) {
        double h = v / 2.0;
        sleepVal->setText(QString("%1 ч").arg(h, 0, 'f', 1));
        m_sleepHours->setValue(h);
    });

    sleepLay->addWidget(sleepTitle);
    sleepLay->addWidget(sleepVal);
    sleepLay->addWidget(sleepSlider);

    // sleep quality: pill 1-5
    auto* qualLabel = new QLabel(tr("Качество сна"), sleepCard);
    qualLabel->setObjectName("MutedLabel");

    auto* qualRow = new QHBoxLayout;
    qualRow->setSpacing(6);
    qualRow->addStretch();

    m_sleepQuality = new QComboBox(sleepCard);
    m_sleepQuality->addItems({"1","2","3","4","5"});
    m_sleepQuality->setCurrentIndex(2);
    m_sleepQuality->hide();

    QList<QPushButton*> sleepPills;
    for (int i = 1; i <= 5; ++i) {
        auto* p = makePillBtn(QString::number(i), i, sleepCard);
        if (i == 3) p->setChecked(true);
        sleepPills << p;
        qualRow->addWidget(p);
    }
    qualRow->addStretch();

    for (auto* p : sleepPills) {
        QObject::connect(p, &QPushButton::clicked, sleepCard, [this, sleepPills, p]() {
            for (auto* o : sleepPills) o->setChecked(o == p);
            m_sleepQuality->setCurrentIndex(p->property("pillValue").toInt() - 1);
        });
    }

    sleepLay->addWidget(qualLabel);
    sleepLay->addLayout(qualRow);

    // ── Triggers card (3 columns) ──────────────────────────────────────
    auto* trigCard = new QFrame(form);
    trigCard->setObjectName("SectionCard");
    auto* trigLay = new QVBoxLayout(trigCard);
    trigLay->setContentsMargins(20, 20, 20, 20);
    trigLay->setSpacing(10);

    auto* trigTitle = new QLabel(tr("Триггеры"), trigCard);
    trigTitle->setObjectName("SectionTitle");

    auto* chipHost = new QWidget(trigCard);
    auto* chipGrid = new QGridLayout(chipHost);
    chipGrid->setContentsMargins(0, 0, 0, 0);
    chipGrid->setSpacing(6);

    const QStringList triggerLabels = {
        tr("Недосып"), tr("Стресс"), tr("Пропуск лек."),
        tr("Яркий свет"), tr("Громкий шум"), tr("Алкоголь"),
        tr("Погода"), tr("Еда"), tr("Экран"), tr("Другое")
    };

    QList<QPushButton*> chipBtns;
    for (int i = 0; i < triggerLabels.size(); ++i) {
        auto* btn = makeChip(triggerLabels[i], chipHost);
        chipBtns << btn;
        chipGrid->addWidget(btn, i / 3, i % 3);  // 3 columns
    }

    trigLay->addWidget(trigTitle);
    trigLay->addWidget(chipHost);

    // ── Note ───────────────────────────────────────────────────────────
    m_triggerNote = new QTextEdit(form);
    m_triggerNote->setPlaceholderText(tr("Как прошёл день? Что чувствовал?"));
    m_triggerNote->setFixedHeight(80);

    // ── Save ───────────────────────────────────────────────────────────
    auto* saveBtn = new QPushButton(tr("Сохранить"), form);
    saveBtn->setObjectName("PrimaryButton");
    saveBtn->setMinimumHeight(48);
    saveBtn->setCursor(Qt::PointingHandCursor);

    fl->addWidget(todCard);
    fl->addWidget(moodCard);
    fl->addWidget(sleepCard);
    fl->addWidget(trigCard);
    fl->addWidget(m_triggerNote);
    fl->addWidget(saveBtn);
    fl->addStretch();

    auto* formScroll = new QScrollArea(page);
    formScroll->setWidgetResizable(true);
    formScroll->setFrameShape(QFrame::NoFrame);
    formScroll->setWidget(form);
    formScroll->setFixedWidth(440);

    // ═══ RIGHT: history ════════════════════════════════════════════════════
    m_triggerListHost = new QWidget(page);
    m_triggerListHost->setObjectName("TriggerListHost");
    m_triggerListLayout = new QVBoxLayout(m_triggerListHost);
    m_triggerListLayout->setContentsMargins(0, 0, 0, 0);
    m_triggerListLayout->setSpacing(10);

    auto* placeholder = new QLabel(
        tr("Здесь появится история самочувствия."), m_triggerListHost);
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
        for (auto* cb : chipBtns)
            if (cb->isChecked()) selected << cb->text().trimmed();
        submitTrigger();
    });

    return page;
}

// ─── Submit ─────────────────────────────────────────────────────────────────

void MainWindow::submitTriggerWithDetails(const QStringList&, const QStringList&)
{
    submitTrigger();
}

void MainWindow::submitTrigger()
{
    const QString note = m_triggerNote ? m_triggerNote->toPlainText().trimmed() : QString();
    if (note.isEmpty()) {
        showToast(m_central, tr("Напиши пару слов о самочувствии"), ToastKind::Error, 2600);
        return;
    }

    const int mood    = m_moodScore ? (m_moodScore->currentIndex() + 1) : 3;
    const double sleepH = m_sleepHours ? m_sleepHours->value() : 8.0;
    const int sleepQ  = m_sleepQuality ? (m_sleepQuality->currentIndex() + 1) : 3;

    QJsonArray kinds;
    auto chips = m_triggerNote->parentWidget()->findChildren<QPushButton*>();
    for (auto* btn : chips) {
        if (btn->isChecked() && btn->isCheckable()
            && btn->property("moodValue").isNull()
            && btn->property("pillValue").isNull())
        {
            QString t = btn->text().trimmed();
            if (!t.isEmpty() && !t.startsWith("☀") && !t.startsWith("🌙"))
                kinds.append(t);
        }
    }

    QJsonObject payload{
        {"user_id",       m_core->currentUserId()},
        {"logged_at",     m_triggerDate->dateTime().toUTC().toString(Qt::ISODate)},
        {"mood",          mood},
        {"sleep_hours",   sleepH},
        {"sleep_quality", sleepQ},
        {"trigger_kind",  kinds},
        {"note",          note},
    };

    m_core->createTrigger(payload);

    m_triggerDate->setDateTime(QDateTime::currentDateTime());
    m_triggerNote->clear();
    for (auto* btn : chips) {
        if (btn->isCheckable() && btn->property("moodValue").isNull()
            && btn->property("pillValue").isNull())
            btn->setChecked(false);
    }

    showToast(m_central, tr("Запись сохранена"), ToastKind::Success, 2000);
}

// ─── Reminders ──────────────────────────────────────────────────────────────

void MainWindow::startWellbeingReminderLoop()
{
    if (!m_wellbeingReminderTimer) {
        m_wellbeingReminderTimer = new QTimer(this);
        QObject::connect(m_wellbeingReminderTimer, &QTimer::timeout,
                         this, &MainWindow::checkWellbeingReminders);
        m_wellbeingReminderTimer->start(60'000);
    }
}

void MainWindow::checkWellbeingReminders()
{
    if (!m_trayIcon) return;
    const QTime now = QTime::currentTime();
    const QDate today = QDate::currentDate();
    static QDate lastMorning, lastEvening;

    if (now.hour() == 8 && now.minute() == 0 && lastMorning != today) {
        lastMorning = today;
        m_trayIcon->showMessage("Trackly",
            tr("Доброе утро! Отметь своё самочувствие ☀️"),
            QSystemTrayIcon::Information, 5000);
    }
    if (now.hour() == 20 && now.minute() == 0 && lastEvening != today) {
        lastEvening = today;
        m_trayIcon->showMessage("Trackly",
            tr("Добрый вечер! Как прошёл день? 🌙"),
            QSystemTrayIcon::Information, 5000);
    }
}

// ─── List refresh ───────────────────────────────────────────────────────────

void MainWindow::refreshTriggersList()
{
    if (!m_triggerListLayout || !m_triggerListHost) return;
    ::clearLayout(m_triggerListLayout);

    if (m_cachedTriggers.isEmpty()) {
        auto* empty = new QLabel(tr("Здесь появится история самочувствия."), m_triggerListHost);
        empty->setObjectName("MutedLabel");
        empty->setWordWrap(true);
        empty->setAlignment(Qt::AlignCenter);
        m_triggerListLayout->addStretch();
        m_triggerListLayout->addWidget(empty);
        m_triggerListLayout->addStretch();
        return;
    }

    const QStringList moodEmojis = {"😫", "😟", "😐", "😊", "🤩"};

    for (const auto& v : m_cachedTriggers) {
        const QJsonObject o = v.toObject();

        auto* card = new QFrame(m_triggerListHost);
        card->setObjectName("FeedCard");
        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(18, 14, 18, 14);
        cl->setSpacing(6);

        const int mood          = o.value("mood").toInt();
        const double sleepHours = o.value("sleep_hours").toDouble();
        const int sleepQuality  = o.value("sleep_quality").toInt();
        const QString noteText  = o.value("note").toString();
        const QString when      = formatTs(o.value("logged_at").toString());
        const QString moodEmoji = (mood >= 1 && mood <= 5) ? moodEmojis[mood - 1] : "😐";

        auto* header = new QHBoxLayout;
        header->setSpacing(8);

        auto* moodLbl = new QLabel(QString("%1  %2/5").arg(moodEmoji).arg(mood), card);
        moodLbl->setObjectName("BadgeLabel");

        auto* dateLbl = new QLabel(when, card);
        dateLbl->setObjectName("MutedLabel");

        header->addWidget(moodLbl);
        header->addWidget(dateLbl);
        header->addStretch();

        // trash icon button
        auto* delBtn = new QPushButton(card);
        delBtn->setIcon(QIcon(":/icons/delete.svg"));
        delBtn->setIconSize(QSize(18, 18));
        delBtn->setFixedSize(32, 32);
        delBtn->setFlat(true);
        delBtn->setCursor(Qt::PointingHandCursor);
        QObject::connect(delBtn, &QPushButton::clicked, card, [this, o]() {
            const QString id = o.value("id").toString();
            if (!id.isEmpty()) m_core->deleteTrigger(id);
        });
        header->addWidget(delBtn);

        cl->addLayout(header);

        if (sleepHours > 0) {
            QString stars;
            for (int i = 0; i < 5; ++i)
                stars += (i < sleepQuality) ? "★" : "☆";
            auto* sleepLbl = new QLabel(
                tr("Сон: %1 ч  %2").arg(sleepHours, 0, 'f', 1).arg(stars), card);
            sleepLbl->setObjectName("MutedLabel");
            cl->addWidget(sleepLbl);
        }

        const auto kinds = o.value("trigger_kind").toArray();
        if (!kinds.isEmpty()) {
            QStringList tags;
            for (const auto& kv : kinds) {
                const QString s = kv.toString().trimmed();
                if (!s.isEmpty()) tags << s;
            }
            if (!tags.isEmpty()) {
                auto* tagsLbl = new QLabel(tr("Триггеры: %1").arg(tags.join(" · ")), card);
                tagsLbl->setObjectName("MutedLabel");
                tagsLbl->setWordWrap(true);
                cl->addWidget(tagsLbl);
            }
        }

        if (!noteText.isEmpty()) {
            auto* noteLbl = new QLabel(noteText, card);
            noteLbl->setWordWrap(true);
            cl->addWidget(noteLbl);
        }

        m_triggerListLayout->addWidget(card);
    }
    m_triggerListLayout->addStretch();
}

// ─── Delete ─────────────────────────────────────────────────────────────────

void MainWindow::deleteTrigger(const QJsonObject& trigger)
{
    const QString id = trigger.value("id").toString();
    if (id.isEmpty()) return;
    if (showConfirmDialog(tr("Удаление"), tr("Удалить эту запись самочувствия?")))
        m_core->deleteTrigger(id);
}

// ─── Data fill ──────────────────────────────────────────────────────────────

void MainWindow::fillTriggers(const QJsonArray& arr)
{
    m_cachedTriggers = arr;
    refreshTriggersList();
    refreshAnalyticsPage();
}
