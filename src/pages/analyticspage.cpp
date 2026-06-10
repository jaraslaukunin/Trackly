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

    auto* subtitle = new QLabel(
        tr("Сводка по приступам, самочувствию, симптомам, триггерам и приёму лекарств."),
        page);
    subtitle->setObjectName("MutedLabel");
    subtitle->setWordWrap(true);

    auto* exportRow = new QHBoxLayout;
    exportRow->addStretch();

    m_exportAnalyticsPdfBtn = new QPushButton(tr("Экспорт PDF"), page);
    m_exportAnalyticsPdfBtn->setObjectName("SecondaryButton");
    m_exportAnalyticsPdfBtn->setIcon(QIcon(":/icons/description.svg"));
    exportRow->addWidget(m_exportAnalyticsPdfBtn);

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

    root->addWidget(subtitle);
    root->addLayout(exportRow);
    root->addWidget(scroll, 1);

    connect(m_exportAnalyticsPdfBtn, &QPushButton::clicked,
            this, &MainWindow::createAnalyticsPdfReport);

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
    pen.setColor(isDarkTheme() ? QColor("#A78BFA") : QColor("#6D4AFF"));
    series->setPen(pen);

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

    auto* view = new QChartView(chart, m_analyticsHost);
    view->setRenderHint(QPainter::Antialiasing);
    view->setMinimumHeight(300);
    return view;
}

// ---------------------------------------------------------------------------
// Refresh analytics
// ---------------------------------------------------------------------------

void MainWindow::refreshAnalyticsPage()
{
    if (!m_analyticsLayout || !m_analyticsHost)
        return;

    clearLayout(m_analyticsLayout);

    if (m_cachedSeizureItems.isEmpty() &&
        m_cachedTriggers.isEmpty() &&
        m_cachedMedicationLogs.isEmpty()) {
        m_analyticsLayout->addWidget(
            makeAnalyticsEmptyState(tr("Добавь приступы, записи самочувствия или приёмы лекарств, чтобы увидеть аналитику.")));
        m_analyticsLayout->addStretch();
        return;
    }

    const QDate today = QDate::currentDate();
    const QDate from30 = today.addDays(-29);

    int seizures30 = 0;
    int wellbeingCount = 0;
    int medLogCount30 = 0;
    double moodSum = 0.0;
    int moodCount = 0;

    QMap<QDate, QPair<double, int>> moodAggByDay;
    QMap<QString, int> symptomFreq;
    QMap<QString, int> triggerFreq;
    QSet<QDate> seizureDays;
    QSet<QDate> wellbeingDays;

    for (const auto& v : m_cachedSeizureItems) {
        const QJsonObject o = v.toObject();
        QDateTime dt = QDateTime::fromString(o.value("started_at").toString(), Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(o.value("started_at").toString(), Qt::ISODateWithMs);

        if (!dt.isValid())
            continue;

        if (dt.date() >= from30)
            ++seizures30;

        seizureDays.insert(dt.date());
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

        if (dt.date() >= from30)
            ++medLogCount30;
    }

    for (const auto& v : m_cachedTriggers) {
        const QJsonObject o = v.toObject();
        QDateTime dt = QDateTime::fromString(o.value("logged_at").toString(), Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(o.value("logged_at").toString(), Qt::ISODateWithMs);

        if (!dt.isValid())
            continue;

        const QDate d = dt.date();
        wellbeingDays.insert(d);

        const int mood = o.value("mood").toInt();
        if (mood >= 1 && mood <= 5) {
            moodSum += mood;
            ++moodCount;

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

        ++wellbeingCount;
    }

    const double avgMood = moodCount > 0 ? moodSum / moodCount : 0.0;

    QList<QPair<QDate, double>> moodPoints;
    for (auto it = moodAggByDay.cbegin(); it != moodAggByDay.cend(); ++it) {
        if (it.value().second <= 0)
            continue;
        moodPoints.append({it.key(), it.value().first / it.value().second});
    }

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

    auto* statsRow = new QGridLayout;
    statsRow->setHorizontalSpacing(14);
    statsRow->setVerticalSpacing(14);

    statsRow->addWidget(
        makeAnalyticsStatCard(
            tr("Приступы за 30 дней"),
            QString::number(seizures30),
            tr("Считаются записи из журнала приступов за последний месяц.")),
        0, 0);

    statsRow->addWidget(
        makeAnalyticsStatCard(
            tr("Записи самочувствия"),
            QString::number(wellbeingCount),
            tr("Всего сохранённых записей настроения, симптомов и триггеров.")),
        0, 1);

    statsRow->addWidget(
        makeAnalyticsStatCard(
            tr("Средний настрой"),
            moodCount > 0 ? QString::number(avgMood, 'f', 1) : tr("—"),
            tr("Среднее значение по шкале от 1 до 5.")),
        0, 2);

    statsRow->addWidget(
        makeAnalyticsStatCard(
            tr("Приёмы лекарств за 30 дней"),
            QString::number(medLogCount30),
            tr("Отмеченные приёмы препаратов за последний месяц.")),
        0, 3);

    auto* statsHost = new QWidget(m_analyticsHost);
    statsHost->setLayout(statsRow);
    m_analyticsLayout->addWidget(statsHost);

    if (!moodPoints.isEmpty()) {
        m_analyticsLayout->addWidget(
            makeSection(tr("Динамика настроения"),
                        makeMoodChart(moodPoints)));
    }

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

    m_analyticsLayout->addWidget(makeAnalyticsInsightsCard(insights));
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
