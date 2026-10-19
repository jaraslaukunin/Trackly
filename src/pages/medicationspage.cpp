// Medications page implementation for MainWindow
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
#include <QDebug>
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
#include <QSettings>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimeEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

// --- makeMedicationCard ---
QFrame* MainWindow::makeMedicationCard(const QJsonObject& med)
{
    auto* card = new QFrame(m_medCardsHost);
    card->setObjectName("SectionCard");
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    card->setContextMenuPolicy(Qt::CustomContextMenu);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    const QString id          = med.value("id").toString();
    const QString brandName   = med.value("brand_name").toString();
    const QString inn         = med.value("inn").toString();
    const double pillStrength = med.value("pill_strength_mg").toDouble(0.0);
    const double doseMg       = med.value("dose_mg").toDouble(0.0);
    const double doseTabs     = med.value("dose_tablets").toDouble(0.0);
    const int timesPerDay     = med.value("times_per_day").toInt(0);
    const auto timesVal       = med.value("times");
    const QString therapy     = med.value("therapy_mode").toString(); // continuous / courses / prn
    const double stockTabs    = med.value("stock_tablets").toDouble(0.0);

    card->setProperty("medId", id);
    m_medicationNamesById[id] = !brandName.isEmpty() ? brandName : inn;

    // Заголовок: название
    auto* titleLbl = new QLabel(brandName.isEmpty() ? tr("Без названия") : brandName, card);
    titleLbl->setObjectName("CardTitle");

    // Подзаголовок: МНН
    auto* innLbl = new QLabel(inn.isEmpty() ? tr("МНН не указано") : inn, card);
    innLbl->setObjectName("MutedLabel");

    // Доза/таблетки
    QString doseLine;
    if (pillStrength > 0 && doseMg > 0 && doseTabs > 0) {
        doseLine = tr("%1 мг в таблетке • %2 мг за приём (%3 таб.)")
                       .arg(QString::number(pillStrength, 'f', pillStrength == (int)pillStrength ? 0 : 1))
                       .arg(QString::number(doseMg, 'f', doseMg == (int)doseMg ? 0 : 1))
                       .arg(QString::number(doseTabs, 'f', (doseTabs == (int)doseTabs) ? 0 : 1));
    } else if (doseMg > 0) {
        doseLine = tr("%1 мг за приём").arg(QString::number(doseMg, 'f', doseMg == (int)doseMg ? 0 : 1));
    } else {
        doseLine = tr("Доза не указана");
    }
    auto* doseLbl = new QLabel(doseLine, card);
    doseLbl->setObjectName("MutedLabel");

    // Режим терапии и частота
    QString therapyText;
    if (therapy == "continuous") {
        therapyText = tr("Постоянная терапия");
    } else if (therapy == "courses") {
        therapyText = tr("Курсами");
    } else if (therapy == "prn") {
        therapyText = tr("По необходимости");
    } else {
        therapyText = tr("Режим не указан");
    }

    QString timesText;
    if (therapy == "prn") {
        timesText = tr("Приём по необходимости");
    } else if (timesPerDay > 0) {
        timesText = tr("%1 раз(а) в день").arg(timesPerDay);
    } else {
        timesText = tr("Частота не указана");
    }

    auto* therapyLbl = new QLabel(
        tr("%1 • %2").arg(therapyText, timesText),
        card);
    therapyLbl->setObjectName("MutedLabel");

    // Время приёмов (если не PRN)
    QString timesLine;
    if (therapy != "prn") {
        QStringList timeLines;
        if (timesVal.isArray()) {
            for (const auto& v : timesVal.toArray()) {
                timeLines << v.toString();
            }
        } else {
            timeLines = timesVal.toString().split(",", Qt::SkipEmptyParts);
        }
        for (QString& t : timeLines) {
            t = t.trimmed();
        }
        timeLines.removeAll(QString());
        if (!timeLines.isEmpty()) {
            timesLine = tr("Время приёмов: %1").arg(timeLines.join(", "));
        }
    }

    QLabel* timesLbl = nullptr;
    if (!timesLine.isEmpty()) {
        timesLbl = new QLabel(timesLine, card);
        timesLbl->setObjectName("MutedLabel");
    }

    // Остаток
    auto* stockBadge = new QLabel(
        tr("Остаток: %1 табл.")
            .arg(QString::number(stockTabs, 'f', (stockTabs == (int)stockTabs) ? 0 : 1)),
        card);
    stockBadge->setObjectName("PillStockBadge");

    layout->addWidget(titleLbl);
    layout->addWidget(innLbl);
    layout->addWidget(doseLbl);
    layout->addWidget(therapyLbl);
    if (timesLbl)
        layout->addWidget(timesLbl);
    layout->addWidget(stockBadge);
    layout->addStretch();

    connect(card, &QWidget::customContextMenuRequested,
            this, &MainWindow::showMedicationMenuFromCard);

    return card;
}


// --- showMedicationMenuFromCard ---
void MainWindow::showMedicationMenuFromCard(const QPoint& pos)
{
    if (!m_medCardsHost)
        return;

    // pos в координатах m_medCardsHost
    QWidget* child = m_medCardsHost->childAt(pos);
    if (!child)
        return;

    // поднимаемся до карточки, где хранится medId
    QWidget* card = child;
    while (card && card != m_medCardsHost && !card->property("medId").isValid()) {
        card = card->parentWidget();
    }
    if (!card || card == m_medCardsHost)
        return;

    const QString medId = card->property("medId").toString();
    if (medId.isEmpty())
        return;

    // вытаскиваем JSON препарата
    QJsonObject medObj;
    const QJsonArray meds = m_core->medicationsJson();
    for (const auto& v : meds) {
        const QJsonObject mo = v.toObject();
        if (mo.value("id").toString() == medId) {
            medObj = mo;
            break;
        }
    }
    if (medObj.isEmpty())
        return;

    QMenu menu(this);
    QAction* editAct   = menu.addAction(tr("Редактировать препарат"));
    QAction* prnAct    = menu.addAction(tr("Приём по необходимости"));
    QAction* deleteAct = menu.addAction(tr("Удалить препарат"));

    const QPoint globalPos = m_medCardsHost->mapToGlobal(pos);
    QAction* chosen = menu.exec(globalPos);
    if (!chosen)
        return;

    if (chosen == editAct) {
        // редактирование через то же всплывающее окно
        openMedicationDialog(medObj);
    } else if (chosen == prnAct) {
        openPrnIntakeDialog(medObj);
    } else if (chosen == deleteAct) {
        m_core->deleteMedication(medId);
    }
}


// --- handleEditMedicationFromCard ---
void MainWindow::handleEditMedicationFromCard()
{
    if (m_contextMenuMedicationId.isEmpty())
        return;

    // найдём препарат по id и заполним форму редактирования
    const QJsonArray meds = m_core->medicationsJson();
    for (const auto& v : meds) {
        const QJsonObject med = v.toObject();
        if (med.value("id").toString() == m_contextMenuMedicationId) {
            m_editMedicationId = m_contextMenuMedicationId;

            m_medTitle->setText(med.value("brand_name").toString());
            // если ты где-то хранишь inn, сюда тоже заполни
            m_medDose->setText(med.value("dose_mg").toVariant().toString());
            m_medTimesPerDay->setValue(med.value("times_per_day").toInt(1));
            m_medTimes->setText(med.value("times").toString());
            m_medStock->setValue(static_cast<int>(med.value("stock_tablets").toDouble(0.0)));

            const QString therapy = med.value("therapy_mode").toString();
            int idx = m_medTherapy->findData(therapy);
            if (idx >= 0)
                m_medTherapy->setCurrentIndex(idx);

            break;
        }
    }
}


// --- handleDeleteMedicationFromCard ---
void MainWindow::handleDeleteMedicationFromCard()
{
    if (m_contextMenuMedicationId.isEmpty())
        return;

    m_core->deleteMedication(m_contextMenuMedicationId);
}


// --- handlePrnIntakeFromMedication ---
void MainWindow::handlePrnIntakeFromMedication()
{
    if (m_contextMenuMedicationId.isEmpty())
        return;

    QJsonObject med;
    const QJsonArray meds = m_core->medicationsJson();
    for (const auto& v : meds) {
        const QJsonObject mo = v.toObject();
        if (mo.value("id").toString() == m_contextMenuMedicationId) {
            med = mo;
            break;
        }
    }
    if (med.isEmpty())
        return;

    openPrnIntakeDialog(med);
}




// --- rebuildMedicationCards ---
void MainWindow::rebuildMedicationCards()
{
    ::clearLayout(m_medCardsLayout);
    m_medicationNamesById.clear();

    if (m_core->medicationsJson().isEmpty()) {
        auto* empty = new QLabel(
            tr("Пока нет ни одного препарата. Нажмите «Добавить препарат», чтобы начать."),
            m_medCardsHost);
        empty->setObjectName("MutedLabel");
        empty->setWordWrap(true);
        m_medCardsLayout->addWidget(empty, 0, 0);
        return;
    }

    int row = 0;
    for (const auto& v : m_core->medicationsJson()) {
        const QJsonObject med = v.toObject();
        auto* card = makeMedicationCard(med);
        m_medCardsLayout->addWidget(card, row++, 0);
    }
}


// --- onMedicationsReady ---
void MainWindow::onMedicationsReady(const QJsonArray& meds)
{
    Q_UNUSED(meds);
    rebuildMedicationCards();
    m_core->fetchMedicationSchedules();
    refreshDayMedicationSummary();
}


