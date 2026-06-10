// Triggers/Wellbeing page implementation for MainWindow
// Extracted from mainwindow.cpp

#include "mainwindow.h"
#include "appcore.h"
#include "utils.h"
#include "widgets/toastwidget.h"
#include "page_includes.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTextEdit>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
// Page creation
// ---------------------------------------------------------------------------

QWidget* MainWindow::createTriggersPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(16);

    // ==== Top: form ====

    auto* formHost = new QWidget(page);
    auto* formLayout = new QVBoxLayout(formHost);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(12);

    // 1. Mood
    auto* moodCard = new QFrame(formHost);
    moodCard->setObjectName("SectionCard");
    auto* moodLayout = new QVBoxLayout(moodCard);
    moodLayout->setContentsMargins(18, 18, 18, 18);
    moodLayout->setSpacing(10);

    auto* moodTitle = new QLabel(tr("Сегодняшний настрой"), moodCard);
    moodTitle->setObjectName("SectionTitle");

    auto* moodRow = new QHBoxLayout;
    moodRow->setSpacing(10);

    m_moodScore = new QComboBox(moodCard);
    m_moodScore->addItems({
        tr("1 — очень плохо"),
        tr("2 — плохо"),
        tr("3 — нормально"),
        tr("4 — хорошо"),
        tr("5 — отлично")
    });
    m_moodScore->setCurrentIndex(2);

    m_triggerDate = new QDateTimeEdit(QDateTime::currentDateTime(), moodCard);
    m_triggerDate->setCalendarPopup(true);

    moodRow->addWidget(new QLabel(tr("Настрой"), moodCard));
    moodRow->addWidget(m_moodScore, 1);
    moodRow->addSpacing(12);
    moodRow->addWidget(new QLabel(tr("Дата/время"), moodCard));
    moodRow->addWidget(m_triggerDate, 1);

    auto* moodNoteLabel = new QLabel(tr("Коротко опиши своё состояние"), moodCard);
    moodNoteLabel->setObjectName("MutedLabel");

    m_triggerNote = new QTextEdit(moodCard);
    m_triggerNote->setPlaceholderText(tr("Например: чувствую усталость, немного тревожно"));
    m_triggerNote->setFixedHeight(80);

    moodLayout->addWidget(moodTitle);
    moodLayout->addLayout(moodRow);
    moodLayout->addWidget(moodNoteLabel);
    moodLayout->addWidget(m_triggerNote);

    // 2. Symptoms
    auto* symptomsCard = new QFrame(formHost);
    symptomsCard->setObjectName("SectionCard");
    auto* symptomsLayout = new QVBoxLayout(symptomsCard);
    symptomsLayout->setContentsMargins(18, 18, 18, 18);
    symptomsLayout->setSpacing(10);

    auto* symptomsTitle = new QLabel(tr("Симптомы"), symptomsCard);
    symptomsTitle->setObjectName("SectionTitle");

    auto* symptomsChips = new QWidget(symptomsCard);
    auto* symptomsChipsLayout = new QHBoxLayout(symptomsChips);
    symptomsChipsLayout->setContentsMargins(0, 0, 0, 0);
    symptomsChipsLayout->setSpacing(8);

    const QStringList symptomOptions = {
        tr("Головная боль"),
        tr("Головокружение"),
        tr("Тошнота"),
        tr("Сонливость"),
        tr("Тревога"),
        tr("Раздражительность")
    };

    QList<QCheckBox*> symptomChecks;
    for (const QString& label : symptomOptions) {
        auto* cb = new QCheckBox(label, symptomsChips);
        cb->setObjectName("ChipCheckBox");
        symptomChecks << cb;
        symptomsChipsLayout->addWidget(cb);
    }

    auto* otherSymptomRow = new QHBoxLayout;
    otherSymptomRow->setSpacing(8);
    auto* otherSymptomLabel = new QLabel(tr("Другой симптом"), symptomsCard);
    auto* otherSymptomEdit = new QLineEdit(symptomsCard);
    otherSymptomEdit->setPlaceholderText(tr("Например: онемение, шум в ушах"));

    otherSymptomRow->addWidget(otherSymptomLabel);
    otherSymptomRow->addWidget(otherSymptomEdit, 1);

    symptomsLayout->addWidget(symptomsTitle);
    symptomsLayout->addWidget(symptomsChips);
    symptomsLayout->addLayout(otherSymptomRow);

    // 3. Triggers
    auto* triggersCard = new QFrame(formHost);
    triggersCard->setObjectName("SectionCard");
    auto* triggersLayout = new QVBoxLayout(triggersCard);
    triggersLayout->setContentsMargins(18, 18, 18, 18);
    triggersLayout->setSpacing(10);

    auto* triggersTitle = new QLabel(tr("Триггеры"), triggersCard);
    triggersTitle->setObjectName("SectionTitle");

    auto* triggerChips = new QWidget(triggersCard);
    auto* triggerChipsLayout = new QHBoxLayout(triggerChips);
    triggerChipsLayout->setContentsMargins(0, 0, 0, 0);
    triggerChipsLayout->setSpacing(8);

    const QStringList triggerOptions = {
        tr("Недосып"),
        tr("Стресс"),
        tr("Пропуск лекарства"),
        tr("Яркий свет"),
        tr("Шум"),
        tr("Алкоголь")
    };

    QList<QCheckBox*> triggerChecks;
    for (const QString& label : triggerOptions) {
        auto* cb = new QCheckBox(label, triggerChips);
        cb->setObjectName("ChipCheckBox");
        triggerChecks << cb;
        triggerChipsLayout->addWidget(cb);
    }

    auto* otherTriggerRow = new QHBoxLayout;
    otherTriggerRow->setSpacing(8);
    auto* otherTriggerLabel = new QLabel(tr("Другой триггер"), triggersCard);
    auto* otherTriggerEdit = new QLineEdit(triggersCard);
    otherTriggerEdit->setPlaceholderText(tr("Например: жара, сильные запахи"));

    otherTriggerRow->addWidget(otherTriggerLabel);
    otherTriggerRow->addWidget(otherTriggerEdit, 1);

    triggersLayout->addWidget(triggersTitle);
    triggersLayout->addWidget(triggerChips);
    triggersLayout->addLayout(otherTriggerRow);

    // Save button
    m_saveTriggerBtn = new QPushButton(tr("Сохранить запись"), formHost);
    m_saveTriggerBtn->setObjectName("PrimaryButton");
    m_saveTriggerBtn->setIcon(QIcon(":/icons/add_circle.svg"));

    formLayout->addWidget(moodCard);
    formLayout->addWidget(symptomsCard);
    formLayout->addWidget(triggersCard);
    formLayout->addWidget(m_saveTriggerBtn, 0, Qt::AlignRight);

    // ==== Bottom: history ====

    m_triggerListHost = new QWidget(page);
    m_triggerListHost->setObjectName("TriggerListHost");
    m_triggerListLayout = new QVBoxLayout(m_triggerListHost);
    m_triggerListLayout->setContentsMargins(0, 0, 0, 0);
    m_triggerListLayout->setSpacing(10);

    auto* placeholder = new QLabel(tr("После добавления записей здесь появится история самочувствия."), m_triggerListHost);
    placeholder->setObjectName("MutedLabel");
    placeholder->setWordWrap(true);
    m_triggerListLayout->addWidget(placeholder);
    m_triggerListLayout->addStretch();

    auto* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(m_triggerListHost);

    auto* contentRow = new QHBoxLayout;
    contentRow->setSpacing(16);
    contentRow->addWidget(formHost, 2);
    contentRow->addWidget(scroll, 3);

    root->addLayout(contentRow, 1);

    // Wire: save selected checkboxes via lambda in submitTriggerWithDetails
    connect(m_saveTriggerBtn, &QPushButton::clicked, this, [this, symptomChecks, otherSymptomEdit, triggerChecks, otherTriggerEdit]() {
        QStringList symptoms;
        for (auto* cb : symptomChecks) {
            if (cb->isChecked()) symptoms << cb->text().trimmed();
        }
        if (!otherSymptomEdit->text().trimmed().isEmpty())
            symptoms << otherSymptomEdit->text().trimmed();

        QStringList triggers;
        for (auto* cb : triggerChecks) {
            if (cb->isChecked()) triggers << cb->text().trimmed();
        }
        if (!otherTriggerEdit->text().trimmed().isEmpty())
            triggers << otherTriggerEdit->text().trimmed();

        submitTriggerWithDetails(symptoms, triggers);
    });

    return page;
}

