// Analytics page implementation for MainWindow
// Extracted from mainwindow.cpp — methods retain MainWindow:: prefix

#include "mainwindow.h"
#include "appcore.h"
#include "utils.h"
#include "widgets/toastwidget.h"
#include "page_includes.h"

#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QChartView>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineSeries>
#include <QMap>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QValueAxis>
#include <QVBoxLayout>
#include <algorithm>

// ---------------------------------------------------------------------------
// Analytics page creation
// ---------------------------------------------------------------------------

QWidget* MainWindow::createAnalyticsPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(16);

    // ─── Селектор периода ───
    auto* periodRow = new QHBoxLayout;
    periodRow->setSpacing(10);

    m_analyticsPeriodCombo = new QComboBox(page);
    m_analyticsPeriodCombo->addItem(tr("7 дней"), 7);
    m_analyticsPeriodCombo->addItem(tr("30 дней"), 30);
    m_analyticsPeriodCombo->addItem(tr("90 дней"), 90);
    m_analyticsPeriodCombo->addItem(tr("Всё время"), -1);
    m_analyticsPeriodCombo->addItem(tr("Свой период"), 0);
    m_analyticsPeriodCombo->setCurrentIndex(1); // 30 дней по умолчанию

    m_analyticsCustomRange = new QWidget(page);
    auto* customLayout = new QHBoxLayout(m_analyticsCustomRange);
    customLayout->setContentsMargins(0, 0, 0, 0);
    customLayout->setSpacing(6);

    auto* fromLbl = new QLabel(tr("с"), m_analyticsCustomRange);
    m_analyticsFromDate = new QDateEdit(QDate::currentDate().addDays(-29), m_analyticsCustomRange);
    m_analyticsFromDate->setCalendarPopup(true);
    m_analyticsFromDate->setDisplayFormat("dd.MM.yyyy");

    auto* toLbl = new QLabel(tr("по"), m_analyticsCustomRange);
    m_analyticsToDate = new QDateEdit(QDate::currentDate(), m_analyticsCustomRange);
    m_analyticsToDate->setCalendarPopup(true);
    m_analyticsToDate->setDisplayFormat("dd.MM.yyyy");

    customLayout->addWidget(fromLbl);
    customLayout->addWidget(m_analyticsFromDate);
    customLayout->addWidget(toLbl);
    customLayout->addWidget(m_analyticsToDate);
    m_analyticsCustomRange->setVisible(false);

    periodRow->addWidget(new QLabel(tr("Период:"), page));
    periodRow->addWidget(m_analyticsPeriodCombo);
    periodRow->addWidget(m_analyticsCustomRange);
    periodRow->addStretch();

    connect(m_analyticsPeriodCombo, &QComboBox::currentIndexChanged,
            this, [this]() {
        m_analyticsCustomRange->setVisible(
            m_analyticsPeriodCombo->currentData().toInt() == 0);
        refreshAnalyticsPage();
    });

    connect(m_analyticsFromDate, &QDateEdit::dateChanged,
            this, &MainWindow::refreshAnalyticsPage);
    connect(m_analyticsToDate, &QDateEdit::dateChanged,
            this, &MainWindow::refreshAnalyticsPage);

    // ─── Основной контент ───
    m_analyticsHost = new QWidget(page);
    m_analyticsHost->setObjectName("AnalyticsHost");
    m_analyticsLayout = new QVBoxLayout(m_analyticsHost);
    m_analyticsLayout->setContentsMargins(0, 0, 0, 0);
    m_analyticsLayout->setSpacing(16);

    m_analyticsLayout->addWidget(
        makeAnalyticsEmptyState(tr("После загрузки данных здесь появится аналитика.")));
    m_analyticsLayout->addStretch();

    auto* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(m_analyticsHost);

    root->addLayout(periodRow);
    root->addWidget(scroll, 1);

    return page;
}

// ---------------------------------------------------------------------------
// Analytics helper widgets
// ---------------------------------------------------------------------------

QFrame* MainWindow::makeAnalyticsStatCard(const QString& title,
                                          const QString& value,
                                          const QString& hint)
{
    auto* card = new QFrame(m_analyticsHost);
    card->setObjectName("SectionCard");

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(8);

    auto* titleLabel = new QLabel(title, card);
    titleLabel->setObjectName("MutedLabel");

    auto* valueLabel = new QLabel(value, card);
    valueLabel->setObjectName("PageTitle");
    valueLabel->setWordWrap(true);

    auto* hintLabel = new QLabel(hint, card);
    hintLabel->setObjectName("MutedLabel");
    hintLabel->setWordWrap(true);

    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    layout->addStretch();
    layout->addWidget(hintLabel);

    return card;
}

QWidget* MainWindow::makeAnalyticsEmptyState(const QString& text)
{
    auto* host = new QFrame(m_analyticsHost);
    host->setObjectName("SectionCard");

    auto* layout = new QVBoxLayout(host);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(8);

    auto* title = new QLabel(tr("Недостаточно данных"), host);
    title->setObjectName("SectionTitle");

    auto* body = new QLabel(text, host);
    body->setObjectName("MutedLabel");
    body->setWordWrap(true);

    layout->addWidget(title);
    layout->addWidget(body);

    return host;
}

