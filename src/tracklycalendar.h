#pragma once

#include <QCalendarWidget>
#include <QSet>
#include <QMap>
#include <QDate>

struct DayStats {
    int taken = 0;
    int missed = 0;
    int planned = 0;
    int seizures = 0;
    int exams = 0;
};

class TracklyCalendar : public QCalendarWidget
{
    Q_OBJECT
public:
    explicit TracklyCalendar(QWidget* parent = nullptr);

    // режим: 0 = дашборд (приступы+обследования точками), 1 = терапия (приёмы+приступы)
    void setMode(int mode) { m_mode = mode; }

    void setDayStats(const QMap<QDate, DayStats>& stats);

protected:
    void paintCell(QPainter* painter, const QRect& rect, QDate date) const override;

private:
    int m_mode = 0;
    QMap<QDate, DayStats> m_dayStats;
    QSet<QDate> m_seizureDays;
    QSet<QDate> m_examDays;
    QSet<QDate> m_medDays;
    QSet<QDate> m_plannedDays;
};