// ---------------------------------------------------------------------------
// Submit
// ---------------------------------------------------------------------------

void MainWindow::submitTriggerWithDetails(const QStringList& symptoms,
                                          const QStringList& triggers)
{
    const QString note = m_triggerNote ? m_triggerNote->toPlainText().trimmed() : QString();

    if (!m_moodScore) {
        showToast(m_central, tr("Форма настроения ещё не готова"), ToastKind::Error, 2600);
        return;
    }

    const int moodValue = m_moodScore->currentIndex() + 1;
    if (moodValue < 1 || moodValue > 5) {
        showToast(m_central, tr("Выбери настрой от 1 до 5"), ToastKind::Error, 2600);
        return;
    }

    if (symptoms.isEmpty() && triggers.isEmpty() && note.isEmpty()) {
        showToast(m_central, tr("Добавь хотя бы симптом, триггер или комментарий"), ToastKind::Error, 2600);
        return;
    }

    QJsonArray symptomsArray;
    for (const auto& s : symptoms) {
        const auto st = s.trimmed();
        if (!st.isEmpty())
            symptomsArray.append(st);
    }

    QJsonArray triggersArray;
    for (const auto& t : triggers) {
        const auto tt = t.trimmed();
        if (!tt.isEmpty())
            triggersArray.append(tt);
    }

    QJsonObject payload{
        {"user_id", m_core->currentUserId()},
        {"logged_at", m_triggerDate->dateTime().toString(Qt::ISODate)},
        {"mood", moodValue},
        {"note", note},
        {"symptoms", symptomsArray},
        {"triggers", triggersArray}
    };

    m_core->createTrigger(payload);

    m_triggerDate->setDateTime(QDateTime::currentDateTime());
    m_moodScore->setCurrentIndex(2);
    if (m_triggerNote) m_triggerNote->clear();
}

