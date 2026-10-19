// Dashboard page implementation for MainWindow
// Extracted from mainwindow.cpp

#include "mainwindow.h"
#include "appcore.h"
#include "utils.h"
#include "widgets/toastwidget.h"
#include "page_includes.h"

#include <QCalendarWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimeEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

// --- createDashboardPage ---
QWidget* MainWindow::createDashboardPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(16);

    auto* row = new QHBoxLayout;
    row->setSpacing(14);

    auto* c1 = qobject_cast<QPushButton*>(makeSummaryCard(tr("Приступы"), tr("0 за текущий месяц"), &m_summary1));
    auto* c2 = qobject_cast<QPushButton*>(makeSummaryCard(tr("Лекарства"), tr("0 приемов сегодня"), &m_summary2));
    auto* c3 = qobject_cast<QPushButton*>(makeSummaryCard(tr("Обследования"), tr("Нет ближайших записей"), &m_summary3));
    auto* c4 = qobject_cast<QPushButton*>(makeSummaryCard(tr("Врач / профиль"), tr("Не указан"), &m_summary4));

    row->addWidget(c1);
    row->addWidget(c2);
    row->addWidget(c3);
    row->addWidget(c4);
    root->addLayout(row);

    m_dashboardCalendar = new TracklyCalendar(page);
    m_dashboardCalendar->setObjectName("MainCalendar");
    m_dashboardCalendar->setMode(0); // дашборд: приступы + обследования

    m_dashboardExamHost = new QWidget(page);
    m_dashboardExamHost->setObjectName("DashboardExamHost");
    m_dashboardExamLayout = new QVBoxLayout(m_dashboardExamHost);
    m_dashboardExamLayout->setContentsMargins(0, 0, 0, 0);
    m_dashboardExamLayout->setSpacing(10);

    auto* empty = new QLabel(tr("Здесь появятся ближайшие обследования."), m_dashboardExamHost);
    empty->setObjectName("MutedLabel");
    empty->setWordWrap(true);
    m_dashboardExamLayout->addWidget(empty);
    m_dashboardExamLayout->addStretch();

    auto* bottom = new QHBoxLayout;
    bottom->setSpacing(16);
    bottom->addWidget(makeSection(tr("Календарь"), m_dashboardCalendar), 3);
    bottom->addWidget(makeSection(tr("Ближайшие обследования"), m_dashboardExamHost), 2);

    root->addLayout(bottom, 1);

    connect(c1, &QPushButton::clicked, this, [this]() { navigateTo(1, tr("Приступы")); });
    connect(c2, &QPushButton::clicked, this, [this]() { navigateTo(2, tr("Лекарства")); });
    connect(c3, &QPushButton::clicked, this, [this]() { navigateTo(3, tr("Обследования")); });
    connect(c4, &QPushButton::clicked, this, [this]() { navigateTo(6, tr("Профиль")); });

    return page;
}


// --- updateDashboard ---
void MainWindow::updateDashboard(const QJsonObject& obj)
{
    if (m_summary1) {
        m_summary1->setText(tr("%1 за текущий месяц").arg(obj.value("seizures_this_month").toInt()));
    }

    if (m_summary2) {
        m_summary2->setText(tr("%1 приемов сегодня").arg(obj.value("meds_today").toInt()));
    }

    if (m_summary3) {
        const int examsThisMonth = obj.value("exams_this_month").toInt();
        m_summary3->setText(examsThisMonth > 0
            ? tr("%1 за текущий месяц").arg(examsThisMonth)
            : tr("Нет за текущий месяц"));
    }

    if (m_summary4) {
        const QString doctor = obj.value("doctor").toString();
        m_summary4->setText(doctor.isEmpty() ? tr("Не указан") : doctor);
    }
}


