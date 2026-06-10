#include "tracklycalendar.h"

#include <QPainter>

TracklyCalendar::TracklyCalendar(QWidget* parent)
    : QCalendarWidget(parent)
{
}

void TracklyCalendar::setDecorations(const QSet<QDate>& seizures,
                                     const QSet<QDate>& meds,
                                     const QSet<QDate>& exams)
{
    m_seizures = seizures;
    m_meds = meds;
    m_exams = exams;
    updateCells();
}

void TracklyCalendar::paintCell(QPainter* painter, const QRect& rect, QDate date) const
{
    QCalendarWidget::paintCell(painter, rect, date);

    QVector<QColor> dots;
    if (m_seizures.contains(date)) dots.push_back(QColor("#D97C88"));
    if (m_meds.contains(date))     dots.push_back(QColor("#6E7CF6"));
    if (m_exams.contains(date))    dots.push_back(QColor("#8A79E8"));

    if (dots.isEmpty())
        return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const int r = 4;
    const int spacing = 3;
    const int totalWidth = int(dots.size()) * (2 * r) + (int(dots.size()) - 1) * spacing;
    int x = rect.center().x() - totalWidth / 2;
    const int y = rect.bottom() - 8;

    for (const QColor& c : dots) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(c);
        painter->drawEllipse(QPointF(x + r, y), r, r);
        x += 2 * r + spacing;
    }

    painter->restore();
}