// ---------------------------------------------------------------------------
// List refresh
// ---------------------------------------------------------------------------

void MainWindow::refreshTriggersList()
{
    if (!m_triggerListLayout || !m_triggerListHost) {
        return;
    }

    ::clearLayout(m_triggerListLayout);

    if (m_cachedTriggers.isEmpty()) {
        auto* empty = new QLabel(tr("После добавления записей здесь появится история самочувствия."), m_triggerListHost);
        empty->setObjectName("MutedLabel");
        empty->setWordWrap(true);
        m_triggerListLayout->addWidget(empty);
        m_triggerListLayout->addStretch();
        return;
    }

    for (const auto& v : m_cachedTriggers) {
        const QJsonObject o = v.toObject();

        auto* card = new QFrame(m_triggerListHost);
        card->setObjectName("FeedCard");

        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(16, 16, 16, 16);
        cl->setSpacing(8);

        const int mood = o.value("mood").toInt();
        const double sleepHours = o.value("sleep_hours").toDouble();
        const int sleepQuality = o.value("sleep_quality").toInt();

        auto* title = new QLabel(o.value("trigger_title").toString().trimmed(), card);
        title->setObjectName("CardTitle");

        auto* meta = new QLabel(
            tr("%1 · Настрой %2/5 · Сон %3 ч · Качество сна %4/5")
                .arg(formatTs(o.value("logged_at").toString()))
                .arg(mood)
                .arg(QString::number(sleepHours, 'f', 1))
                .arg(sleepQuality),
            card
            );
        meta->setObjectName("MutedLabel");
        meta->setWordWrap(true);

        auto* kind = new QLabel(o.value("trigger_kind").toString().trimmed(), card);
        kind->setObjectName("BadgeLabel");

        QStringList tags;
        const auto arrTags = o.value("triggers").toArray();
        for (const auto& tv : arrTags) {
            const QString s = tv.toString().trimmed();
            if (!s.isEmpty()) {
                tags << s;
            }
        }

        if (!tags.isEmpty()) {
            auto* tagsLabel = new QLabel(tr("Триггеры: %1").arg(tags.join(", ")), card);
            tagsLabel->setObjectName("MutedLabel");
            tagsLabel->setWordWrap(true);
            cl->addWidget(title);
            cl->addWidget(meta);
            cl->addWidget(kind, 0, Qt::AlignLeft);
            cl->addWidget(tagsLabel);

            const QString noteText = o.value("note").toString().trimmed();
            if (!noteText.isEmpty()) {
                auto* note = new QLabel(noteText, card);
                note->setWordWrap(true);
                cl->addWidget(note);
            }
        } else {
            cl->addWidget(title);
            cl->addWidget(meta);
            cl->addWidget(kind, 0, Qt::AlignLeft);

            const QString noteText = o.value("note").toString().trimmed();
            if (!noteText.isEmpty()) {
                auto* note = new QLabel(noteText, card);
                note->setWordWrap(true);
                cl->addWidget(note);
            }
        }

        auto* deleteBtn = new QPushButton(tr("Удалить"), card);
        deleteBtn->setObjectName("SecondaryButton");
        connect(deleteBtn, &QPushButton::clicked, this, [this, o]() {
            const QString id = o.value("id").toString();
            if (!id.isEmpty()) {
                m_core->deleteTrigger(id);
            }
        });

        cl->addSpacing(4);
        cl->addWidget(deleteBtn, 0, Qt::AlignRight);

        m_triggerListLayout->addWidget(card);
    }

    m_triggerListLayout->addStretch();
}