// --- createMedicinesPage ---
QWidget* MainWindow::createMedicinesPage()
{
    auto page = new QWidget;
    auto root = new QVBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(16);

    // ─── верх: слева препараты + кнопка, справа календарь ───
    auto* topRow = new QHBoxLayout;
    topRow->setSpacing(16);

    // ЛЕВО: препараты + кнопка
    auto* leftCol = new QVBoxLayout;
    leftCol->setSpacing(8);

    m_medCardsHost = new QWidget(page);
    m_medCardsHost->setObjectName("MedCardsHost");

    m_medCardsLayout = new QGridLayout(m_medCardsHost);
    m_medCardsLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_medCardsLayout->setContentsMargins(0, 0, 0, 0);
    m_medCardsLayout->setSpacing(14);

    auto* cardsScroll = new QScrollArea(page);
    cardsScroll->setWidgetResizable(true);
    cardsScroll->setFrameShape(QFrame::NoFrame);
    cardsScroll->setWidget(m_medCardsHost);

    auto* medsSection = makeSection(
        tr("Препараты"),
        cardsScroll,
        "SectionCard");

    leftCol->addWidget(medsSection, 1);

    // кнопка "Добавить / редактировать"
    auto* editBtnHost = new QWidget(page);
    auto* editBtnLayout = new QHBoxLayout(editBtnHost);
    editBtnLayout->setContentsMargins(0, 0, 0, 0);
    editBtnLayout->setSpacing(8);

    m_openMedicationDialogBtn = new QPushButton(
        tr("Добавить препарат"),
        editBtnHost);
    m_openMedicationDialogBtn->setObjectName("PrimaryButton");

    editBtnLayout->addWidget(m_openMedicationDialogBtn, 0, Qt::AlignLeft);
    editBtnLayout->addStretch();

    leftCol->addWidget(editBtnHost, 0);

    auto* leftHost = new QWidget(page);
    leftHost->setLayout(leftCol);

    topRow->addWidget(leftHost, 3);

    // ПРАВО: календарь
    m_medCalendar = new TracklyCalendar(page);
    m_medCalendar->setObjectName("MainCalendar");
    m_medCalendar->setMode(1); // терапия: приёмы + приступы

    auto* calendarSection = makeSection(
        tr("Календарь приёмов"),
        m_medCalendar,
        "SectionCard");

    topRow->addWidget(calendarSection, 4);

    root->addLayout(topRow, 3);

    // ─── низ: приёмы за день на всю ширину ───
    m_dayMedicationHost = new QWidget(page);
    m_dayMedicationHost->setObjectName("DayMedicationHost");

    m_dayMedicationLayout = new QVBoxLayout(m_dayMedicationHost);
    m_dayMedicationLayout->setContentsMargins(0, 0, 0, 0);
    m_dayMedicationLayout->setSpacing(10);

    auto* dayEmpty = new QLabel(
        tr("Выберите день в календаре, чтобы увидеть приёмы."),
        m_dayMedicationHost);
    dayEmpty->setObjectName("MutedLabel");
    dayEmpty->setWordWrap(true);
    m_dayMedicationLayout->addWidget(dayEmpty);
    m_dayMedicationLayout->addStretch();

    auto* dayScroll = new QScrollArea(page);
    dayScroll->setWidgetResizable(true);
    dayScroll->setFrameShape(QFrame::NoFrame);
    dayScroll->setWidget(m_dayMedicationHost);

    auto* daySection = makeSection(
        tr("Приёмы за день"),
        dayScroll,
        "SectionCard");

    root->addWidget(daySection, 2);

    // контекстное меню для карточек препаратов
    m_medCardsHost->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_medCardsHost, &QWidget::customContextMenuRequested,
            this, &MainWindow::showMedicationMenuFromCard);

    // кнопка «Добавить / редактировать»
    connect(m_openMedicationDialogBtn, &QPushButton::clicked,
            this, [this]() {
                // пустой объект -> создание
                this->openMedicationDialog(QJsonObject());
            });

    // календарь
    connect(m_medCalendar, &TracklyCalendar::selectionChanged,
            this, &MainWindow::refreshDayMedicationSummary);

    return page;
}