// --- updateDashboardCalendarDecorations ---
void MainWindow::updateDashboardCalendarDecorations()
{
    // строим DayStats по дням
    QMap<QDate, DayStats> stats;

    for (const auto& v : std::as_const(m_cachedSeizureItems)) {
        QDateTime dt = QDateTime::fromString(v.toObject().value("started_at").toString(), Qt::ISODate);
        if (dt.isValid()) stats[dt.date()].seizures++;
    }
    for (const auto& v : std::as_const(m_cachedExaminations)) {
        QDateTime dt = QDateTime::fromString(v.toObject().value("planned_at").toString(), Qt::ISODate);
        if (dt.isValid()) stats[dt.date()].exams++;
    }
    for (const auto& v : std::as_const(m_cachedMedicationLogs)) {
        const QJsonObject o = v.toObject();
        QDateTime dt = QDateTime::fromString(o.value("taken_at").toString(), Qt::ISODate);
        if (!dt.isValid()) continue;
        const QString st = o.value("status").toString().toLower();
        if (st == "taken") stats[dt.date()].taken++;
        else if (st == "missed") stats[dt.date()].missed++;
    }

    // запланированные приёмы из schedules
    const QDate today = QDate::currentDate();
    for (const auto& sv : std::as_const(m_cachedMedicationItems)) {
        const QJsonObject s = sv.toObject();
        const QString medId = s.value("medication_id").toString();
        if (medId.isEmpty()) continue;
        if (!m_medicationScheduleDates.contains(medId)) continue;
        const auto& range = m_medicationScheduleDates[medId];
        QDate cursor = range.first.isValid() ? qMax(range.first, today) : today;
        const QDate end = range.second.isValid() ? range.second : today.addMonths(3);
        for (; cursor <= end; cursor = cursor.addDays(1)) {
            stats[cursor].planned++;
        }
    }

    if (m_dashboardCalendar)
        m_dashboardCalendar->setDayStats(stats);
    if (m_medCalendar)
        m_medCalendar->setDayStats(stats);
}


// --- refreshDashboardDayDetails ---
void MainWindow::refreshDashboardDayDetails()
{
    if (!m_dashboardCalendar || !m_dashboardExamLayout) return;
    ::clearLayout(m_dashboardExamLayout);

    // собираем предстоящие обследования (сегодня + будущие, с учётом времени)
    struct ExamItem { QDate date; QDateTime dt; QString type, subtype, notes; };
    QList<ExamItem> upcoming;
    const QDateTime now = QDateTime::currentDateTime();
    for (const auto& v : std::as_const(m_cachedExaminations)) {
        const QJsonObject o = v.toObject();
        QDateTime dt = QDateTime::fromString(o.value("planned_at").toString(), Qt::ISODate);
        if (!dt.isValid()) continue;
        if (dt < now) continue;
        const QDate d = dt.date();
        upcoming.append({d, dt, o.value("exam_type").toString(),
                         o.value("exam_subtype").toString(),
                         o.value("notes").toString().trimmed()});
    }
    std::sort(upcoming.begin(), upcoming.end(), [](const auto& a, const auto& b) { return a.date < b.date; });
    if (upcoming.size() > 8) upcoming = upcoming.mid(0, 8);

    if (upcoming.isEmpty()) {
        auto* empty = new QLabel(tr("Нет предстоящих обследований"), m_dashboardExamHost);
        empty->setObjectName("MutedLabel"); empty->setWordWrap(true);
        m_dashboardExamLayout->addWidget(empty);
    } else {
        for (const auto& item : upcoming) {
            auto* card = new QFrame(m_dashboardExamHost);
            card->setObjectName("FeedCard");
            auto* cl = new QVBoxLayout(card);
            cl->setContentsMargins(16, 12, 16, 12);
            cl->setSpacing(4);

            const QString title = item.subtype.isEmpty() ? item.type : item.type + " — " + item.subtype;
            auto* nameLbl = new QLabel(title, card);
            nameLbl->setObjectName("CardTitle"); nameLbl->setWordWrap(true);

            auto* dateLbl = new QLabel(item.date.toString("dd.MM.yyyy") + "  " + item.dt.time().toString("HH:mm"), card);
            dateLbl->setObjectName("MutedLabel");

            QString badge;
            if (item.date == now.date())
                badge = tr("Сегодня");
            else if (item.date == now.date().addDays(1))
                badge = tr("Завтра");
            else
                badge = tr("Предстоит");
            auto* badgeLbl = new QLabel(badge, card);
            badgeLbl->setObjectName("BadgeLabel");

            auto* headerRow = new QHBoxLayout;
            headerRow->setSpacing(8);
            headerRow->addWidget(nameLbl, 1);
            headerRow->addWidget(badgeLbl, 0, Qt::AlignRight | Qt::AlignTop);
            cl->addLayout(headerRow);
            cl->addWidget(dateLbl);

            if (!item.notes.isEmpty()) {
                auto* notesLbl = new QLabel(item.notes, card);
                notesLbl->setWordWrap(true);
                notesLbl->setObjectName("MutedLabel");
                cl->addWidget(notesLbl);
            }

            m_dashboardExamLayout->addWidget(card);
        }
    }
    m_dashboardExamLayout->addStretch();
}


