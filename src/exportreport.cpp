// exportreport.cpp — Экспорт отчёта в PDF
#include "mainwindow.h"
#include "appcore.h"
#include "utils.h"
#include "widgets/toastwidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>
#include <QFrame>
#include <QGraphicsBlurEffect>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QPainter>
#include <QPageSize>
#include <QPdfWriter>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

void MainWindow::showReportDialog()
{
    auto* central = this->centralWidget();
    if (!central) return;

    // ─── Блюр ───
    auto* blur = new QGraphicsBlurEffect(central);
    blur->setBlurRadius(6.0);
    blur->setBlurHints(QGraphicsBlurEffect::QualityHint);
    central->setGraphicsEffect(blur);

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Экспорт отчёта"));
    dlg.setModal(true);
    dlg.setWindowFlag(Qt::FramelessWindowHint, true);
    dlg.setAttribute(Qt::WA_TranslucentBackground, true);
    dlg.setMinimumWidth(420);

    auto* outer = new QVBoxLayout(&dlg);
    outer->setContentsMargins(16, 16, 16, 16);
    outer->setSpacing(12);

    auto* frame = new QFrame(&dlg);
    frame->setObjectName("SectionCard");
    auto* layout = new QVBoxLayout(frame);
    layout->setSpacing(12);

    auto* title = new QLabel(tr("Создать отчёт"), frame);
    title->setObjectName("SectionTitle");

    // Период
    auto* periodLbl = new QLabel(tr("Период:"), frame);
    auto* fromDate = new QDateEdit(QDate::currentDate().addDays(-30), frame);
    fromDate->setCalendarPopup(true);
    fromDate->setDisplayFormat("dd.MM.yyyy");
    auto* toDate = new QDateEdit(QDate::currentDate(), frame);
    toDate->setCalendarPopup(true);
    toDate->setDisplayFormat("dd.MM.yyyy");

    auto* periodRow = new QHBoxLayout;
    periodRow->addWidget(fromDate);
    periodRow->addWidget(new QLabel("—", frame));
    periodRow->addWidget(toDate);
    periodRow->addStretch();

    // Чекбоксы разделов
    auto* cbMeds = new QCheckBox(tr("Лекарства и приёмы"), frame);
    cbMeds->setChecked(true);
    auto* cbSeizures = new QCheckBox(tr("Приступы"), frame);
    cbSeizures->setChecked(true);
    auto* cbWellbeing = new QCheckBox(tr("Самочувствие"), frame);
    cbWellbeing->setChecked(true);
    auto* cbExams = new QCheckBox(tr("Обследования"), frame);
    cbExams->setChecked(true);
    auto* cbCharts = new QCheckBox(tr("Графики и диаграммы"), frame);
    cbCharts->setChecked(true);

    // Кнопки
    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto* cancelBtn = new QPushButton(tr("Отмена"), frame);
    auto* generateBtn = new QPushButton(tr("Создать PDF"), frame);
    generateBtn->setObjectName("PrimaryButton");
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(generateBtn);

    layout->addWidget(title);
    layout->addWidget(periodLbl);
    layout->addLayout(periodRow);
    layout->addWidget(cbMeds);
    layout->addWidget(cbSeizures);
    layout->addWidget(cbWellbeing);
    layout->addWidget(cbExams);
    layout->addWidget(cbCharts);
    layout->addStretch();
    layout->addLayout(btnRow);

    outer->addWidget(frame);

    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(generateBtn, &QPushButton::clicked, &dlg, [&dlg, this, fromDate, toDate,
            cbMeds, cbSeizures, cbWellbeing, cbExams, cbCharts]() {
        dlg.accept();
        generatePdfReport(fromDate->date(), toDate->date(),
                          cbMeds->isChecked(), cbSeizures->isChecked(),
                          cbWellbeing->isChecked(), cbExams->isChecked(),
                          cbCharts->isChecked());
    });

    dlg.exec();
    central->setGraphicsEffect(nullptr);
}