// --- openMedicationDialog ---
void MainWindow::openMedicationDialog(const QJsonObject& existingMed)
{
    auto* central = this->centralWidget();
    QGraphicsBlurEffect* blur = nullptr;
    if (central) {
        blur = new QGraphicsBlurEffect(central);
        blur->setBlurRadius(6.0);
        blur->setBlurHints(QGraphicsBlurEffect::QualityHint);
        central->setGraphicsEffect(blur);
    }

    QDialog dlg(this);
    dlg.setModal(true);
    dlg.setWindowFlag(Qt::FramelessWindowHint, true);
    dlg.setAttribute(Qt::WA_TranslucentBackground, true);
    dlg.setWindowTitle(existingMed.isEmpty()
                           ? tr("Добавление препарата")
                           : tr("Редактирование препарата"));

    auto* outer = new QVBoxLayout(&dlg);
    outer->setContentsMargins(16, 16, 16, 16);
    outer->setSpacing(12);

    auto* formFrame = new QFrame(&dlg);
    formFrame->setObjectName("SectionCard");
    auto* formLayout = new QVBoxLayout(formFrame);
    formLayout->setContentsMargins(16, 16, 16, 16);
    formLayout->setSpacing(10);

    auto* grid = new QGridLayout;
    grid->setSpacing(10);

    int row = 0;

    // Название
    auto* titleLbl = new QLabel(tr("Название препарата"), formFrame);
    titleLbl->setObjectName("MutedLabel");
    auto* titleEdit = new QLineEdit(formFrame);
    titleEdit->setPlaceholderText(tr("Брендовое название..."));
    grid->addWidget(titleLbl, row, 0);
    grid->addWidget(titleEdit, row, 1);
    row++;

    // МНН
    auto* innLbl = new QLabel(tr("МНН"), formFrame);
    innLbl->setObjectName("MutedLabel");
    auto* innEdit = new QLineEdit(formFrame);
    innEdit->setPlaceholderText(tr("Международное непатентованное..."));
    grid->addWidget(innLbl, row, 0);
    grid->addWidget(innEdit, row, 1);
    row++;

    // Режим терапии
    auto* therapyLbl = new QLabel(tr("Режим терапии"), formFrame);
    therapyLbl->setObjectName("MutedLabel");
    auto* therapyCombo = new QComboBox(formFrame);
    therapyCombo->addItem(tr("Постоянная"), "continuous");
    therapyCombo->addItem(tr("Курсами"), "courses");
    therapyCombo->addItem(tr("По необходимости"), "prn");
    grid->addWidget(therapyLbl, row, 0);
    grid->addWidget(therapyCombo, row, 1);
    row++;

    // Мг в одной таблетке
    auto* pillStrengthLbl = new QLabel(tr("Сила таблетки, мг"), formFrame);
    pillStrengthLbl->setObjectName("MutedLabel");
    auto* pillStrengthEdit = new QLineEdit(formFrame);
    pillStrengthEdit->setPlaceholderText(tr("Например, 400"));
    grid->addWidget(pillStrengthLbl, row, 0);
    grid->addWidget(pillStrengthEdit, row, 1);
    row++;

    // Разовая доза, мг
    auto* doseLbl = new QLabel(tr("Разовая доза, мг"), formFrame);
    doseLbl->setObjectName("MutedLabel");
    auto* doseEdit = new QLineEdit(formFrame);
    doseEdit->setPlaceholderText(tr("Например, 600"));
    grid->addWidget(doseLbl, row, 0);
    grid->addWidget(doseEdit, row, 1);
    row++;

    // Приёмов в день (для не-PRN)
    auto* timesPerDayLbl = new QLabel(tr("Приёмов в день"), formFrame);
    timesPerDayLbl->setObjectName("MutedLabel");
    auto* timesPerDaySpin = new QSpinBox(formFrame);
    timesPerDaySpin->setMinimum(1);
    timesPerDaySpin->setMaximum(12);
    timesPerDaySpin->setValue(2);
    grid->addWidget(timesPerDayLbl, row, 0);
    grid->addWidget(timesPerDaySpin, row, 1);
    row++;

    // Время приёмов (строка с HH:MM, через запятую)
    auto* timesLbl = new QLabel(tr("Время приёмов"), formFrame);
    timesLbl->setObjectName("MutedLabel");
    auto* timesEdit = new QLineEdit(formFrame);
    timesEdit->setPlaceholderText(tr("Например: 08:00, 20:00"));
    grid->addWidget(timesLbl, row, 0);
    grid->addWidget(timesEdit, row, 1);
    row++;

    // Остаток, табл.
    auto* stockLbl = new QLabel(tr("Остаток, табл."), formFrame);
    stockLbl->setObjectName("MutedLabel");
    auto* stockSpin = new QSpinBox(formFrame);
    stockSpin->setMinimum(0);
    stockSpin->setMaximum(100000);
    stockSpin->setValue(0);
    grid->addWidget(stockLbl, row, 0);
    grid->addWidget(stockSpin, row, 1);
    row++;

    // Дата начала графика приёма
    auto* startDateLbl = new QLabel(tr("Начало графика"), formFrame);
    startDateLbl->setObjectName("MutedLabel");
    auto* startDateEdit = new QDateEdit(QDate::currentDate(), formFrame);
    startDateEdit->setCalendarPopup(true);
    startDateEdit->setDisplayFormat("dd.MM.yyyy");
    grid->addWidget(startDateLbl, row, 0);
    grid->addWidget(startDateEdit, row, 1);
    row++;

    // Дата окончания графика приёма
    auto* endDateLbl = new QLabel(tr("Конец графика"), formFrame);
    endDateLbl->setObjectName("MutedLabel");
    auto* endDateEdit = new QDateEdit(QDate::currentDate().addMonths(1), formFrame);
    endDateEdit->setCalendarPopup(true);
    endDateEdit->setDisplayFormat("dd.MM.yyyy");
    auto* noEndDateCheck = new QCheckBox(tr("Без конечной даты"), formFrame);
    grid->addWidget(endDateLbl, row, 0);
    grid->addWidget(endDateEdit, row, 1);
    row++;
    grid->addWidget(new QLabel("", formFrame), row, 0);
    grid->addWidget(noEndDateCheck, row, 1);
    row++;
    connect(noEndDateCheck, &QCheckBox::toggled, endDateEdit, &QWidget::setDisabled);

    formLayout->addLayout(grid);

    // Кнопки
    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto* cancelBtn = new QPushButton(tr("Отмена"), formFrame);
    auto* saveBtn   = new QPushButton(existingMed.isEmpty() ? tr("Добавить") : tr("Сохранить"), formFrame);
    saveBtn->setObjectName("PrimaryButton");
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(saveBtn);
    formLayout->addLayout(btnRow);

    outer->addWidget(formFrame);

    // Если редактируем – заполнить поля
    const QString existingId = existingMed.value("id").toString();
    if (!existingMed.isEmpty()) {
        titleEdit->setText(existingMed.value("brand_name").toString());
        innEdit->setText(existingMed.value("inn").toString());
        doseEdit->setText(QString::number(existingMed.value("dose_mg").toDouble(0.0), 'f', 0));
        pillStrengthEdit->setText(QString::number(existingMed.value("pill_strength_mg").toDouble(0.0), 'f', 0));
        timesPerDaySpin->setValue(existingMed.value("times_per_day").toInt(1));
        stockSpin->setValue(static_cast<int>(existingMed.value("stock_tablets").toDouble(0.0)));

        // therapy
        const QString therapy = existingMed.value("therapy_mode").toString();
        int idx = therapyCombo->findData(therapy);
        if (idx >= 0)
            therapyCombo->setCurrentIndex(idx);

        // times
        QStringList timeLines;
        const auto timesVal = existingMed.value("times");
        if (timesVal.isArray()) {
            for (const auto& v : timesVal.toArray())
                timeLines << v.toString();
        } else {
            timeLines = timesVal.toString().split(",", Qt::SkipEmptyParts);
        }
        for (QString& t : timeLines)
            t = t.trimmed();
        timeLines.removeAll(QString());
        timesEdit->setText(timeLines.join(", "));

        // pre-fill dates from local map
        if (m_medicationScheduleDates.contains(existingId)) {
            const auto& dates = m_medicationScheduleDates[existingId];
            if (dates.first.isValid()) startDateEdit->setDate(dates.first);
            if (dates.second.isValid()) {
                endDateEdit->setDate(dates.second);
                noEndDateCheck->setChecked(false);
            } else {
                noEndDateCheck->setChecked(true);
                endDateEdit->setDisabled(true);
            }
        }
    }

    // therapy == prn → скрываем поля планирования
    auto updatePlanVisibility = [therapyCombo, timesPerDayLbl, timesPerDaySpin, timesLbl, timesEdit,
                                  startDateLbl, startDateEdit, endDateLbl, endDateEdit,
                                  noEndDateCheck]() {
        const QString mode = therapyCombo->currentData().toString();
        const bool isPrn = (mode == "prn");
        timesPerDayLbl->setVisible(!isPrn);
        timesPerDaySpin->setVisible(!isPrn);
        timesLbl->setVisible(!isPrn);
        timesEdit->setVisible(!isPrn);
        startDateLbl->setVisible(!isPrn);
        startDateEdit->setVisible(!isPrn);
        endDateLbl->setVisible(!isPrn);
        endDateEdit->setVisible(!isPrn);
        noEndDateCheck->setVisible(!isPrn);
    };
    updatePlanVisibility();
    connect(therapyCombo, &QComboBox::currentIndexChanged,
            &dlg, [updatePlanVisibility]() { updatePlanVisibility(); });

    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    connect(saveBtn, &QPushButton::clicked, &dlg,
            [this,
             &dlg,
             titleEdit,
             innEdit,
             therapyCombo,
             pillStrengthEdit,
             doseEdit,
             timesPerDaySpin,
             timesEdit,
             stockSpin,
             startDateEdit,
             endDateEdit,
             noEndDateCheck,
             existingId]() {

                QJsonObject obj;
                obj["user_id"]          = m_core->currentUserId();
                obj["brand_name"]      = titleEdit->text().trimmed();
                obj["inn"]             = innEdit->text().trimmed();
                obj["therapy_mode"]    = therapyCombo->currentData().toString();

                const double pillStrength = pillStrengthEdit->text().trimmed().toDouble();
                const double doseMg       = doseEdit->text().trimmed().toDouble();
                double doseTabs = 0.0;
                if (pillStrength > 0.0 && doseMg > 0.0)
                    doseTabs = doseMg / pillStrength;

                obj["pill_strength_mg"] = pillStrength;
                obj["dose_mg"]          = doseMg;
                obj["dose_tablets"]     = doseTabs;
                obj["stock_tablets"]    = stockSpin->value();

                if (obj.value("therapy_mode").toString() == "prn") {
                    obj["times_per_day"] = 0;
                    obj["times"] = QString();
                } else {
                    obj["times_per_day"] = timesPerDaySpin->value();

                    // храним как строку "08:00, 14:00, 20:00" (не JSON-массив)
                    QStringList normalized;
                    const QStringList parts = timesEdit->text().split(",", Qt::SkipEmptyParts);
                    for (QString t : parts) {
                        t = t.trimmed();
                        if (!t.isEmpty()) {
                            const QTime ti = QTime::fromString(t, "HH:mm");
                            normalized.append(ti.isValid() ? ti.toString("HH:mm") : t);
                        }
                    }
                    obj["times"] = normalized.join(", ");
                }

                // сохраняем даты графика
                const QDate sd = startDateEdit->date();
                const QDate ed = noEndDateCheck->isChecked() ? QDate() : endDateEdit->date();

                if (existingId.isEmpty()) {
                    m_pendingMedStartDate = sd;
                    m_pendingMedEndDate   = ed;
                    m_pendingMedNoEndDate = noEndDateCheck->isChecked();
                    m_core->createMedication(obj);
                } else {
                    m_medicationScheduleDates[existingId] = {sd, ed};
                    // сохраняем в QSettings для персистентности
                    QSettings s("Trackly", "Trackly");
                    s.setValue("medication/" + existingId + "/sched_start", sd);
                    s.setValue("medication/" + existingId + "/sched_end", ed);
                    m_core->updateMedication(existingId, obj);
                }

                dlg.accept();
            });

    dlg.exec();

    if (central) {
        central->setGraphicsEffect(nullptr);
    }
}


// --- openMarkIntakeDialog ---
void MainWindow::openMarkIntakeDialog(const QJsonObject& med,
                                      const QString& plannedTime)
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Отметить приём"));
    dlg.setModal(true);
    dlg.setWindowFlag(Qt::FramelessWindowHint, true);
    dlg.setAttribute(Qt::WA_TranslucentBackground, true);

    auto* outerLayout = new QVBoxLayout(&dlg);
    outerLayout->setContentsMargins(16, 16, 16, 16);
    outerLayout->setSpacing(12);

    auto* frame = new QFrame(&dlg);
    frame->setObjectName("SectionCard");
    auto* root = new QVBoxLayout(frame);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    const QString brand = med.value("brand_name").toString();
    const QString inn   = med.value("inn").toString();

    auto* titleLbl = new QLabel(
        tr("Препарат: %1").arg(brand.isEmpty() ? inn : brand),
        &dlg);
    titleLbl->setObjectName("CardTitle");

    auto* plannedLbl = new QLabel(
        plannedTime.isEmpty()
            ? tr("По расписанию: не задано")
            : tr("По расписанию: %1").arg(plannedTime),
        &dlg);
    plannedLbl->setObjectName("MutedLabel");

    auto* dateEdit = new QDateEdit(QDate::currentDate(), &dlg);
    dateEdit->setCalendarPopup(true);

    auto* timeEdit = new QTimeEdit(&dlg);
    timeEdit->setDisplayFormat("HH:mm");
    if (!plannedTime.isEmpty()) {
        const QTime t = QTime::fromString(plannedTime, "HH:mm");
        timeEdit->setTime(t.isValid() ? t : QTime::currentTime());
    } else {
        timeEdit->setTime(QTime::currentTime());
    }

    auto* dateRow = new QHBoxLayout;
    dateRow->addWidget(new QLabel(tr("Дата фактического приёма:"), &dlg));
    dateRow->addWidget(dateEdit);

    auto* timeRow = new QHBoxLayout;
    timeRow->addWidget(new QLabel(tr("Время фактического приёма:"), &dlg));
    timeRow->addWidget(timeEdit);

    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                        Qt::Horizontal,
                                        &dlg);

    root->addWidget(titleLbl);
    root->addWidget(plannedLbl);
    root->addLayout(dateRow);
    root->addLayout(timeRow);
    root->addWidget(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    outerLayout->addWidget(frame);

    showBlurredDialog(&dlg);

    if (dlg.result() != QDialog::Accepted)
        return;

    const QDate date = dateEdit->date();
    const QTime time = timeEdit->time();
    const QDateTime dtLocal(date, time);

    // Собираем лог для createMedicationLog
    QJsonObject log;
    log["user_id"]       = m_core->currentUserId();
    log["medication_id"] = med.value("id").toString();
    log["schedule_id"]   = QJsonValue(); // приём вне конкретного schedule
    log["taken_at"]      = dtLocal.toUTC().toString(Qt::ISODate);
    log["status"]        = "taken";

    m_core->createMedicationLog(log);
}