QWidget* MainWindow::makeAnalyticsInsightsCard(const QStringList& insights)
{
    auto* host = new QFrame(m_analyticsHost);
    host->setObjectName("SectionCard");

    auto* layout = new QVBoxLayout(host);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(10);

    auto* title = new QLabel(tr("Выводы"), host);
    title->setObjectName("SectionTitle");
    layout->addWidget(title);

    if (insights.isEmpty()) {
        auto* empty = new QLabel(tr("Недостаточно данных для выводов."), host);
        empty->setObjectName("MutedLabel");
        empty->setWordWrap(true);
        layout->addWidget(empty);
        return host;
    }

    for (const QString& line : insights) {
        auto* label = new QLabel(QStringLiteral("• %1").arg(line), host);
        label->setWordWrap(true);
        layout->addWidget(label);
    }

    return host;
}

// ---------------------------------------------------------------------------
// Charts
// ---------------------------------------------------------------------------

QChartView* MainWindow::makeMoodChart(const QList<QPair<QDate, double>>& points)
{
    auto* series = new QLineSeries;
    QStringList categories;

    int index = 0;
    for (const auto& point : points) {
        categories << point.first.toString("dd.MM");
        series->append(index++, point.second);
    }

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->legend()->hide();
    chart->setTitle(tr("Средний настрой по дням"));
    chart->setAnimationOptions(QChart::SeriesAnimations);

    auto* axisX = new QBarCategoryAxis;
    axisX->append(categories);

    auto* axisY = new QValueAxis;
    axisY->setRange(1.0, 5.0);
    axisY->setTickCount(5);
    axisY->setLabelFormat("%.1f");

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    auto pen = series->pen();
    pen.setWidth(3);
    pen.setColor(isDarkTheme() ? QColor("#8E74FF") : QColor("#7C5CFA"));
    series->setPen(pen);
    chart->setBackgroundBrush(isDarkTheme() ? QColor("#1F1A2E") : QColor("#FCFBFF"));
    chart->setTitleBrush(isDarkTheme() ? QColor("#E2E8F0") : QColor("#231A34"));
    axisX->setLabelsColor(isDarkTheme() ? QColor("#94A3B8") : QColor("#64748B"));
    axisY->setLabelsColor(isDarkTheme() ? QColor("#94A3B8") : QColor("#64748B"));

    auto* view = new QChartView(chart, m_analyticsHost);
    view->setRenderHint(QPainter::Antialiasing);
    view->setMinimumHeight(280);
    return view;
}

QChartView* MainWindow::makeBarChart(const QString& chartTitle,
                                     const QList<QPair<QString, int>>& items,
                                     const QColor& color,
                                     int maxItems)
{
    auto* set = new QBarSet(chartTitle);
    set->setColor(color);

    QStringList categories;
    int count = 0;
    int maxValue = 0;

    for (const auto& item : items) {
        if (count >= maxItems)
            break;

        categories << item.first;
        *set << item.second;
        maxValue = qMax(maxValue, item.second);
        ++count;
    }

    auto* series = new QBarSeries;
    series->append(set);

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->setTitle(chartTitle);
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::SeriesAnimations);

    auto* axisX = new QBarCategoryAxis;
    axisX->append(categories);

    auto* axisY = new QValueAxis;
    axisY->setRange(0, qMax(1, maxValue + 1));
    axisY->setTickCount(qMin(6, qMax(2, maxValue + 2)));
    axisY->setLabelFormat("%d");

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    chart->setBackgroundBrush(isDarkTheme() ? QColor("#1F1A2E") : QColor("#FCFBFF"));
    chart->setTitleBrush(isDarkTheme() ? QColor("#E2E8F0") : QColor("#231A34"));
    axisX->setLabelsColor(isDarkTheme() ? QColor("#94A3B8") : QColor("#64748B"));
    axisY->setLabelsColor(isDarkTheme() ? QColor("#94A3B8") : QColor("#64748B"));
    chart->legend()->setLabelColor(isDarkTheme() ? QColor("#CBD5E1") : QColor("#334155"));

    auto* view = new QChartView(chart, m_analyticsHost);
    view->setRenderHint(QPainter::Antialiasing);
    view->setMinimumHeight(300);
    return view;
}

QChartView* MainWindow::makeAdherenceChart(const QList<QPair<QString, QPair<int, int>>>& dailyStats)
{
    auto* takenSet = new QBarSet(tr("Принято"));
    auto* plannedSet = new QBarSet(tr("Запланировано"));
    takenSet->setColor(isDarkTheme() ? QColor("#74AA9C") : QColor("#5E9A8C"));
    plannedSet->setColor(isDarkTheme() ? QColor("#F87171") : QColor("#DC2626"));

    QStringList categories;
    int maxVal = 1;

    for (const auto& day : dailyStats) {
        categories << day.first;
        *takenSet << day.second.first;
        *plannedSet << day.second.second;
        maxVal = qMax(maxVal, qMax(day.second.first, day.second.second));
    }

    auto* series = new QBarSeries;
    series->append(takenSet);
    series->append(plannedSet);

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->setTitle(tr("Принято / запланировано по дням"));
    chart->setAnimationOptions(QChart::SeriesAnimations);

    auto* axisX = new QBarCategoryAxis;
    axisX->append(categories);

    auto* axisY = new QValueAxis;
    axisY->setRange(0, qMax(1, maxVal + 1));
    axisY->setTickCount(qMin(6, qMax(2, maxVal + 2)));
    axisY->setLabelFormat("%d");

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    chart->setBackgroundBrush(isDarkTheme() ? QColor("#1F1A2E") : QColor("#FCFBFF"));
    chart->setTitleBrush(isDarkTheme() ? QColor("#E2E8F0") : QColor("#231A34"));
    axisX->setLabelsColor(isDarkTheme() ? QColor("#94A3B8") : QColor("#64748B"));
    axisY->setLabelsColor(isDarkTheme() ? QColor("#94A3B8") : QColor("#64748B"));
    chart->legend()->setLabelColor(isDarkTheme() ? QColor("#CBD5E1") : QColor("#334155"));

    auto* view = new QChartView(chart, m_analyticsHost);
    view->setRenderHint(QPainter::Antialiasing);
    view->setMinimumHeight(300);
    return view;
}