// ---------------------------------------------------------------------------
// Delete
// ---------------------------------------------------------------------------

void MainWindow::deleteTrigger(const QJsonObject& trigger)
{
    const QString id = trigger.value("id").toString();
    if (id.isEmpty())
        return;

    const QString title = trigger.value("title").toString().trimmed().isEmpty()
                              ? trigger.value("kind").toString()
                              : trigger.value("title").toString();

    const auto res = QMessageBox::question(
        this,
        tr("Удаление"),
        tr("Удалить запись \"%1\"?").arg(title),
        QMessageBox::Yes | QMessageBox::No
        );

    if (res == QMessageBox::Yes)
        m_core->deleteTrigger(id);
}

// ---------------------------------------------------------------------------
// Legacy submit (unused but kept for compatibility)
// ---------------------------------------------------------------------------

void MainWindow::submitTrigger()
{
    const QString title = m_triggerTitle ? m_triggerTitle->text().trimmed() : QString();
    const QString note = m_triggerNote ? m_triggerNote->toPlainText().trimmed() : QString();
    const QString tagsRaw = m_triggerTags ? m_triggerTags->text().trimmed() : QString();
    const QString kind = m_triggerKind ? m_triggerKind->currentText().trimmed() : QString();

    if (title.isEmpty()) {
        showToast(m_central, tr("Введите короткое название записи самочувствия"), ToastKind::Error, 2600);
        return;
    }

    if (m_sleepHours && m_sleepHours->value() <= 0.0) {
        showToast(m_central, tr("Укажите количество часов сна"), ToastKind::Error, 2600);
        return;
    }

    QJsonArray triggerArray;
    const QStringList parts = tagsRaw.split(',', Qt::SkipEmptyParts);
    for (QString part : parts) {
        part = part.trimmed();
        if (!part.isEmpty()) {
            triggerArray.append(part);
        }
    }

    QJsonObject payload{
        {"user_id", m_core->currentUserId()},
        {"logged_at", m_triggerDate->dateTime().toString(Qt::ISODate)},
        {"mood", m_moodScore->currentIndex() + 1},
        {"sleep_hours", m_sleepHours->value()},
        {"sleep_quality", m_sleepQuality->currentIndex() + 1},
        {"trigger_kind", kind.isEmpty() ? tr("Общее") : kind},
        {"trigger_title", title},
        {"note", note},
        {"triggers", triggerArray}
    };

    m_core->createTrigger(payload);

    m_triggerDate->setDateTime(QDateTime::currentDateTime());
    m_moodScore->setCurrentIndex(2);
    m_sleepHours->setValue(8.0);
    m_sleepQuality->setCurrentIndex(2);
    m_triggerKind->setCurrentIndex(0);
    m_triggerTitle->clear();
    m_triggerTags->clear();
    m_triggerNote->clear();
}

// ---------------------------------------------------------------------------
// Data fill
// ---------------------------------------------------------------------------

void MainWindow::fillTriggers(const QJsonArray& arr)
{
    m_cachedTriggers = arr;
    refreshTriggersList();
    refreshAnalyticsPage();
}
