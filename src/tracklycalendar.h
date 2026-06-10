#pragma once

#include <QCalendarWidget>
#include <QSet>
#include <QDate>

class TracklyCalendar : public QCalendarWidget
{
    Q_OBJECT
public:
    explicit TracklyCalendar(QWidget* parent = nullptr);

    void setDecorations(const QSet<QDate>& seizures,
                        const QSet<QDate>& meds,
                        const QSet<QDate>& exams);

protected:
    void paintCell(QPainter* painter, const QRect& rect, QDate date) const override;

private:
    QSet<QDate> m_seizures;
    QSet<QDate> m_meds;
    QSet<QDate> m_exams;
};