QChartView* MainWindow::makeSeizureFrequencyChart(const QMap<QDate, int>& byDay)
{
    auto* set = new QBarSet(tr("Приступы"));
    set->setColor(isDarkTheme() ? QColor("#F87171") : QColor("#DC2626"));

    QStringList categories;
    int maxVal = 1;
    QList<QDate> sortedDays = byDay.keys();
    std::sort(sortedDays.begin(), sortedDays.end());

    for (const QDate& d : sortedDays) {
        const int count = byDay.value(d);
        categories << d.toString("dd.MM");
        *set << count;
        maxVal = qMax(maxVal, count);
    }

    auto* series = new QBarSeries;
    series->append(set);

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->legend()->hide();
    chart->setTitle(tr("Количество приступов по дням"));
    chart->setAnimationOptions(QChart::SeriesAnimations);

    auto* axisX = new QBarCategoryAxis;
    axisX->append(categories);

    auto* axisY = new QValueAxis;
    axisY->setRange(0, qMax(1, maxVal + 1));
    axisY->setTickCount(qMin(6, qMax(2, maxVal + 2)));
    axisY->setLabelFormat("%d");

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    chart->setBackgroundBrush(isDarkTheme() ? QColor("#1F1A2E") : QColor("#FCFBFF"));
    chart->setTitleBrush(isDarkTheme() ? QColor("#E2E8F0") : QColor("#231A34"));
    axisX->setLabelsColor(isDarkTheme() ? QColor("#94A3B8") : QColor("#64748B"));
    axisY->setLabelsColor(isDarkTheme() ? QColor("#94A3B8") : QColor("#64748B"));

    auto* view = new QChartView(chart, m_analyticsHost);
    view->setRenderHint(QPainter::Antialiasing);
    view->setMinimumHeight(280);
    return view;
}

// ---------------------------------------------------------------------------
// Refresh analytics
// ---------------------------------------------------------------------------