void MainWindow::generatePdfReport(const QDate& from, const QDate& to,
                                    bool includeMeds, bool includeSeizures,
                                    bool includeWellbeing, bool includeExams,
                                    bool includeCharts)
{
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить отчёт"),
        QDir::homePath() + "/Trackly_Отчёт_" + from.toString("yyyyMMdd") + "-" + to.toString("yyyyMMdd") + ".pdf",
        tr("PDF (*.pdf)"));
    if (fileName.isEmpty()) return;

    QPdfWriter writer(fileName);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Landscape);
    writer.setResolution(150);
    writer.setTitle(tr("Trackly — Отчёт о здоровье"));

    QPainter p(&writer);
    if (!p.isActive()) {
        showToast(m_central, tr("Не удалось создать PDF"), ToastKind::Error, 3000);
        return;
    }

    // ─── Параметры страницы ───
    const int M = 36;
    const int pageW = writer.width();
    const int pageH = writer.height();
    const QColor textColor(Qt::black);
    const QColor headerBg("#E8F0FE");
    const QColor altRowColor("#F8FAFC");
    const QColor gridColor("#CBD5E1");
    const QColor takenColor("#16A34A");
    const QColor missedColor("#DC2626");
    const QColor seizureColor("#7C3AED");

    // ─── Шрифт ───
    QFont f("Helvetica");
    f.setPointSize(11);
    p.setFont(f);

    // ─── Информация о пациенте ───
    QString fullName, birthDate;
    if (!m_cachedProfile.isEmpty()) {
        fullName  = m_cachedProfile.value("full_name").toString().trimmed();
        birthDate = m_cachedProfile.value("birth_date").toString().trimmed();
    }

    // ─── Функция верхнего колонтитула ───
    auto drawPageHeader = [&](int pg) {
        QFont hf = p.font(); hf.setPointSize(8); hf.setBold(false); p.setFont(hf);
        p.setPen(QColor("#94A3B8"));
        p.drawText(M, 10, pageW/2 - M, 18, Qt::AlignLeft,
                   QString("Trackly  |  %1  |  %2").arg(
                       fullName.isEmpty() ? tr("Пациент") : fullName,
                       from.toString("dd.MM.yy") + " – " + to.toString("dd.MM.yy")));
        p.drawText(pageW/2, 10, pageW/2 - M, 18, Qt::AlignRight, tr("Стр. %1").arg(pg));
        p.setPen(gridColor);
        p.drawLine(M, 30, pageW - M, 30);
    };

    // ─── ТИТУЛЬНИК ───
    {
        writer.newPage();
        // Логотип
        QPixmap logo(":/icons/logo-trackly-light.svg");
        if (!logo.isNull()) {
            logo = logo.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            p.drawPixmap((pageW - logo.width())/2, 60, logo);
        }

        f.setPointSize(28); f.setBold(true); p.setFont(f);
        p.setPen(QColor("#1A1A2E"));
        p.drawText(QRect(0, 280, pageW, 40), Qt::AlignCenter, "Trackly");

        f.setPointSize(14); f.setBold(false); p.setFont(f);
        p.setPen(QColor("#4F5B7A"));
        p.drawText(QRect(0, 330, pageW, 24), Qt::AlignCenter, tr("Отчёт о здоровье"));

        f.setPointSize(12); p.setFont(f);
        int y = 390;
        if (!fullName.isEmpty()) {
            p.drawText(QRect(0, y, pageW, 24), Qt::AlignCenter, fullName);
            y += 28;
        }
        if (!birthDate.isEmpty()) {
            p.drawText(QRect(0, y, pageW, 24), Qt::AlignCenter, tr("Дата рождения: %1").arg(birthDate));
            y += 28;
        }
        p.drawText(QRect(0, y + 10, pageW, 24), Qt::AlignCenter,
                   tr("Период: %1 – %2").arg(from.toString("dd.MM.yyyy"), to.toString("dd.MM.yyyy")));
    }

    // ─── ТАБЛИЦА ───
    writer.newPage();
    int pageNum = 2;
    drawPageHeader(pageNum);
    int pageY = 45;
    const int headerH = 30;
    const int minRowH = 18;
    const int maxRowH = 72;

    // Индексация данных по дням
    QMap<QDate, QStringList> dayMeds;
    QMap<QDate, QStringList> daySeizures;
    QMap<QDate, int> dayMood;
    QMap<QDate, QStringList> dayTriggers;
    QMap<QDate, double> daySleep;
    QMap<QDate, QString> dayNotes;
    QMap<QDate, QStringList> dayExams;

    if (includeMeds) {
        QMap<QString, double> medDoses;
        for (const auto& v : std::as_const(m_cachedMedications)) {
            const QString id = v.toObject().value("id").toString();
            medDoses[id] = v.toObject().value("dose_mg").toDouble();
        }
        for (const auto& v : std::as_const(m_cachedMedicationLogs)) {
            QDateTime dt = QDateTime::fromString(v.toObject().value("taken_at").toString(), Qt::ISODate);
            if (!dt.isValid()) continue;
            const QDate d = dt.toLocalTime().date();
            if (d < from || d > to) continue;
            const QString status = v.toObject().value("status").toString();
            bool isMissed = (status != "taken");
            const QString medId = v.toObject().value("medication_id").toString();
            const QString name = m_medicationNamesById.value(medId, tr("Лек."));
            double dose = medDoses.value(medId, 0);
            QString doseStr = dose > 0 ? QString(" %1mg").arg(dose, 0, 'f', 0) : QString();
            QString line = QString("%1 %2%3").arg(dt.toLocalTime().time().toString("HH:mm"), name, doseStr);
            if (isMissed) line = "[!] " + line;
            else line = "✓ " + line;
            dayMeds[d].append(line);
        }
    }

    if (includeSeizures) {
        for (const auto& v : std::as_const(m_cachedSeizureItems)) {
            QDateTime dt = QDateTime::fromString(v.toObject().value("started_at").toString(), Qt::ISODate);
            if (!dt.isValid()) continue;
            const QDate d = dt.date();
            if (d < from || d > to) continue;
            const QString type = v.toObject().value("seizure_type").toString();
            const int sev = v.toObject().value("severity").toInt();
            daySeizures[d].append(QString("%1(%2) %3").arg(type, QString::number(sev), dt.time().toString("HH:mm")));
        }
    }

    if (includeWellbeing) {
        for (const auto& v : std::as_const(m_cachedTriggers)) {
            QDateTime dt = QDateTime::fromString(v.toObject().value("logged_at").toString(), Qt::ISODate);
            if (!dt.isValid()) continue;
            const QDate d = dt.date();
            if (d < from || d > to) continue;
            const int mood = v.toObject().value("mood").toInt();
            if (mood > 0) dayMood[d] = mood;
            double sleepH = v.toObject().value("sleep_hours").toDouble();
            if (sleepH > 0) daySleep[d] = sleepH;
            QString note = v.toObject().value("note").toString().trimmed();
            if (!note.isEmpty()) dayNotes[d] = note;
            auto arr = v.toObject().value("triggers").toArray();
            for (const auto& t : arr) if (!t.toString().isEmpty()) dayTriggers[d].append(t.toString());
            auto symArr = v.toObject().value("symptoms").toArray();
            for (const auto& s : symArr) if (!s.toString().isEmpty()) dayTriggers[d].append(s.toString());
        }
    }

    if (includeExams) {
        for (const auto& v : std::as_const(m_cachedExaminations)) {
            QDateTime dt = QDateTime::fromString(v.toObject().value("planned_at").toString(), Qt::ISODate);
            if (!dt.isValid()) continue;
            const QDate d = dt.date();
            if (d < from || d > to) continue;
            const QString t = v.toObject().value("exam_type").toString();
            const QString s = v.toObject().value("exam_subtype").toString();
            dayExams[d].append(s.isEmpty() ? t : t + ": " + s);
        }
    }

    // Заголовок таблицы
    QFont tblF = p.font(); tblF.setPointSize(8); tblF.setBold(false); p.setFont(tblF);
    QFont hdrF = tblF; hdrF.setBold(true);

    QList<int> colW = {8, 5, 26, 20, 6, 18, 17};
    QStringList headers = {tr("Дата"), tr("День"), tr("Лекарства"), tr("Приступы"), tr("Настр."), tr("Самочувствие"), tr("Обслед.")};
    const int totalW = pageW - 2*M;

    // Рисуем шапку
    p.setFont(hdrF);
    p.fillRect(M, pageY, totalW, headerH, headerBg);
    int x = M;
    for (int i = 0; i < headers.size(); ++i) {
        int w = totalW * colW[i] / 100;
        if (i > 0) { p.setPen(gridColor); p.drawLine(x, pageY, x, pageY+headerH); }
        p.setPen(textColor);
        p.drawText(QRect(x+3, pageY, w-6, headerH), Qt::AlignVCenter|Qt::AlignLeft, headers[i]);
        x += w;
    }
    p.setPen(gridColor);
    p.drawRect(M, pageY, totalW, headerH);
    pageY += headerH;

    // Строки
    p.setFont(tblF);
    QFontMetrics fm(tblF);
    int dayIdx = 0;
    for (QDate d = from; d <= to; d = d.addDays(1)) {
        QStringList colTexts;
        colTexts << d.toString("dd.MM.yy");
        colTexts << QLocale::system().dayName(d.dayOfWeek(), QLocale::ShortFormat);

        QString medsStr = "—";
        if (includeMeds && dayMeds.contains(d)) medsStr = dayMeds[d].join("\n");
        colTexts << medsStr;

        QString seizStr = "—";
        if (includeSeizures && daySeizures.contains(d)) seizStr = daySeizures[d].join("\n");
        colTexts << seizStr;

        colTexts << (includeWellbeing && dayMood.contains(d) ? QString::number(dayMood[d]) + "/5" : "—");

        QString trigStr = "—";
        if (includeWellbeing) {
            QStringList trigParts;
            if (dayTriggers.contains(d)) {
                QSet<QString> uniq;
                for (const auto& t : dayTriggers[d]) uniq.insert(t);
                if (!uniq.isEmpty()) trigParts << QStringList(uniq.begin(), uniq.end()).join(", ");
            }
            if (daySleep.contains(d)) trigParts << tr("Сон %1ч").arg(daySleep[d], 0, 'f', 1);
            if (dayNotes.contains(d)) trigParts << dayNotes[d];
            if (!trigParts.isEmpty()) trigStr = trigParts.join("\n");
        }
        colTexts << trigStr;

        QString examStr = "—";
        if (includeExams && dayExams.contains(d)) examStr = dayExams[d].join("\n");
        colTexts << examStr;

        int thisRowH = minRowH;
        for (int i = 0; i < colTexts.size(); ++i) {
            int w = totalW * colW[i] / 100 - 6;
            QRect br = fm.boundingRect(QRect(0,0,w,9999), Qt::AlignLeft|Qt::TextWordWrap, colTexts[i]);
            thisRowH = qMax(thisRowH, br.height() + 8);
        }
        thisRowH = qMin(thisRowH, maxRowH);

        if (pageY + thisRowH > pageH - M) { writer.newPage(); pageNum++; drawPageHeader(pageNum); pageY = 45; }

        if (dayIdx % 2 == 1) p.fillRect(M, pageY, totalW, thisRowH, altRowColor);

        x = M;
        for (int i = 0; i < colTexts.size(); ++i) {
            int w = totalW * colW[i] / 100;
            if (i > 0) { p.setPen(gridColor); p.drawLine(x, pageY, x, pageY+thisRowH); }

            QColor c = textColor;
            if (i == 3 && includeSeizures && daySeizures.contains(d)) c = seizureColor;
            else if (i == 2 && includeMeds && dayMeds.contains(d)) c = takenColor;

            p.setPen(c);
            QRect textRect(x+3, pageY+3, w-6, thisRowH-6);
            p.drawText(textRect, Qt::AlignLeft|Qt::TextWordWrap, colTexts[i]);
            x += w;
        }
        p.setPen(gridColor);
        p.drawLine(M, pageY+thisRowH, M+totalW, pageY+thisRowH);
        pageY += thisRowH;
        dayIdx++;
    }
    p.setPen(gridColor);
    p.drawRect(M, 45+headerH, totalW, pageY-45-headerH);

    // ─── ГРАФИКИ ───
    if (includeCharts) {
        writer.newPage();
        pageNum++;
        drawPageHeader(pageNum);

        f.setPointSize(14); f.setBold(true); p.setFont(f);
        p.setPen(QColor("#1A1A2E"));
        p.drawText(QRect(0, 45, pageW, 24), Qt::AlignCenter, tr("Графики и диаграммы"));

        // TODO: рендеринг графиков QtCharts в QPdfWriter
        f.setPointSize(10); f.setBold(false); p.setFont(f);
        p.setPen(QColor("#64748B"));
        p.drawText(QRect(M, 80, pageW-2*M, 20), Qt::AlignLeft, tr("Графики доступны в приложении на вкладке «Аналитика»."));
    }

    p.end();

    showToast(m_central, tr("Отчёт сохранён"), ToastKind::Success, 3500);
}
