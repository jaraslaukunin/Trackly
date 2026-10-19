#include "tracklycalendar.h"
#include <QPainter>
#include <QVector>
#include <QPair>

TracklyCalendar::TracklyCalendar(QWidget* parent) : QCalendarWidget(parent) {}

void TracklyCalendar::setDayStats(const QMap<QDate, DayStats>& stats)
{
    m_dayStats = stats;
    m_seizureDays.clear();
    m_examDays.clear();
    m_medDays.clear();
    m_plannedDays.clear();
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        if (it.value().seizures > 0) m_seizureDays.insert(it.key());
        if (it.value().exams > 0)    m_examDays.insert(it.key());
        if (it.value().taken > 0 || it.value().missed > 0) m_medDays.insert(it.key());
        if (it.value().planned > 0 && it.value().taken == 0 && it.value().missed == 0)
            m_plannedDays.insert(it.key());
    }
    updateCells();
}

void TracklyCalendar::paintCell(QPainter* p, const QRect& rect, QDate date) const
{
    QCalendarWidget::paintCell(p, rect, date);

    if (m_mode == 0) {
        // дашборд: молнии (приступы) + синие точки (обследования)
        auto it = m_dayStats.find(date);
        if (it == m_dayStats.end()) return;
        const int s = it.value().seizures;
        const int e = it.value().exams;
        if (s == 0 && e == 0) return;

        p->save();
        p->setRenderHint(QPainter::Antialiasing, true);

        // молнии для приступов
        if (s > 0) {
            QPen boltPen(QColor("#DC2626"), 1.5);
            boltPen.setJoinStyle(Qt::MiterJoin);
            p->setPen(boltPen);
            p->setBrush(Qt::NoBrush);
            int lx = rect.right() - 8, ly = rect.top() + 4;
            for (int i = 0; i < qMin(s, 2); ++i) {
                const int off = i * 7;
                p->drawLine(lx - off, ly, lx - 3 - off, ly + 7);
                p->drawLine(lx - 3 - off, ly + 7, lx + 1 - off, ly + 7);
                p->drawLine(lx + 1 - off, ly + 7, lx - 2 - off, ly + 12);
            }
        }

        // синие точки для обследований
        if (e > 0) {
            int r = 3, sp = 2;
            int tw = qMin(e, 3) * (2*r) + (qMin(e, 3)-1) * sp;
            int x = rect.center().x() - tw/2;
            int y = rect.bottom() - 6;
            p->setPen(Qt::NoPen);
            p->setBrush(QColor("#3B82F6"));
            for (int i = 0; i < qMin(e, 3); ++i) {
                p->drawEllipse(QPointF(x+r, y), r, r);
                x += 2*r + sp;
            }
        }
        p->restore();
    } else {
        // терапия: кружки приёмов + молнии приступов
        auto it = m_dayStats.find(date);
        if (it == m_dayStats.end()) return;
        const auto& ds = it.value();
        p->save();
        p->setRenderHint(QPainter::Antialiasing, true);
        int r = 3, sp = 2, y = rect.bottom() - 6;
        QVector<QPair<QColor, int>> dots;
        if (ds.taken > 0 && ds.missed > 0) {
            dots.append({QColor("#EAB308"), 0}); // жёлтый = смешанный
        } else {
            for (int i = 0; i < ds.taken; ++i)   dots.append({QColor("#16A34A"), i});
            for (int i = 0; i < ds.missed; ++i)  dots.append({QColor("#DC2626"), i});
            if (ds.taken == 0 && ds.missed == 0 && ds.planned > 0) {
                dots.append({QColor("#6366F1"), 0}); // индиго = запланировано
            }
        }
        int tw = dots.size() * (2*r) + qMax(0, (int)dots.size()-1) * sp;
        int x = rect.center().x() - tw/2;
        for (const auto& d : dots) {
            p->setPen(Qt::NoPen);
            p->setBrush(d.first);
            p->drawEllipse(QPointF(x+r, y), r, r);
            x += 2*r + sp;
        }
        // молния для приступов
        if (ds.seizures > 0) {
            p->setPen(QPen(QColor("#F97373"), 1.5));
            p->setBrush(Qt::NoBrush);
            int lx = rect.right() - 10, ly = rect.top() + 6;
            p->drawLine(lx, ly, lx-3, ly+8);
            p->drawLine(lx-3, ly+8, lx+1, ly+8);
            p->drawLine(lx+1, ly+8, lx-2, ly+14);
            if (ds.seizures >= 2) {
                lx -= 8;
                p->drawLine(lx, ly, lx-3, ly+8);
                p->drawLine(lx-3, ly+8, lx+1, ly+8);
                p->drawLine(lx+1, ly+8, lx-2, ly+14);
            }
        }
        p->restore();
    }
}