// --- submitMedicationForm ---
void MainWindow::submitMedicationForm()
{
    const QString brand   = m_medTitle ? m_medTitle->text().trimmed() : QString();
    const QString inn     = m_medInn   ? m_medInn->text().trimmed()   : QString();
    const QString therapy = m_medTherapy ? m_medTherapy->currentText().trimmed()
                                         : QString();
    const QString doseStr = m_medDose ? m_medDose->text().trimmed()   : QString();
    const QString timesRaw= m_medTimes ? m_medTimes->text().trimmed() : QString();

    if (brand.isEmpty() && inn.isEmpty()) {
        showToast(m_central,
                          tr("Укажите бренд или МНН препарата."),
                          ToastKind::Error,
                          2600);
        return;
    }
    if (doseStr.isEmpty()) {
        showToast(m_central,
                          tr("Введите дозу в мг."),
                          ToastKind::Error,
                          2600);
        return;
    }
    if (timesRaw.isEmpty()) {
        showToast(m_central,
                          tr("Укажите время приёмов."),
                          ToastKind::Error,
                          2600);
        return;
    }

    QString doseNorm = doseStr;
    doseNorm.replace(',', '.');

    bool okDoseMg = false;
    const double doseMg = doseNorm.toDouble(&okDoseMg);
    if (!okDoseMg || doseMg <= 0.0) {
        showToast(m_central,
                          tr("Некорректная доза в мг."),
                          ToastKind::Error,
                          2600);
        return;
    }

    // разовая доза в табл., шаг 0.25
    double doseTabs = m_medDoseTablets ? m_medDoseTablets->value() : 1.0;
    if (doseTabs <= 0.0)
        doseTabs = 1.0;
    doseTabs = std::round(doseTabs / 0.25) * 0.25;

    // времена
    QJsonArray timesArray;
    const QStringList timesList = timesRaw.split(',', Qt::SkipEmptyParts);
    for (QString t : timesList) {
        t = t.trimmed();
        if (!t.isEmpty())
            timesArray.append(t);
    }
    if (timesArray.isEmpty()) {
        showToast(m_central,
                          tr("Укажите хотя бы одно корректное время приёма."),
                          ToastKind::Error,
                          2600);
        return;
    }

    const int timesPerDay = m_medTimesPerDay
                                ? m_medTimesPerDay->value()
                                : timesArray.size();

    // остаток, табл.
    double stockTabs = m_medStockTablets ? m_medStockTablets->value() : 0.0;
    if (stockTabs < 0.0)
        stockTabs = 0.0;
    stockTabs = std::round(stockTabs / 0.25) * 0.25;

    QJsonObject payload;
    payload["user_id"]        = m_core->currentUserId();
    payload["brand_name"]    = brand;
    payload["inn"]           = inn;
    payload["therapy_mode"]  = therapy;
    payload["dose_mg"]       = QString::number(doseMg, 'f', 0);
    payload["dose_tablets"]  = doseTabs;
    payload["times_per_day"] = timesPerDay;
    payload["times"]         = timesArray;
    payload["stock_tablets"] = stockTabs;

    if (m_editMedicationId.isEmpty()) {
        m_core->createMedication(payload);
    } else {
        m_core->updateMedication(m_editMedicationId, payload);
    }

    m_editMedicationId.clear();
    resetMedicationForm();
}


// --- resetMedicationForm ---
void MainWindow::resetMedicationForm()
{
    m_editMedicationId.clear();

    if (m_saveMedicationBtn)
        m_saveMedicationBtn->setText(tr("Сохранить препарат"));

    if (m_medTitle)
        m_medTitle->clear();
    if (m_medInn)
        m_medInn->clear();
    if (m_medTherapy)
        m_medTherapy->setCurrentIndex(0);
    if (m_medDose)
        m_medDose->clear();
    if (m_medDoseTablets)
        m_medDoseTablets->setValue(1.0);
    if (m_medTimesPerDay)
        m_medTimesPerDay->setValue(1);
    if (m_medTimes)
        m_medTimes->clear();
    if (m_medStockTablets)
        m_medStockTablets->setValue(0.0);
}


// --- editMedication ---
void MainWindow::editMedication(const QJsonObject& med)
{
    m_editMedicationId = med.value("id").toString();
    if (m_saveMedicationBtn)
        m_saveMedicationBtn->setText(tr("Сохранить изменения"));

    if (m_medTitle)
        m_medTitle->setText(med.value("brand_name").toString());
    if (m_medInn)
        m_medInn->setText(med.value("inn").toString());
    if (m_medTherapy) {
        const QString therapy = med.value("therapy_mode").toString();
        int idx = m_medTherapy->findText(therapy);
        if (idx < 0) idx = 0;
        m_medTherapy->setCurrentIndex(idx);
    }
    if (m_medDose)
        m_medDose->setText(med.value("dose_mg").toString());

    if (m_medDoseTablets) {
        double tabs = med.value("dose_tablets").toDouble(1.0);
        if (tabs <= 0.0) tabs = 1.0;
        m_medDoseTablets->setValue(tabs);
    }

    if (m_medTimesPerDay)
        m_medTimesPerDay->setValue(med.value("times_per_day").toInt(1));

    if (m_medTimes) {
        QString timesStr;
        const auto timesVal = med.value("times");
        if (timesVal.isArray()) {
            QStringList parts;
            for (const auto& v : timesVal.toArray())
                parts << v.toString();
            timesStr = parts.join(", ");
        } else {
            timesStr = timesVal.toString();
        }
        m_medTimes->setText(timesStr);
    }

    if (m_medStockTablets) {
        double stockTabs = med.value("stock_tablets").toDouble(0.0);
        if (stockTabs < 0.0) stockTabs = 0.0;
        m_medStockTablets->setValue(stockTabs);
    }

    navigateTo(2, tr("Терапия"));
}


// --- editMedicationFromCard ---
void MainWindow::editMedicationFromCard()
{
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn)
        return;

    const QString medId = btn->property("medId").toString();
    if (medId.isEmpty())
        return;

    const QJsonArray meds = m_core->medicationsJson(); // геттер к m_cachedMedications
    for (const auto& v : meds) {
        const QJsonObject o = v.toObject();
        if (o.value("id").toString() == medId) {
            // Заполняем форму
            m_medTitle->setText(o.value("title").toString());
            const QString therapy = o.value("therapy").toString();
            const int idx = m_medTherapy->findData(therapy);
            if (idx >= 0)
                m_medTherapy->setCurrentIndex(idx);
            m_medDose->setText(o.value("dose").toString());
            m_medTimesPerDay->setValue(o.value("times_per_day").toInt(1));

            // times может быть строкой или массивом
            if (o.value("times").isArray()) {
                QStringList times;
                for (const auto& t : o.value("times").toArray()) {
                    times << t.toString();
                }
                m_medTimes->setText(times.join(", "));
            } else {
                m_medTimes->setText(o.value("times").toString());
            }

            m_medStock->setValue(o.value("stock").toInt(0));

            // можно сохранить текущий редактируемый id в property, если нужно при сохранении
            m_saveMedicationBtn->setProperty("editingMedId", medId);

            break;
        }
    }
}


// --- toggleMedicationArchive ---
void MainWindow::toggleMedicationArchive()
{
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn)
        return;

    const QString medId = btn->property("medId").toString();
    if (medId.isEmpty())
        return;

    const bool archived = btn->property("archived").toBool();
    const bool newArchived = !archived;

    // Если есть метод в AppCore, дергаем его:
    if (m_core->metaObject()->indexOfMethod("setMedicationArchived(QString,bool)") != -1) {
        QMetaObject::invokeMethod(m_core, "setMedicationArchived",
                                  Q_ARG(QString, medId),
                                  Q_ARG(bool, newArchived));
    }

    // Локально просто перерисуем карточки после обновления списка
    m_core->fetchMedications(); // если есть такой метод, у тебя он уже есть

    // В любом случае сбросим локальное свойство, UI обновится при приходе новых данных
}


// --- deleteMedication ---
void MainWindow::deleteMedication(const QJsonObject& med)
{
    const QString id = med.value("id").toString();
    if (id.isEmpty()) return;

    if (showConfirmDialog(tr("Удалить препарат"),
                          tr("Удалить \"%1\"?").arg(med.value("title").toString()))) {
        m_core->deleteMedication(id);
        if (m_editMedicationId == id) resetMedicationForm();
    }
}


// --- showMedicationContextMenu ---
void MainWindow::showMedicationContextMenu(const QPoint& pos)
{
    auto* card = qobject_cast<QWidget*>(sender());
    if (!card)
        return;

    const QString medId = card->property("medId").toString();
    if (medId.isEmpty())
        return;

    QMenu menu(card);
    QAction* takeNow = menu.addAction(tr("Отметить приём сейчас"));
    QAction* editAct = menu.addAction(tr("Редактировать"));
    menu.addSeparator();
    QAction* deleteAct = menu.addAction(tr("Удалить препарат"));

    QAction* chosen = menu.exec(card->mapToGlobal(pos));
    if (!chosen)
        return;

    if (chosen == takeNow) {
        const QJsonArray meds = m_core->medicationsJson();
        for (const auto& v : meds) {
            const auto o = v.toObject();
            if (o.value("id").toString() == medId) {
                markMedicationTaken(o,
                                    m_medCalendar ? m_medCalendar->selectedDate()
                                                  : QDate::currentDate(),
                                    QString());
                break;
            }
        }
    } else if (chosen == editAct) {
        const QJsonArray meds = m_core->medicationsJson();
        for (const auto& v : meds) {
            const auto o = v.toObject();
            if (o.value("id").toString() == medId) {
                editMedication(o);
                break;
            }
        }
    } else if (chosen == deleteAct) {
        if (showConfirmDialog(tr("Удалить препарат"), tr("Удалить препарат?"))) {
            m_core->deleteMedication(medId);
            if (m_editMedicationId == medId)
                resetMedicationForm();
        }
    }
}