void MainWindow::refreshAnalyticsPage()
{
    if (!m_analyticsLayout || !m_analyticsHost)
        return;

    ::clearLayout(m_analyticsLayout);

    if (m_cachedSeizureItems.isEmpty() &&
        m_cachedTriggers.isEmpty() &&
        m_cachedMedicationLogs.isEmpty()) {
        m_analyticsLayout->addWidget(
            makeAnalyticsEmptyState(tr("Добавь приступы, записи самочувствия или приёмы лекарств, чтобы увидеть аналитику.")));
        m_analyticsLayout->addStretch();
        return;
    }

    // ─── Определяем период ───
    QDate from, to;
    const QDate today = QDate::currentDate();

    if (m_analyticsPeriodCombo) {
        const int periodDays = m_analyticsPeriodCombo->currentData().toInt();
        if (periodDays == 0) {
            // свой период
            from = m_analyticsFromDate ? m_analyticsFromDate->date() : today.addDays(-29);
            to   = m_analyticsToDate   ? m_analyticsToDate->date()   : today;
        } else if (periodDays == -1) {
            // всё время
            from = QDate(2000, 1, 1);
            to   = today;
        } else {
            // N дней
            from = today.addDays(-(periodDays - 1));
            to   = today;
        }
    } else {
        from = today.addDays(-29);
        to   = today;
    }

    const QString periodLabel = (m_analyticsPeriodCombo)
        ? m_analyticsPeriodCombo->currentText()
        : tr("30 дней");

    int seizuresCount = 0;
    int wellbeingCount = 0;
    int medLogCount = 0;
    int medLogTaken = 0;
    int medLogMissed = 0;
    double moodSum = 0.0;
    int moodCount = 0;

    QMap<QDate, QPair<double, int>> moodAggByDay;
    QMap<QString, int> symptomFreq;
    QMap<QString, int> triggerFreq;
    QSet<QDate> seizureDays;
    QSet<QDate> wellbeingDays;
    QMap<QDate, int> seizureByDay;
    // adherence: day → (taken, total_slots)
    QMap<QDate, QPair<int, int>> adherenceByDay;

    for (const auto& v : m_cachedSeizureItems) {
        const QJsonObject o = v.toObject();
        QDateTime dt = QDateTime::fromString(o.value("started_at").toString(), Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(o.value("started_at").toString(), Qt::ISODateWithMs);
        if (!dt.isValid())
            continue;

        const QDate d = dt.date();
        if (d >= from && d <= to) {
            ++seizuresCount;
            seizureDays.insert(d);
            seizureByDay[d]++;
        }
    }

    for (const auto& v : m_cachedMedicationLogs) {
        const QJsonObject o = v.toObject();
        QDateTime dt = QDateTime::fromString(o.value("taken_at").toString(), Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(o.value("taken_at").toString(), Qt::ISODateWithMs);
        if (!dt.isValid())
            dt = QDateTime::fromString(o.value("takenat").toString(), Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(o.value("takenat").toString(), Qt::ISODateWithMs);
        if (!dt.isValid())
            continue;

        const QDate d = dt.date();
        if (d < from || d > to)
            continue;

        ++medLogCount;

        const QString status = o.value("status").toString().trimmed().toLower();
        if (status == "taken")
            ++medLogTaken;
        else if (status == "missed")
            ++medLogMissed;

        // adherence: подсчёт planned + taken для этого дня
        // (planned определяется по наличию schedule_id, taken — по статусу)
        auto& adh = adherenceByDay[d];
        if (!o.value("schedule_id").isNull()) {
            adh.second++; // total planned
            if (status == "taken")
                adh.first++; // taken
        }
    }

    for (const auto& v : m_cachedTriggers) {
        const QJsonObject o = v.toObject();
        QDateTime dt = QDateTime::fromString(o.value("logged_at").toString(), Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(o.value("logged_at").toString(), Qt::ISODateWithMs);
        if (!dt.isValid())
            continue;

        const QDate d = dt.date();
        if (d < from || d > to)
            continue;

        wellbeingDays.insert(d);

        const int mood = o.value("mood").toInt();
        if (mood >= 1 && mood <= 5) {
            moodSum += mood;
            ++moodCount;
            ++wellbeingCount; // считаем только записи с настроением

            auto agg = moodAggByDay.value(d, qMakePair(0.0, 0));
            agg.first += mood;
            agg.second += 1;
            moodAggByDay[d] = agg;
        }

        const QJsonValue symptomsVal = o.value("symptoms");
        if (symptomsVal.isArray()) {
            for (const auto& sv : symptomsVal.toArray()) {
                const QString key = sv.toString().trimmed();
                if (!key.isEmpty())
                    symptomFreq[key] += 1;
            }
        }

        const QJsonValue triggersVal = o.value("triggers");
        if (triggersVal.isArray()) {
            for (const auto& tv : triggersVal.toArray()) {
                const QString key = tv.toString().trimmed();
                if (!key.isEmpty())
                    triggerFreq[key] += 1;
            }
        }
    }

    const double avgMood = moodCount > 0 ? moodSum / moodCount : 0.0;

    QList<QPair<QDate, double>> moodPoints;
    for (auto it = moodAggByDay.cbegin(); it != moodAggByDay.cend(); ++it) {
        if (it.value().second <= 0)
            continue;
        moodPoints.append({it.key(), it.value().first / it.value().second});
    }
    std::sort(moodPoints.begin(), moodPoints.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    auto sortFreqDesc = [](const QMap<QString, int>& src) {
        QList<QPair<QString, int>> out;
        for (auto it = src.cbegin(); it != src.cend(); ++it)
            out.append({it.key(), it.value()});
        std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
            if (a.second != b.second)
                return a.second > b.second;
            return a.first < b.first;
        });
        return out;
    };

    const QList<QPair<QString, int>> topSymptoms = sortFreqDesc(symptomFreq);
    const QList<QPair<QString, int>> topTriggers = sortFreqDesc(triggerFreq);

    // adherence points (sorted by day)
    QList<QPair<QString, QPair<int, int>>> adherencePoints;
    for (auto it = adherenceByDay.cbegin(); it != adherenceByDay.cend(); ++it) {
        adherencePoints.append({it.key().toString("dd.MM"), it.value()});
    }
    std::sort(adherencePoints.begin(), adherencePoints.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    // ─── Stat cards ───
    auto* statsRow = new QGridLayout;
    statsRow->setHorizontalSpacing(14);
    statsRow->setVerticalSpacing(14);

    statsRow->addWidget(
        makeAnalyticsStatCard(
            tr("Приступы (%1)").arg(periodLabel),
            QString::number(seizuresCount),
            tr("Записи из журнала приступов за выбранный период.")),
        0, 0);

    statsRow->addWidget(
        makeAnalyticsStatCard(
            tr("Записи самочувствия"),
            QString::number(wellbeingCount),
            tr("Сохранённые записи настроения, симптомов и триггеров за период.")),
        0, 1);

    statsRow->addWidget(
        makeAnalyticsStatCard(
            tr("Средний настрой"),
            moodCount > 0 ? QString::number(avgMood, 'f', 1) : tr("—"),
            tr("Среднее по шкале от 1 до 5 за период.")),
        0, 2);

    int adherencePct = 0;
    int adherenceTotal = 0, adherenceTaken = 0;
    for (const auto& pt : adherencePoints) {
        adherenceTotal += pt.second.second;
        adherenceTaken += pt.second.first;
    }
    if (adherenceTotal > 0)
        adherencePct = (adherenceTaken * 100) / adherenceTotal;

    statsRow->addWidget(
        makeAnalyticsStatCard(
            tr("Приверженность"),
            adherenceTotal > 0 ? tr("%1%").arg(adherencePct) : tr("—"),
            tr("%2/%3 приёмов по расписанию.")
                .arg(adherenceTaken)
                .arg(adherenceTotal)),
        0, 3);

    auto* statsHost = new QWidget(m_analyticsHost);
    statsHost->setLayout(statsRow);
    m_analyticsLayout->addWidget(statsHost);

    // ─── График частоты приступов ───
    if (!seizureByDay.isEmpty()) {
        m_analyticsLayout->addWidget(
            makeSection(tr("Частота приступов"),
                        makeSeizureFrequencyChart(seizureByDay)));
    }

    // ─── График настроения ───
    if (!moodPoints.isEmpty()) {
        m_analyticsLayout->addWidget(
            makeSection(tr("Динамика настроения"),
                        makeMoodChart(moodPoints)));
    }

    // ─── Adherence chart ───
    if (!adherencePoints.isEmpty()) {
        m_analyticsLayout->addWidget(
            makeSection(tr("Приверженность приёму лекарств"),
                        makeAdherenceChart(adherencePoints)));
    }

    // ─── Bar charts row ───
    auto* chartsRow = new QHBoxLayout;
    chartsRow->setSpacing(16);

    if (!topSymptoms.isEmpty()) {
        chartsRow->addWidget(
            makeSection(tr("Частые симптомы"),
                        makeBarChart(tr("Симптомы"),
                                     topSymptoms,
                                     isDarkTheme() ? QColor("#8B9CFF") : QColor("#5B6CFF"),
                                     6)),
            1);
    }

    if (!topTriggers.isEmpty()) {
        chartsRow->addWidget(
            makeSection(tr("Частые триггеры"),
                        makeBarChart(tr("Триггеры"),
                                     topTriggers,
                                     isDarkTheme() ? QColor("#C58BFF") : QColor("#8E44FF"),
                                     6)),
            1);
    }

    if (chartsRow->count() > 0) {
        auto* rowHost = new QWidget(m_analyticsHost);
        rowHost->setLayout(chartsRow);
        m_analyticsLayout->addWidget(rowHost);
    }

    // ─── Insights ───
    QStringList insights;

    if (moodCount > 0) {
        if (avgMood >= 4.0)
            insights << tr("Общее настроение в записях в основном положительное.");
        else if (avgMood <= 2.5)
            insights << tr("Средний настрой понижен — стоит обратить внимание на самочувствие и режим.");
        else
            insights << tr("Настрой в среднем находится в нейтральной зоне.");
    }

    if (!topTriggers.isEmpty()) {
        insights << tr("Самый частый триггер: %1 (%2).")
                        .arg(topTriggers.first().first)
                        .arg(topTriggers.first().second);
    }

    if (!topSymptoms.isEmpty()) {
        insights << tr("Самый частый симптом: %1 (%2).")
                        .arg(topSymptoms.first().first)
                        .arg(topSymptoms.first().second);
    }

    int overlapDays = 0;
    for (const QDate& d : seizureDays) {
        if (wellbeingDays.contains(d))
            ++overlapDays;
    }

    if (!seizureDays.isEmpty()) {
        insights << tr("Дней, где есть и приступ, и запись самочувствия: %1 из %2.")
                        .arg(overlapDays)
                        .arg(seizureDays.size());
    }

    if (adherencePct < 50 && adherenceTotal > 0) {
        insights << tr("Приверженность приёму лекарств ниже 50%% — обратите внимание на соблюдение графика.");
    } else if (adherencePct >= 90 && adherenceTotal > 0) {
        insights << tr("Отличная приверженность приёму лекарств — %1%%.").arg(adherencePct);
    }

    // ─── ПРОГНОЗ ТАБЛЕТОК ───
    QJsonArray medsArr = m_core->medicationsJson();
    if (!medsArr.isEmpty()) {
        auto* predSection = new QFrame(m_analyticsHost);
        predSection->setObjectName("SectionCard");
        auto* predLayout = new QVBoxLayout(predSection);
        predLayout->setContentsMargins(18, 18, 18, 18);
        predLayout->setSpacing(8);
        auto* predTitle = new QLabel(tr("Прогноз остатка препаратов"), predSection);
        predTitle->setObjectName("SectionTitle");
        predLayout->addWidget(predTitle);

        for (const auto& v : medsArr) {
            const QJsonObject med = v.toObject();
            const QString name = med.value("brand_name").toString();
            if (name.isEmpty()) continue;
            const double stock = med.value("stock_tablets").toDouble(0);
            const double doseTabs = med.value("dose_tablets").toDouble(0);
            const int timesPerDay = med.value("times_per_day").toInt(0);
            if (doseTabs <= 0 || timesPerDay <= 0 || stock <= 0) continue;
            int daysLeft = static_cast<int>(stock / (doseTabs * timesPerDay));
            QString predText;
            if (daysLeft <= 0) predText = tr("Закончились!");
            else if (daysLeft <= 3) predText = tr("Хватит на ~%1 дн. — ПОРА КУПИТЬ!").arg(daysLeft);
            else predText = tr("Хватит на ~%1 дн.").arg(daysLeft);

            auto* lbl = new QLabel(QString("%1: %2").arg(name, predText), predSection);
            lbl->setObjectName(daysLeft <= 3 ? "LowStockWarning" : "MutedLabel");
            lbl->setWordWrap(true);
            predLayout->addWidget(lbl);
        }
        m_analyticsLayout->addWidget(predSection);
    }

    // ─── КОРРЕЛЯЦИЯ ТРИГГЕРЫ → ПРИСТУПЫ ───
    if (!m_cachedSeizureItems.isEmpty() && !m_cachedTriggers.isEmpty()) {
        QSet<QDate> seizureDates;
        for (const auto& v : std::as_const(m_cachedSeizureItems)) {
            QDateTime dt = QDateTime::fromString(v.toObject().value("started_at").toString(), Qt::ISODate);
            if (dt.isValid()) seizureDates.insert(dt.date());
        }

        QMap<QString, QPair<int, int>> triggerStats; // trigger -> (days_with_trigger, days_with_seizure)
        for (const auto& v : std::as_const(m_cachedTriggers)) {
            const QJsonObject o = v.toObject();
            QDateTime dt = QDateTime::fromString(o.value("logged_at").toString(), Qt::ISODate);
            if (!dt.isValid()) continue;
            const QJsonValue triggers = o.value("triggers");
            if (!triggers.isArray()) continue;
            const bool hadSeizure = seizureDates.contains(dt.date());
            for (const auto& t : triggers.toArray()) {
                const QString key = t.toString().trimmed();
                if (key.isEmpty()) continue;
                auto& st = triggerStats[key];
                st.first++;
                if (hadSeizure) st.second++;
            }
        }

        QStringList correlations;
        for (auto it = triggerStats.begin(); it != triggerStats.end(); ++it) {
            if (it.value().first < 2) continue;
            int pct = it.value().second * 100 / it.value().first;
            if (pct >= 30)
                correlations << QString("%1: %2% дней с приступами").arg(it.key()).arg(pct);
        }

        if (!correlations.isEmpty()) {
            auto* corrSection = new QFrame(m_analyticsHost);
            corrSection->setObjectName("SectionCard");
            auto* corrLayout = new QVBoxLayout(corrSection);
            corrLayout->setContentsMargins(18, 18, 18, 18);
            corrLayout->setSpacing(6);
            auto* corrTitle = new QLabel(tr("Корреляция триггеров с приступами"), corrSection);
            corrTitle->setObjectName("SectionTitle");
            corrLayout->addWidget(corrTitle);
            for (const QString& c : correlations) {
                auto* l = new QLabel("• " + c, corrSection);
                l->setWordWrap(true);
                corrLayout->addWidget(l);
            }
            m_analyticsLayout->addWidget(corrSection);
        }
    }

    m_analyticsLayout->addStretch();
}

// ---------------------------------------------------------------------------
// PDF export
// ---------------------------------------------------------------------------

void MainWindow::createAnalyticsPdfReport()
{
    const QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Сохранить PDF аналитики"),
        QDir::homePath() + "/trackly-analytics.pdf",
        tr("PDF files (*.pdf)")
        );

    if (filePath.isEmpty()) {
        return;
    }

    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(144);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        showToast(m_central, tr("Не удалось создать PDF"), ToastKind::Error, 3000);
        return;
    }

    QFont titleFont = painter.font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);

    QFont textFont = painter.font();
    textFont.setPointSize(10);
    textFont.setBold(false);

    int x = 120;
    int y = 140;
    const int w = writer.width() - 240;

    painter.setFont(titleFont);
    painter.drawText(x, y, w, 40, Qt::AlignLeft | Qt::AlignVCenter, tr("Trackly — аналитический отчёт"));
    y += 70;

    painter.setFont(textFont);

    double avgMood = 0.0;
    double avgSleep = 0.0;
    int moodN = 0;
    int sleepN = 0;

    for (const auto& v : m_cachedTriggers) {
        const auto o = v.toObject();
        const int mood = o.value("mood").toInt();
        const double sleep = o.value("sleep_hours").toDouble();
        if (mood > 0) {
            avgMood += mood;
            ++moodN;
        }
        if (sleep > 0.0) {
            avgSleep += sleep;
            ++sleepN;
        }
    }

    if (moodN > 0) avgMood /= moodN;
    if (sleepN > 0) avgSleep /= sleepN;

    const QString summary = tr(
                                "Приступов: %1\n"
                                "Записей самочувствия: %2\n"
                                "Приёмов лекарств: %3\n"
                                "Средний настрой: %4 из 5")
                                .arg(m_cachedSeizureItems.size())
                                .arg(m_cachedTriggers.size())
                                .arg(m_cachedMedicationLogs.size())
                                .arg(moodN > 0 ? QString::number(avgMood, 'f', 1) : tr("—"));

    painter.drawText(QRect(x, y, w, 220), Qt::TextWordWrap, summary);
    y += 240;

    QList<QChartView*> chartViews = m_analyticsHost->findChildren<QChartView*>();
    for (int i = 0; i < chartViews.size(); ++i) {
        QChartView* view = chartViews.at(i);
        if (!view) {
            continue;
        }

        QPixmap px(view->size());
        px.fill(Qt::transparent);

        QPainter p(&px);
        view->render(&p);
        p.end();

        if (i > 0) {
            writer.newPage();
            y = 120;
        }

        painter.setFont(titleFont);
        painter.drawText(x, y, w, 36, Qt::AlignLeft | Qt::AlignVCenter,
                         tr("График %1").arg(i + 1));
        y += 50;

        QRect target(x, y, w, writer.height() - y - 120);
        painter.drawPixmap(target, px);
    }

    painter.end();

    showToast(m_central, tr("PDF аналитики сохранён"), ToastKind::Success, 2600);
}

// --- showAiAnalyticsDialog ---
void MainWindow::showAiAnalyticsDialog()
{
    // собираем данные
    QJsonObject data;
    data["total_seizures"] = m_cachedSeizureItems.size();
    data["total_triggers"] = m_cachedTriggers.size();
    data["total_medication_logs"] = m_cachedMedicationLogs.size();
    data["total_medications"] = m_cachedMedications.size();

    int takenCount = 0, missedCount = 0;
    for (const auto& v : std::as_const(m_cachedMedicationLogs)) {
        const QString s = v.toObject().value("status").toString().toLower();
        if (s == "taken") takenCount++;
        else if (s == "missed") missedCount++;
    }
    data["taken_intakes"] = takenCount;
    data["missed_intakes"] = missedCount;

    int adherencePct = 0;
    if (takenCount + missedCount > 0)
        adherencePct = (takenCount * 100) / (takenCount + missedCount);
    data["adherence_pct"] = adherencePct;

    QJsonObject symptomCounts, triggerCounts;
    double moodSum = 0; int moodCount = 0;
    for (const auto& v : std::as_const(m_cachedTriggers)) {
        const QJsonObject o = v.toObject();
        const int mood = o.value("mood").toInt();
        if (mood >= 1 && mood <= 5) { moodSum += mood; moodCount++; }
        const QJsonValue sv = o.value("symptoms");
        if (sv.isArray()) for (const auto& s : sv.toArray()) {
            const QString key = s.toString().trimmed();
            if (!key.isEmpty()) symptomCounts[key] = symptomCounts[key].toInt() + 1;
        }
        const QJsonValue tv = o.value("triggers");
        if (tv.isArray()) for (const auto& t : tv.toArray()) {
            const QString key = t.toString().trimmed();
            if (!key.isEmpty()) triggerCounts[key] = triggerCounts[key].toInt() + 1;
        }
    }
    data["symptom_frequencies"] = symptomCounts;
    data["trigger_frequencies"] = triggerCounts;
    data["avg_mood"] = moodCount > 0 ? moodSum / moodCount : 0;
    data["mood_entries"] = moodCount;

    // --- строим диалог ---
    auto* central = this->centralWidget();
    if (central) {
        auto* blur = new QGraphicsBlurEffect(central);
        blur->setBlurRadius(8.0);
        central->setGraphicsEffect(blur);
    }

    QDialog* dlg = new QDialog(this);
    dlg->setModal(true);
    dlg->setWindowFlag(Qt::FramelessWindowHint, true);
    dlg->setAttribute(Qt::WA_TranslucentBackground, true);
    dlg->resize(660, 560);

    auto* outer = new QVBoxLayout(dlg);
    outer->setContentsMargins(16, 16, 16, 16);

    auto* card = new QFrame(dlg);
    card->setObjectName("SectionCard");
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 20, 24, 16);
    layout->setSpacing(10);

    // ─── Заголовок ───
    auto* headerRow = new QHBoxLayout;
    auto* icon = new QLabel(card);
    icon->setPixmap(QPixmap(":/icons/chatgpt.svg").scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    auto* titleBlock = new QVBoxLayout;
    auto* title = new QLabel(tr("Trackly AI"), card);
    title->setObjectName("SectionTitle");
    title->setStyleSheet("font-size:18px;font-weight:bold;color:#74AA9C;");
    auto* subtitle = new QLabel(tr("powered by ChatGPT 4o"), card);
    subtitle->setObjectName("MutedLabel");
    subtitle->setStyleSheet("font-size:11px;");
    titleBlock->addWidget(title);
    titleBlock->addWidget(subtitle);
    headerRow->addWidget(icon);
    headerRow->addLayout(titleBlock);
    headerRow->addStretch();
    auto* closeBtn = new QPushButton(tr("✕"), card);
    closeBtn->setFixedSize(28, 28);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setObjectName("AiCloseBtn");
    headerRow->addWidget(closeBtn);
    layout->addLayout(headerRow);

    auto* sep = new QFrame(card);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("background: rgba(116,170,156,0.15);");
    sep->setFixedHeight(1);
    layout->addWidget(sep);

    // ─── Быстрые статы (до ответа AI) ───
    auto* quickStats = new QHBoxLayout;
    quickStats->setSpacing(12);
    auto makeStat = [card](const QString& val, const QString& label) {
        auto* w = new QFrame(card);
        w->setStyleSheet("background:rgba(116,170,156,0.08);border-radius:10px;padding:8px 12px;");
        auto* l = new QVBoxLayout(w);
        l->setContentsMargins(0,0,0,0);
        auto* v = new QLabel(val, w);
        v->setStyleSheet("font-size:16px;font-weight:bold;color:#74AA9C;");
        v->setAlignment(Qt::AlignCenter);
        auto* lb = new QLabel(label, w);
        lb->setObjectName("MutedLabel");
        lb->setAlignment(Qt::AlignCenter);
        lb->setStyleSheet("font-size:10px;");
        l->addWidget(v);
        l->addWidget(lb);
        return w;
    };
    quickStats->addWidget(makeStat(QString::number(data["total_seizures"].toInt()), tr("Приступы")));
    quickStats->addWidget(makeStat(QString::number(takenCount), tr("Принято")));
    quickStats->addWidget(makeStat(QString::number(adherencePct) + "%", tr("Приверж.")));
    quickStats->addWidget(makeStat(moodCount > 0 ? QString::number(data["avg_mood"].toDouble(), 'f', 1) : "—", tr("Настроение")));
    layout->addLayout(quickStats);

    // ─── Контент ───
    auto* contentStack = new QStackedWidget(card);

    auto* loadingPage = new QWidget;
    auto* loadLayout = new QVBoxLayout(loadingPage);
    loadLayout->setAlignment(Qt::AlignCenter);
    auto* spinner = new QLabel(tr("Анализирую данные..."), loadingPage);
    spinner->setObjectName("MutedLabel");
    spinner->setAlignment(Qt::AlignCenter);
    spinner->setStyleSheet("font-size:16px;padding:30px;");
    loadLayout->addWidget(spinner);
    contentStack->addWidget(loadingPage);

    auto* resultPage = new QWidget;
    auto* resultLayout = new QVBoxLayout(resultPage);
    auto* resultScroll = new QScrollArea(resultPage);
    resultScroll->setFrameShape(QFrame::NoFrame);
    resultScroll->setWidgetResizable(true);
    auto* resultContent = new QWidget;
    auto* resultContentLayout = new QVBoxLayout(resultContent);
    auto* resultLabel = new QLabel(resultContent);
    resultLabel->setWordWrap(true);
    resultLabel->setTextFormat(Qt::RichText);
    resultContentLayout->addWidget(resultLabel);
    resultContentLayout->addStretch();
    resultScroll->setWidget(resultContent);
    resultLayout->addWidget(resultScroll);
    contentStack->addWidget(resultPage);

    auto* errorPage = new QWidget;
    auto* errLayout = new QVBoxLayout(errorPage);
    errLayout->setAlignment(Qt::AlignCenter);
    auto* errLabel = new QLabel(errorPage);
    errLabel->setObjectName("MutedLabel");
    errLabel->setAlignment(Qt::AlignCenter);
    errLabel->setWordWrap(true);
    errLayout->addWidget(errLabel);
    contentStack->addWidget(errorPage);

    layout->addWidget(contentStack, 1);

    // ─── Disclaimer ───
    auto* disclaimer = new QLabel(
        tr("ИИ может допускать ошибки. Данный анализ не является медицинской рекомендацией."), card);
    disclaimer->setObjectName("MutedLabel");
    disclaimer->setWordWrap(true);
    disclaimer->setStyleSheet("font-size:10px;color:#94A3B8;padding-top:4px;");
    layout->addWidget(disclaimer);

    auto* retryBtn = new QPushButton(tr("Обновить анализ"), card);
    retryBtn->setObjectName("SecondaryButton");
    retryBtn->hide();
    layout->addWidget(retryBtn);

    outer->addWidget(card);

    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    m_core->requestAiAnalytics(data);

    QMetaObject::Connection conn1, conn2;
    conn1 = connect(m_core, &AppCore::aiAnalyticsReady, dlg,
        [contentStack, resultLabel, retryBtn](const QString& text) {
            contentStack->setCurrentIndex(1);
            QString html = "<div style='line-height:1.8;font-size:13px;'>";
            const QStringList sections = text.split("\n\n", Qt::SkipEmptyParts);
            for (const QString& section : sections) {
                QString sec = section.trimmed();
                bool isHeader = sec.startsWith("**") || sec.startsWith("📊") || sec.startsWith("🔍")
                    || sec.startsWith("⚠") || sec.startsWith("📈") || sec.startsWith("💊")
                    || sec.startsWith("✅") || sec.startsWith("📋") || sec.startsWith("📌");
                if (isHeader) {
                    html += "<p style='font-weight:bold;color:#74AA9C;margin:12px 0 4px 0;font-size:14px'>"
                            + sec.toHtmlEscaped() + "</p>";
                } else {
                    html += "<p style='margin:2px 0;'>" + sec.toHtmlEscaped().replace("\n", "<br>") + "</p>";
                }
            }
            html += "</div>";
            resultLabel->setText(html);
            retryBtn->show();
        });

    conn2 = connect(m_core, &AppCore::aiAnalyticsError, dlg,
        [contentStack, errLabel, retryBtn](const QString& err) {
            contentStack->setCurrentIndex(2);
            errLabel->setText(tr("Не удалось получить анализ.\n%1").arg(err.left(200)));
            retryBtn->show();
        });

    connect(retryBtn, &QPushButton::clicked, dlg, [this, dlg, data, contentStack, retryBtn]() {
        contentStack->setCurrentIndex(0);
        retryBtn->hide();
        m_core->requestAiAnalytics(data);
    });

    connect(dlg, &QDialog::finished, dlg, [central, conn1, conn2]() {
        if (central) central->setGraphicsEffect(nullptr);
        disconnect(conn1);
        disconnect(conn2);
    });

    dlg->show();
}

// --- positionAiWidgets ---
void MainWindow::positionAiWidgets()
{
    if (!m_aiAnalyticsBtn) return;

    // ищем страницу аналитики
    QWidget* analyticsPage = nullptr;
    for (int i = 0; i < m_stack->count(); ++i) {
        if (m_stack->widget(i) == m_aiAnalyticsBtn->parentWidget()) {
            analyticsPage = m_stack->widget(i);
            break;
        }
    }
    if (!analyticsPage) {
        analyticsPage = qobject_cast<QWidget*>(m_aiAnalyticsBtn->parent());
    }
    if (!analyticsPage) return;

    const int w = analyticsPage->width();
    const int h = analyticsPage->height();
    const int margin = 20;

    // FAB: правый нижний угол
    m_aiAnalyticsBtn->move(w - 76, h - 76);
    m_aiAnalyticsBtn->raise();

    // AI результат: карточка по центру
    if (m_aiResultLabel && m_aiResultLabel->isVisible()) {
        const int cardW = qMin(w - 60, 600);
        m_aiResultLabel->setFixedWidth(cardW);
        m_aiResultLabel->adjustSize();
        const int cardH = qMin(m_aiResultLabel->sizeHint().height() + 20, h - 40);
        m_aiResultLabel->setFixedHeight(cardH);
        m_aiResultLabel->move((w - cardW) / 2, (h - cardH) / 2);
        m_aiResultLabel->raise();
    }
}