// --- showMedicationMenu ---
void MainWindow::showMedicationMenu(const QJsonObject& med, QWidget* anchor)
{
    if (!anchor) return;

    QMenu menu(anchor);
    QAction* markTaken = menu.addAction(tr("Отметить приём"));
    QAction* editAct = menu.addAction(tr("Редактировать"));
    menu.addSeparator();
    QAction* deleteAct = menu.addAction(tr("Удалить"));

    QAction* chosen = menu.exec(anchor->mapToGlobal(anchor->rect().bottomLeft()));
    if (!chosen) return;

    if (chosen == markTaken) {
        QString firstTime;
        if (med.value("times").isArray() && !med.value("times").toArray().isEmpty()) {
            firstTime = med.value("times").toArray().first().toString();
        } else {
            firstTime = med.value("times").toString().split(',', Qt::SkipEmptyParts).value(0).trimmed();
        }
        markMedicationTaken(med, m_medCalendar ? m_medCalendar->selectedDate() : QDate::currentDate(), firstTime);
        return;
    }

    if (chosen == editAct) {
        editMedication(med);
        return;
    }

    if (chosen == deleteAct) {
        deleteMedication(med);
    }
}


// --- fillMedications ---
void MainWindow::fillMedications(const QJsonArray& arr)
{
    m_cachedMedications = arr;
    m_datesWithMedication.clear();

    // загружаем даты графиков из QSettings
    QSettings s("Trackly", "Trackly");
    for (const auto& v : arr) {
        const QString medId = v.toObject().value("id").toString();
        if (medId.isEmpty()) continue;
        QDate sd = s.value("medication/" + medId + "/sched_start").toDate();
        QDate ed = s.value("medication/" + medId + "/sched_end").toDate();
        if (sd.isValid() || ed.isValid())
            m_medicationScheduleDates[medId] = {sd, ed};
    }

    ::clearLayout(m_medCardsLayout);

    if (arr.isEmpty()) {
        auto empty = new QLabel(tr("Пока нет препаратов. Добавьте первый справа."), m_medCardsHost);
        empty->setObjectName("MutedLabel");
        empty->setWordWrap(true);
        m_medCardsLayout->addWidget(empty, 0, 0);
        refreshDayMedicationSummary();
        refreshAnalyticsPage();
        return;
    }

    int row = 0;
    int col = 0;

    for (const auto& v : arr) {
        const auto o = v.toObject();

        auto card = new QFrame(m_medCardsHost);
        card->setObjectName("SectionCard");
        card->setCursor(Qt::PointingHandCursor);
        card->setContextMenuPolicy(Qt::CustomContextMenu);

        auto cl = new QVBoxLayout(card);
        cl->setContentsMargins(18, 18, 18, 18);
        cl->setSpacing(8);

        const QString medId     = o.value("id").toString();
        const QString brandName = o.value("title").toString();          // бренд
        const QString inn       = o.value("inn").toString();            // МНН
        const QString doseMg    = o.value("dosage").toString();         // мг
        const QString therapy   = o.value("therapymode").toString();    // режим
        const auto    timesVal  = o.value("times");                     // массив/строка
        const int     stockInt  = o.value("stock").toInt(0);
        const double  stockTabs = o.value("stock_tablets").toDouble(0);

        card->setProperty("medId", medId);

        auto title = new QLabel(brandName.isEmpty() ? tr("Без названия") : brandName, card);
        title->setObjectName("CardTitle");

        auto innLbl = new QLabel(inn.isEmpty() ? tr("МНН не указано") : inn, card);
        innLbl->setObjectName("MutedLabel");

        auto dose = new QLabel(
            doseMg.isEmpty()
                ? tr("Доза не указана")
                : tr("Разовая доза: %1 мг").arg(doseMg),
            card);
        dose->setObjectName("MutedLabel");

        QStringList timeLines;
        if (timesVal.isArray()) {
            for (const auto& tv : timesVal.toArray()) {
                const QString t = tv.toString().trimmed();
                if (!t.isEmpty())
                    timeLines << t;
            }
        } else {
            const QString timesStr = timesVal.toString();
            const QStringList parts = timesStr.split(',', Qt::SkipEmptyParts);
            for (QString t : parts) {
                t = t.trimmed();
                if (!t.isEmpty())
                    timeLines << t;
            }
        }
        const QString timesText = timeLines.isEmpty()
                                      ? tr("не задано")
                                      : timeLines.join(", ");

        auto times = new QLabel(tr("Приём: %1").arg(timesText), card);
        times->setObjectName("MutedLabel");
        times->setWordWrap(true);

        auto therapyLbl = new QLabel(
            tr("Режим: %1").arg(therapy.isEmpty() ? tr("не указан") : therapy),
            card);
        therapyLbl->setObjectName("MutedLabel");

        QString stockText;
        if (stockTabs > 0.0)
            stockText = QString::number(stockTabs, 'f', 1);
        else
            stockText = QString::number(stockInt);

        auto stockLbl = new QLabel(tr("Остаток: %1 табл.").arg(stockText), card);
        stockLbl->setObjectName("PillStockBadge");

        auto todayBtn = new QPushButton(tr("Приёмы сегодня"), card);
        todayBtn->setObjectName("SecondaryButton");
        todayBtn->setProperty("medId", medId);

        cl->addWidget(title);
        cl->addWidget(innLbl);
        cl->addWidget(dose);
        cl->addWidget(times);
        cl->addWidget(therapyLbl);
        cl->addWidget(stockLbl);

        // красная плашка если остаток < 10 таблеток
        const double currentStock = qMax(stockTabs, static_cast<double>(stockInt));
        if (currentStock > 0.0 && currentStock < 10.0) {
            auto* lowStockWarn = new QLabel(
                tr("⚠️ Осталось менее 10 таблеток!"), card);
            lowStockWarn->setObjectName("LowStockWarning");
            lowStockWarn->setWordWrap(true);
            cl->addWidget(lowStockWarn);
        }

        cl->addStretch();
        cl->addWidget(todayBtn);

        connect(todayBtn, &QPushButton::clicked, this, [this, medId]() {
            showIntakesForMedicationToday(medId);
        });

        connect(card, &QWidget::customContextMenuRequested, this,
                [this, o, card](const QPoint& pos) {
                    showMedicationMenu(o, card);
                });

        m_medCardsLayout->addWidget(card, row, col);
        col++;
        if (col > 1) { // 2 колонки
            col = 0;
            row++;
        }
    }

    refreshDayMedicationSummary();
    refreshAnalyticsPage();
}


// --- fillMedicationLogs ---
void MainWindow::fillMedicationLogs(const QJsonArray& arr)
{
    m_cachedMedicationLogs = arr;
    m_datesWithMedication.clear();

    for (const auto& v : arr) {
        const auto o = v.toObject();
        QDateTime dt = QDateTime::fromString(o.value("taken_at").toString(), Qt::ISODate);
        if (!dt.isValid()) dt = QDateTime::fromString(o.value("taken_at").toString(), Qt::ISODateWithMs);
        if (dt.isValid()) m_datesWithMedication.insert(dt.date());
    }

    refreshDayMedicationSummary();
    updateDashboardCalendarDecorations();
    refreshAnalyticsPage();
}

// --- showIntakesForMedicationToday ---
void MainWindow::showIntakesForMedicationToday(const QString& medId)
{
    Q_UNUSED(medId);

    if (!m_medsTab || !m_intakesDateEdit)
        return;

    m_intakesDateEdit->setDate(QDate::currentDate());
    m_medsTab->setCurrentIndex(1); // вкладка "Приёмы"
    refreshTodayIntakesList();
}



// --- createMedicationSchedulesFor ---
void MainWindow::createMedicationSchedulesFor(const QJsonObject& med)
{
    const QString medId = med.value("id").toString();
    if (medId.isEmpty())
        return;

    // собираем времена приёма
    QJsonArray timesArray;
    const auto timesVal = med.value("times");

    if (timesVal.isArray()) {
        timesArray = timesVal.toArray();
    } else {
        const QString timesStr = timesVal.toString();
        const QStringList parts = timesStr.split(',', Qt::SkipEmptyParts);
        for (QString t : parts) {
            t = t.trimmed();
            if (!t.isEmpty())
                timesArray.append(t);
        }
    }

    if (timesArray.isEmpty())
        return;

    const QString userId    = m_core->currentUserId();
    const QString brandName = med.value("brand_name").toString();
    const QString inn       = med.value("inn").toString();
    const QString doseMgStr = med.value("dose_mg").toVariant().toString();

    const QString baseName  = !brandName.isEmpty() ? brandName : inn;

    for (const auto& v : timesArray) {
        const QString timeStr = v.toString().trimmed();
        if (timeStr.isEmpty())
            continue;

        // красивый label для строки расписания
        QString label = timeStr;
        if (!baseName.isEmpty()) {
            if (!doseMgStr.isEmpty())
                label = QStringLiteral("%1 %2 мг — %3")
                            .arg(baseName, doseMgStr, timeStr);
            else
                label = QStringLiteral("%1 — %2").arg(baseName, timeStr);
        }

        QJsonObject sched;
        sched["user_id"]        = userId;
        sched["medication_id"] = medId;
        sched["intake_time"]   = timeStr;  // имя колонки времени
        sched["label"]         = label;    // NOT NULL

        m_core->createMedicationSchedule(sched);
    }
}


// --- openPrnIntakeDialog ---
void MainWindow::openPrnIntakeDialog(const QJsonObject& med)
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Приём по необходимости"));
    dlg.setModal(true);
    dlg.setWindowFlag(Qt::FramelessWindowHint, true);
    dlg.setAttribute(Qt::WA_TranslucentBackground, true);

    auto* outerLayout = new QVBoxLayout(&dlg);
    outerLayout->setContentsMargins(16, 16, 16, 16);
    outerLayout->setSpacing(12);

    auto* frame = new QFrame(&dlg);
    frame->setObjectName("SectionCard");
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    const QString brandName = med.value("brand_name").toString();
    const QString inn       = med.value("inn").toString();
    const QString doseMgStr = med.value("dose_mg").toVariant().toString();
    const double  doseTabs  = med.value("dose_tablets").toDouble(1.0);

    const QString mainName = !brandName.isEmpty() ? brandName : inn;

    auto* title = new QLabel(
        tr("Препарат: %1").arg(mainName.isEmpty() ? tr("Без названия") : mainName),
        &dlg);
    title->setObjectName("SectionTitle");

    auto* doseLbl = new QLabel(
        tr("Разовая доза: %1 мг · %2 табл.")
            .arg(doseMgStr.isEmpty() ? tr("не указана") : doseMgStr)
            .arg(QString::number(doseTabs, 'f', 2)),
        &dlg);
    doseLbl->setObjectName("MutedLabel");

    auto* dateTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime(), &dlg);
    dateTimeEdit->setCalendarPopup(true);
    dateTimeEdit->setDisplayFormat("dd.MM.yyyy HH:mm");

    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto* cancelBtn = new QPushButton(tr("Отмена"), &dlg);
    auto* okBtn     = new QPushButton(tr("Отметить приём"), &dlg);
    okBtn->setObjectName("PrimaryButton");
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(okBtn);

    layout->addWidget(title);
    layout->addWidget(doseLbl);
    layout->addWidget(new QLabel(tr("Дата и время приёма:"), &dlg));
    layout->addWidget(dateTimeEdit);
    layout->addStretch();
    layout->addLayout(btnRow);

    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(okBtn,     &QPushButton::clicked, &dlg, &QDialog::accept);

    outerLayout->addWidget(frame);

    showBlurredDialog(&dlg);

    if (dlg.result() != QDialog::Accepted)
        return;

    const QDateTime dt = dateTimeEdit->dateTime();
    const QString medId = med.value("id").toString();
    if (medId.isEmpty())
        return;

    // создаём лог PRN через AppCore
    m_core->markMedicationIntakePrn(medId, dt, "taken");

    // уменьшаем остаток на разовую дозу
    const double doseTabsLocal = med.value("dose_tablets").toDouble(1.0);
    updateMedicationStockOnIntake(medId, -doseTabsLocal);
}


// --- refreshTodayIntakesList ---
void MainWindow::refreshTodayIntakesList()
{
    if (!m_todayIntakesLayout || !m_intakesDateEdit)
        return;

    ::clearLayout(m_todayIntakesLayout);

    const QDate date = m_intakesDateEdit->date();

    const QJsonArray meds      = m_core->medicationsJson();
    const QJsonArray schedules = m_cachedMedicationItems;
    const QJsonArray logs      = m_cachedMedicationLogs;

    // индексируем логи по schedule_id+date
    QHash<QString, QJsonObject> logBySchedule;
    QSet<QString> takenScheduleIds;

    for (const auto& v : logs) {
        const QJsonObject o = v.toObject();
        const QString schedId = o.value("schedule_id").toString();
        const QString status  = o.value("status").toString().trimmed().toLower();
        const QString takenAt = o.value("taken_at").toString();

        QDateTime dt = QDateTime::fromString(takenAt, Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(takenAt, Qt::ISODateWithMs);
        if (!dt.isValid() || dt.date() != date)
            continue;

        if (!schedId.isEmpty()) {
            logBySchedule.insert(schedId, o);
            if (status == "taken")
                takenScheduleIds.insert(schedId);
        }
    }

    int count = 0;

    for (const auto& sv : schedules) {
        const QJsonObject s = sv.toObject();
        const QString medId = s.value("medication_id").toString();
        if (medId.isEmpty())
            continue;

        // ищем препарат
        QJsonObject med;
        for (const auto& mv : meds) {
            const QJsonObject mo = mv.toObject();
            if (mo.value("id").toString() == medId) {
                med = mo;
                break;
            }
        }
        if (med.isEmpty())
            continue;

        // фильтр по диапазону дат графика (из локальной мапы)
        if (m_medicationScheduleDates.contains(medId)) {
            const auto& dates = m_medicationScheduleDates[medId];
            if (dates.first.isValid() && date < dates.first)
                continue;
            if (dates.second.isValid() && date > dates.second)
                continue;
        }

        const QString schedId = s.value("id").toString();
        const bool isTaken = takenScheduleIds.contains(schedId);
        const QJsonObject existingLog = logBySchedule.value(schedId);

        createIntakeItemWidget(date, med, s, existingLog, isTaken);
        ++count;
    }

    if (count == 0) {
        auto* empty = new QLabel(
            tr("На этот день нет запланированных приёмов."),
            m_todayIntakesHost);
        empty->setObjectName("MutedLabel");
        empty->setWordWrap(true);
        m_todayIntakesLayout->addWidget(empty);
    }

    m_todayIntakesLayout->addStretch();
}


// --- createIntakeItemWidget ---
void MainWindow::createIntakeItemWidget(const QDate& date,
                                        const QJsonObject& med,
                                        const QJsonObject& schedule,
                                        const QJsonObject& existingLog,
                                        bool isTaken)
{
    Q_UNUSED(date);

    auto* row = new QFrame(m_dayMedicationHost);
    row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    row->setContextMenuPolicy(Qt::CustomContextMenu);

    const QString logStatus =
        existingLog.value("status").toString().trimmed().toLower();

    QString cardObjectName = "FeedCard";
    if (logStatus == "taken")
        cardObjectName = "FeedCardTaken";
    else if (logStatus == "missed")
        cardObjectName = "FeedCardMissed";

    row->setObjectName(cardObjectName);

    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(8);

    const QString schedId   = schedule.value("id").toString();
    const QString medId     = med.value("id").toString();
    QString timeStr = schedule.value("intake_time").toString();
    // нормализуем: убираем секунды если есть
    QTime tNorm = QTime::fromString(timeStr, "HH:mm:ss");
    if (!tNorm.isValid()) tNorm = QTime::fromString(timeStr, "HH:mm");
    if (tNorm.isValid()) timeStr = tNorm.toString("HH:mm");

    const QString labelStr  = schedule.value("label").toString();
    const QString brandName = med.value("brand_name").toString();
    const QString inn       = med.value("inn").toString();
    const QString doseMgStr = med.value("dose_mg").toVariant().toString();
    const double  doseTabs  = med.value("dose_tablets").toDouble(1.0);

    const QString mainName = !inn.isEmpty() ? inn : brandName;

    QString acceptedInfo;
    if (logStatus == "taken") {
        const QString takenAtIso = existingLog.value("taken_at").toString();
        QDateTime dt = QDateTime::fromString(takenAtIso, Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(takenAtIso, Qt::ISODateWithMs);
        if (dt.isValid()) {
            acceptedInfo = tr(" (принято в %1)")
                               .arg(dt.toLocalTime().time().toString("HH:mm"));
        }
    }

    QString timeLabelText;
    if (!labelStr.isEmpty())
        timeLabelText = labelStr + " · ";
    timeLabelText += (timeStr.isEmpty() ? tr("Время не указано") : timeStr);
    timeLabelText += acceptedInfo;

    auto* timeLbl = new QLabel(timeLabelText, row);
    timeLbl->setObjectName("CardTitle");

    auto* nameLbl = new QLabel(
        mainName.isEmpty() ? tr("Без названия") : mainName,
        row);
    nameLbl->setObjectName("CardSubtitle");

    QString detail;
    if (!doseMgStr.isEmpty()) {
        detail = tr("%1 мг · %2 табл.")
                     .arg(doseMgStr)
                     .arg(QString::number(doseTabs, 'f', 2));
    }

    auto* detailsLbl = new QLabel(detail, row);
    detailsLbl->setObjectName("MutedLabel");

    auto* col = new QVBoxLayout;
    col->setSpacing(2);
    col->addWidget(timeLbl);
    col->addWidget(nameLbl);
    col->addWidget(detailsLbl);

    layout->addLayout(col);
    layout->addStretch();

    m_scheduleTitlesById[schedId] = mainName + " · " + timeStr;

    // === ВАЖНО: свойства для логики тоггла ===
    row->setProperty("logId",      existingLog.value("id").toString());
    row->setProperty("scheduleId", schedId);
    row->setProperty("medId",      medId);
    row->setProperty("status",     logStatus.isEmpty() ? "planned" : logStatus);
    row->setProperty("isPrn",      schedId.isEmpty()); // PRN, если без schedule

    connect(row,
            &QWidget::customContextMenuRequested,
            this,
            &MainWindow::showDayIntakeContextMenu);

    m_dayMedicationLayout->addWidget(row);
}


// --- handleIntakeStatusChange ---
void MainWindow::handleIntakeStatusChange(const QString& clicked)
{
    if (m_contextMenuMedId.isEmpty() || m_contextMenuScheduleId.isEmpty())
        return;

    QString newStatus;

    if (clicked == "taken") {
        if (m_contextMenuCurrentStatus == "taken")
            newStatus = "none";
        else
            newStatus = "taken";
    } else if (clicked == "missed") {
        if (m_contextMenuCurrentStatus == "missed")
            newStatus = "none";
        else
            newStatus = "missed";
    }

    const QDate date = m_medCalendar ? m_medCalendar->selectedDate()
                                     : QDate::currentDate();

    m_core->setMedicationIntakeStatus(
        m_contextMenuMedId,
        m_contextMenuScheduleId,
        date,
        newStatus);

    if (m_contextMenuCurrentStatus != "taken" && newStatus == "taken") {
        const QJsonArray meds = m_core->medicationsJson();
        for (const auto& v : meds) {
            const QJsonObject med = v.toObject();
            if (med.value("id").toString() == m_contextMenuMedId) {
                const double doseTabs = med.value("dose_tablets").toDouble(1.0);
                updateMedicationStockOnIntake(m_contextMenuMedId, -doseTabs);
                break;
            }
        }
    }
}


// --- startMedicationReminderLoop ---
void MainWindow::startMedicationReminderLoop()
{
    if (!m_medReminderTimer) {
        m_medReminderTimer = new QTimer(this);
        connect(m_medReminderTimer, &QTimer::timeout,
                this, &MainWindow::checkMedicationReminders);
        m_medReminderTimer->start(30'000); // каждые 30 сек чтобы не пропустить минуту
    }
}


// --- checkMedicationReminders ---
void MainWindow::checkMedicationReminders()
{
    if (!m_trayIcon)
        return;

    const QDate today = QDate::currentDate();
    const QTime now   = QTime::currentTime();

    const QJsonArray meds      = m_core->medicationsJson();
    const QJsonArray schedules = m_cachedMedicationItems;

    // раз в 5 минут печатаем все времена
    static QTime lastDebugPrint;
    if (lastDebugPrint.isNull() || lastDebugPrint.secsTo(now) >= 300) {
        lastDebugPrint = now;
        qDebug() << "📋 ALL schedule times:";
        for (const auto& sv : schedules) {
            const QJsonObject ss = sv.toObject();
            qDebug() << "  time:" << ss.value("intake_time").toString()
                     << "med_id:" << ss.value("medication_id").toString();
        }
    }

    qDebug() << "checkMedicationReminders:" << now.toString("HH:mm")
             << "schedules:" << schedules.size() << "meds:" << meds.size();

    for (const auto& sv : schedules) {
        const QJsonObject s = sv.toObject();

        const QString timeStr = s.value("intake_time").toString().trimmed();
        QTime t = QTime::fromString(timeStr, "HH:mm:ss");  // Supabase возвращает с секундами
        if (!t.isValid())
            t = QTime::fromString(timeStr, "HH:mm");       // фолбэк на старый формат
        if (!t.isValid())
            continue;

        // сравниваем только часы+минуты
        if (now.hour() == t.hour() && now.minute() == t.minute()) {
            qDebug() << "⏰ MATCH:" << timeStr << "now=" << now.toString("HH:mm");
            const QString medId = s.value("medication_id").toString();

            QString medName;
            for (const auto& mv : meds) {
                const QJsonObject med = mv.toObject();
                if (med.value("id").toString() == medId) {
                    medName = med.value("brand_name").toString();
                    if (medName.isEmpty())
                        medName = med.value("inn").toString();
                    break;
                }
            }
            qDebug() << "⏰ medId=" << medId << "medName=" << medName;

            if (!medName.isEmpty()) {
                qDebug() << "🔔 NOTIFY:" << medName << timeStr;
                m_trayIcon->showMessage(
                    tr("Пора принять лекарство"),
                    tr("%1 — %2").arg(medName, timeStr),
                    QSystemTrayIcon::Information,
                    10000);
            }
        }
    }
}




// --- removeLogFromCache ---
void MainWindow::removeLogFromCache(const QString& logId)
{
    if (logId.isEmpty()) return;
    QJsonArray filtered;
    for (const auto& v : std::as_const(m_cachedMedicationLogs)) {
        if (v.toObject().value("id").toString() != logId)
            filtered.append(v);
    }
    m_cachedMedicationLogs = filtered;
}

// --- showDayIntakeContextMenu ---
void MainWindow::showDayIntakeContextMenu(const QPoint& pos)
{
    auto* row = qobject_cast<QWidget*>(sender());
    if (!row)
        return;

    const QString schedId = row->property("scheduleId").toString();
    const QString medId   = row->property("medId").toString();
    const QString logId   = row->property("logId").toString();
    const QString status  = row->property("status").toString(); // planned / taken / missed
    const bool isPrn      = row->property("isPrn").toBool();

    if (medId.isEmpty())
        return;

    const bool alreadyMarked = (status == "taken" || status == "missed");

    QMenu menu; // без parent, чтобы не крашнуться при удалении row во время показа
    QAction* markTakenAct   = nullptr;
    QAction* markMissedAct  = nullptr;
    QAction* cancelMarkAct  = nullptr;
    QAction* deleteAct      = nullptr;

    if (alreadyMarked) {
        // уже отмечен → только «Отменить отметку»
        cancelMarkAct = menu.addAction(tr("Отменить отметку"));
    } else {
        // ещё не отмечен → «Принято» / «Пропущено»
        markTakenAct  = menu.addAction(tr("Отметить принятым"));
        markMissedAct = menu.addAction(tr("Отметить пропущенным"));
    }

    // редактировать время (всегда, если есть лог)
    QAction* editTimeAct = nullptr;
    if (!logId.isEmpty()) {
        menu.addSeparator();
        editTimeAct = menu.addAction(tr("Редактировать время"));
    }

    // для PRN всегда доступно удаление
    if (isPrn) {
        menu.addSeparator();
        deleteAct = menu.addAction(tr("Удалить приём"));
    }

    QAction* chosen = menu.exec(row->mapToGlobal(pos));
    if (!chosen)
        return;

    // --- Редактировать время ---
    if (chosen == editTimeAct && !logId.isEmpty()) {
        editIntakeTime(logId);
        return;
    }

    // --- Удаление PRN-приёма ---
    if (chosen == deleteAct && isPrn) {
        qDebug() << "PRN delete: logId=" << logId << "isPrn=" << isPrn;
        if (!logId.isEmpty()) {
            m_core->deleteMedicationLog(logId);
            // мгновенное обновление: удаляем из локального кэша
            removeLogFromCache(logId);
            refreshDayMedicationSummary();
        }
        return;
    }

    // --- Отменить отметку (вернуть в planned) ---
    if (chosen == cancelMarkAct && !logId.isEmpty()) {
        qDebug() << "Cancel mark: logId=" << logId << "status=" << status;
        // если отменяем принятый — возвращаем таблетки
        if (status == "taken") {
            const QJsonArray meds = m_core->medicationsJson();
            for (const auto& v : meds) {
                const QJsonObject m = v.toObject();
                if (m.value("id").toString() == medId) {
                    const double doseTabs = m.value("dose_tablets").toDouble(1.0);
                    updateMedicationStockOnIntake(medId, +doseTabs);
                    break;
                }
            }
        }
        m_core->deleteMedicationLog(logId);
        removeLogFromCache(logId);
        refreshDayMedicationSummary();
        return;
    }

    // --- Новая отметка (дата из календаря + текущее время) ---
    const QDate selectedDate = m_medCalendar ? m_medCalendar->selectedDate()
                                             : QDate::currentDate();
    auto createLog = [this, medId, schedId, selectedDate](const QString& newStatus) {
        QJsonObject log;
        log["user_id"]       = m_core->currentUserId();
        log["medication_id"] = medId;
        if (!schedId.isEmpty())
            log["schedule_id"] = schedId;
        else
            log["schedule_id"] = QJsonValue(); // PRN

        const QDateTime dt(selectedDate, QTime::currentTime());
        log["taken_at"] = dt.toUTC().toString(Qt::ISODate);
        log["status"]   = newStatus;

        m_core->createMedicationLog(log);
    };

    if (chosen == markTakenAct) {
        createLog("taken");
        // вычитаем таблетки
        const QJsonArray meds = m_core->medicationsJson();
        for (const auto& v : meds) {
            const QJsonObject m = v.toObject();
            if (m.value("id").toString() == medId) {
                const double doseTabs = m.value("dose_tablets").toDouble(1.0);
                updateMedicationStockOnIntake(medId, -doseTabs);
                break;
            }
        }
        // перерисовка произойдёт когда fetchMedicationLogs вернёт ответ
    } else if (chosen == markMissedAct) {
        createLog("missed");
    }
}


// --- editIntakeTime (новый, с параметром logId) ---
void MainWindow::editIntakeTime(const QString& logId)
{
    if (logId.isEmpty()) return;

    // 1. Находим лог по id
    QJsonObject log;
    for (const auto& v : std::as_const(m_cachedMedicationLogs)) {
        const QJsonObject o = v.toObject();
        if (o.value("id").toString() == logId) {
            log = o;
            break;
        }
    }
    if (log.isEmpty()) return;

    // 2. Текущее время taken_at или сейчас
    QDateTime dt = QDateTime::fromString(log.value("taken_at").toString(), Qt::ISODate);
    if (!dt.isValid())
        dt = QDateTime::currentDateTime();

    // 3. Диалог с блюром
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Редактировать приём"));
    dlg.setModal(true);
    dlg.setWindowFlag(Qt::FramelessWindowHint, true);
    dlg.setAttribute(Qt::WA_TranslucentBackground, true);

    auto* outerLayout = new QVBoxLayout(&dlg);
    outerLayout->setContentsMargins(16, 16, 16, 16);
    outerLayout->setSpacing(12);

    auto* frame = new QFrame(&dlg);
    frame->setObjectName("SectionCard");
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    const QString medId = log.value("medication_id").toString();
    QString medName;
    const QJsonArray meds = m_core->medicationsJson();
    for (const auto& v : meds) {
        const QJsonObject m = v.toObject();
        if (m.value("id").toString() == medId) {
            medName = m.value("brand_name").toString();
            if (medName.isEmpty()) medName = m.value("inn").toString();
            break;
        }
    }

    auto* titleLbl = new QLabel(tr("Препарат: %1").arg(medName.isEmpty() ? tr("Без названия") : medName), frame);
    titleLbl->setObjectName("SectionTitle");

    auto* dateLbl = new QLabel(tr("Дата приёма:"), frame);
    auto* dateEdit = new QDateEdit(dt.date(), frame);
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("dd.MM.yyyy");

    auto* timeLbl = new QLabel(tr("Время приёма:"), frame);
    auto* timeEdit = new QTimeEdit(dt.time(), frame);
    timeEdit->setDisplayFormat("HH:mm");

    auto* statusLbl = new QLabel(tr("Статус:"), frame);
    auto* statusCombo = new QComboBox(frame);
    statusCombo->addItem(tr("Принято"), "taken");
    statusCombo->addItem(tr("Пропущено"), "missed");
    const QString curStatus = log.value("status").toString().trimmed().toLower();
    statusCombo->setCurrentIndex(curStatus == "missed" ? 1 : 0);

    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto* cancelBtn = new QPushButton(tr("Отмена"), frame);
    auto* okBtn = new QPushButton(tr("Сохранить"), frame);
    okBtn->setObjectName("PrimaryButton");
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(okBtn);

    layout->addWidget(titleLbl);
    layout->addWidget(dateLbl);
    layout->addWidget(dateEdit);
    layout->addWidget(timeLbl);
    layout->addWidget(timeEdit);
    layout->addWidget(statusLbl);
    layout->addWidget(statusCombo);
    layout->addStretch();
    layout->addLayout(btnRow);

    outerLayout->addWidget(frame);

    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    showBlurredDialog(&dlg);

    if (dlg.result() != QDialog::Accepted) return;

    // 4. Обновляем лог
    QDateTime newDt(dateEdit->date(), timeEdit->time());
    m_core->updateMedicationLogTime(logId, newDt);

    // если изменился статус, нужно обновить и его
    const QString newStatus = statusCombo->currentData().toString();
    if (newStatus != curStatus) {
        // для смены статуса проще удалить и создать новый лог
        QJsonObject newLog;
        newLog["user_id"] = m_core->currentUserId();
        newLog["medication_id"] = medId;
        newLog["schedule_id"] = log.value("schedule_id");
        newLog["taken_at"] = newDt.toUTC().toString(Qt::ISODate);
        newLog["status"] = newStatus;
        m_core->deleteMedicationLog(logId);
        m_core->createMedicationLog(newLog);
    }
}

// --- handleEditIntakeTime (старый, через m_contextMenuLogId) ---
void MainWindow::handleEditIntakeTime()
{
    if (m_contextMenuLogId.isEmpty())
        return;

    // 1. Находим лог по id
    QJsonObject log;
    for (const auto& v : m_cachedMedicationLogs) {
        const QJsonObject o = v.toObject();
        if (o.value("id").toString() == m_contextMenuLogId) {
            log = o;
            break;
        }
    }
    if (log.isEmpty())
        return;

    // 2. Текущее время taken_at или сейчас
    QDateTime dt = QDateTime::fromString(log.value("taken_at").toString(), Qt::ISODate);
    if (!dt.isValid())
        dt = QDateTime::currentDateTime();

    // 3. Диалог выбора времени
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Изменить время приёма"));
    dlg.setModal(true);
    dlg.setWindowFlag(Qt::FramelessWindowHint, true);
    dlg.setAttribute(Qt::WA_TranslucentBackground, true);

    auto* outerLayout = new QVBoxLayout(&dlg);
    outerLayout->setContentsMargins(16, 16, 16, 16);
    outerLayout->setSpacing(12);

    auto* frame = new QFrame(&dlg);
    frame->setObjectName("SectionCard");
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* label = new QLabel(tr("Фактическое время приёма:"), &dlg);
    auto* timeEdit = new QTimeEdit(dt.time(), &dlg);
    timeEdit->setDisplayFormat("HH:mm");

    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto* cancelBtn = new QPushButton(tr("Отмена"), &dlg);
    auto* okBtn = new QPushButton(tr("Сохранить"), &dlg);
    okBtn->setObjectName("PrimaryButton");
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(okBtn);

    layout->addWidget(label);
    layout->addWidget(timeEdit);
    layout->addStretch();
    layout->addLayout(btnRow);

    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    outerLayout->addWidget(frame);

    showBlurredDialog(&dlg);

    if (dlg.result() != QDialog::Accepted)
        return;

    // 4. Собираем новое время: тот же день, другое время
    dt.setTime(timeEdit->time());

    // 5. Просим AppCore обновить taken_at
    m_core->updateMedicationLogTime(m_contextMenuLogId, dt);
}


// --- handleDeletePrnIntake ---
void MainWindow::handleDeletePrnIntake()
{
    if (m_contextMenuLogId.isEmpty())
        return;

    m_core->deleteMedicationLog(m_contextMenuLogId);
}


// --- refreshDayMedicationSummary ---
void MainWindow::refreshDayMedicationSummary()
{
    if (!m_dayMedicationLayout || !m_medCalendar)
        return;

    ::clearLayout(m_dayMedicationLayout);

    const QDate date = m_medCalendar->selectedDate();

    const QJsonArray meds      = m_core->medicationsJson();
    const QJsonArray schedules = m_cachedMedicationItems;
    const QJsonArray logs      = m_cachedMedicationLogs;

    QHash<QString, QJsonObject> logBySchedule;

    for (const auto& v : logs) {
        const QJsonObject o = v.toObject();
        const QString schedId = o.value("schedule_id").toString();
        const QString takenAt = o.value("taken_at").toString();

        QDateTime dt = QDateTime::fromString(takenAt, Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(takenAt, Qt::ISODateWithMs);
        if (!dt.isValid() || dt.date() != date)
            continue;

        if (!schedId.isEmpty())
            logBySchedule.insert(schedId, o);
    }

    struct SchedItem {
        QJsonObject sched;
        QJsonObject med;
        QJsonObject log;
        QTime time;
    };
    QList<SchedItem> items;

    for (const auto& sv : schedules) {
        const QJsonObject s = sv.toObject();
        const QString medId = s.value("medication_id").toString();
        if (medId.isEmpty())
            continue;

        QJsonObject med;
        for (const auto& mv : meds) {
            const QJsonObject mo = mv.toObject();
            if (mo.value("id").toString() == medId) {
                med = mo;
                break;
            }
        }
        if (med.isEmpty())
            continue;

        // фильтр по диапазону дат графика (из локальной мапы)
        if (m_medicationScheduleDates.contains(medId)) {
            const auto& dates = m_medicationScheduleDates[medId];
            if (dates.first.isValid() && date < dates.first)
                continue;
            if (dates.second.isValid() && date > dates.second)
                continue;
        }

        const QString schedId = s.value("id").toString();
        const QJsonObject log = logBySchedule.value(schedId);

        const QString timeStr = s.value("intake_time").toString();
        QTime t = QTime::fromString(timeStr, "HH:mm:ss");
        if (!t.isValid()) t = QTime::fromString(timeStr, "HH:mm");
        if (!t.isValid())
            t = QTime(0, 0);

        SchedItem item;
        item.sched = s;
        item.med   = med;
        item.log   = log;
        item.time  = t;

        items.append(item);
    }

    std::sort(items.begin(), items.end(),
              [](const SchedItem& a, const SchedItem& b) {
                  return a.time < b.time;
              });

    int count = 0;

    for (const auto& it : items) {
        const QString status =
            it.log.value("status").toString().trimmed().toLower();
        const bool isTaken = (status == "taken");
        createIntakeItemWidget(date, it.med, it.sched, it.log, isTaken);
        ++count;
    }

    // PRN-логи (schedule_id NULL)
    for (const auto& v : logs) {
        const QJsonObject o = v.toObject();
        if (!o.value("schedule_id").isNull())
            continue;

        const QString medId = o.value("medication_id").toString();
        if (medId.isEmpty())
            continue;

        QDateTime dt = QDateTime::fromString(o.value("taken_at").toString(), Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(o.value("taken_at").toString(), Qt::ISODateWithMs);
        if (!dt.isValid() || dt.date() != date)
            continue;

        QJsonObject med;
        for (const auto& mv : meds) {
            const QJsonObject mo = mv.toObject();
            if (mo.value("id").toString() == medId) {
                med = mo;
                break;
            }
        }
        if (med.isEmpty())
            continue;

        QJsonObject sched;
        sched["id"]           = QString();
        sched["medication_id"]= medId;
        sched["intake_time"]  = dt.time().toString("HH:mm");
        sched["label"]        = tr("Приём по необходимости");

        const QString status = o.value("status").toString().trimmed().toLower();
        const bool isTaken   = (status == "taken");

        createIntakeItemWidget(date, med, sched, o, isTaken);
        ++count;
    }

    if (count == 0) {
        auto* empty = new QLabel(
            tr("На этот день нет запланированных приёмов."),
            m_dayMedicationHost);
        empty->setObjectName("MutedLabel");
        empty->setWordWrap(true);
        m_dayMedicationLayout->addWidget(empty);
    }

    m_dayMedicationLayout->addStretch();
}


// --- updateMedicationStockOnIntake ---
void MainWindow::updateMedicationStockOnIntake(const QString& medId,
                                               double deltaTablets)
{
    if (medId.isEmpty())
        return;

    QJsonArray meds = m_core->medicationsJson();
    bool changed = false;

    for (int i = 0; i < meds.size(); ++i) {
        QJsonObject o = meds.at(i).toObject();
        if (o.value("id").toString() != medId)
            continue;

        double stockTabs = o.value("stock_tablets").toDouble(0.0);
        stockTabs += deltaTablets;
        if (stockTabs < 0.0)
            stockTabs = 0.0;
        stockTabs = std::round(stockTabs / 0.25) * 0.25;

        o["stock_tablets"] = stockTabs;
        meds[i] = o;

        // отправляем только то, что нужно
        QJsonObject payload;
        payload["stock_tablets"] = stockTabs;
        m_core->updateMedication(medId, payload);

        changed = true;
        break;
    }

    if (changed) {
        m_cachedMedications = meds;
        rebuildMedicationCards();
        refreshTodayIntakesList();
    }
}




// --- cancelMedicationIntake ---
void MainWindow::cancelMedicationIntake()
{
    if (m_contextMenuLogId.isEmpty())
        return;

    m_core->cancelMedicationLog(m_contextMenuLogId); // реализуешь в AppCore
    m_contextMenuLogId.clear();
}




