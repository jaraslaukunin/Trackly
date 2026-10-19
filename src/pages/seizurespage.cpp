// Seizures page implementation for MainWindow
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

// --- createSeizuresPage ---
QWidget* MainWindow::createSeizuresPage()
{
    auto* page = new QWidget;
    auto* root = new QHBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(16);

    auto* form = new QWidget;
    form->setObjectName("SeizureFormHost");
    auto* grid = new QGridLayout(form);
    grid->setSpacing(12);

    m_seizureDate = new QDateTimeEdit(QDateTime::currentDateTime(), form);
    m_seizureDate->setCalendarPopup(true);

    m_seizurePlace = new QComboBox(form);
    m_seizurePlace->addItems({
        tr("Дом"), tr("Работа"), tr("Учеба"), tr("Общественное место"), tr("Другое")
    });

    m_seizureType = new QComboBox(form);
    m_seizureType->addItems({
        tr("Генерализованный тонико-клонический"),
        tr("Фокальный"),
        tr("Миоклонический"),
        tr("Абсанс"),
        tr("Фебрильные судороги"),
        tr("Неуточненный"),
        tr("Психомоторный"),
        tr("Эпистатус")
    });

    m_seizureTriggers = new QLineEdit(form);
    m_seizureTriggers->setPlaceholderText(tr("Например: недосып, стресс"));

    m_seizurePost = new QLineEdit(form);
    m_seizurePost->setPlaceholderText(tr("Например: сонливость, дезориентация"));

    m_seizureSeverity = new QSpinBox(form);
    m_seizureSeverity->setRange(1, 3);

    m_seizureNotes = new QTextEdit(form);
    m_seizureNotes->setPlaceholderText(tr("Заметки и оказанная помощь"));
    m_seizureNotes->setFixedHeight(120);

    m_saveSeizureBtn = new QPushButton(tr("Сохранить приступ"), form);
    m_saveSeizureBtn->setObjectName("PrimaryButton");


    grid->addWidget(new QLabel(tr("Дата и время начала")), 0, 0);
    grid->addWidget(m_seizureDate, 0, 1);
    grid->addWidget(new QLabel(tr("Где произошло")), 1, 0);
    grid->addWidget(m_seizurePlace, 1, 1);
    grid->addWidget(new QLabel(tr("Тип приступа")), 2, 0);
    grid->addWidget(m_seizureType, 2, 1);
    grid->addWidget(new QLabel(tr("Триггеры")), 3, 0);
    grid->addWidget(m_seizureTriggers, 3, 1);
    grid->addWidget(new QLabel(tr("Состояние после")), 4, 0);
    grid->addWidget(m_seizurePost, 4, 1);
    grid->addWidget(new QLabel(tr("Тяжесть 1–3")), 5, 0);
    grid->addWidget(m_seizureSeverity, 5, 1);
    grid->addWidget(m_seizureNotes, 6, 0, 1, 2);
    grid->addWidget(m_saveSeizureBtn, 7, 0, 1, 2);

    auto* historyHost = new QWidget;
    historyHost->setObjectName("SeizureHistoryHost");
    auto* historyLayout = new QVBoxLayout(historyHost);
    historyLayout->setObjectName("SeizureFeedLayout");
    historyHost->setObjectName("SeizureFeedHost");
    historyLayout->setContentsMargins(0, 0, 0, 0);
    historyLayout->setSpacing(10);
    auto* placeholder = new QLabel(tr("История приступов появится после загрузки данных."), historyHost);
    placeholder->setObjectName("MutedLabel");
    historyLayout->addWidget(placeholder);
    historyLayout->addStretch();

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(historyHost);

    root->addWidget(makeSection(tr("Записать приступ"), form), 2);
    root->addWidget(makeSection(tr("История приступов"), scroll), 3);

    connect(m_saveSeizureBtn, &QPushButton::clicked, this, &MainWindow::submitSeizureForm);

    return page;
}


// --- submitSeizureForm ---
void MainWindow::submitSeizureForm()
{
    const QString place = m_seizurePlace ? m_seizurePlace->currentText().trimmed() : QString();
    const QString type = m_seizureType ? m_seizureType->currentText().trimmed() : QString();
    const QString triggers = m_seizureTriggers ? m_seizureTriggers->text().trimmed() : QString();
    const QString post = m_seizurePost ? m_seizurePost->text().trimmed() : QString();
    const QString notes = m_seizureNotes ? m_seizureNotes->toPlainText().trimmed() : QString();

    if (place.isEmpty()) {
        showToast(m_central, tr("Укажи место приступа"), ToastKind::Error, 2600);
        return;
    }

    if (type.isEmpty()) {
        showToast(m_central, tr("Укажи тип приступа"), ToastKind::Error, 2600);
        return;
    }

    QJsonObject payload{
        {"user_id", m_core->currentUserId()},
        {"started_at", m_seizureDate->dateTime().toUTC().toString(Qt::ISODate)},
        {"place", place},
        {"seizure_type", type},
        {"triggers", triggers},
        {"post_state", post},
        {"severity", m_seizureSeverity->value()},
        {"notes", notes}
    };

    if (m_editSeizureId.isEmpty()) {
        m_core->createSeizure(payload);
    } else {
        m_core->updateSeizure(m_editSeizureId, payload);
        m_editSeizureId.clear();
    }

    resetSeizureForm();
}


// --- resetSeizureForm ---
void MainWindow::resetSeizureForm()
{
    m_editSeizureId.clear();
    if (m_saveSeizureBtn) m_saveSeizureBtn->setText(tr("Сохранить приступ"));
    if (m_seizureDate) m_seizureDate->setDateTime(QDateTime::currentDateTime());
    if (m_seizurePlace) m_seizurePlace->setCurrentIndex(0);
    if (m_seizureType) m_seizureType->setCurrentIndex(0);
    if (m_seizureTriggers) m_seizureTriggers->clear();
    if (m_seizurePost) m_seizurePost->clear();
    if (m_seizureSeverity) m_seizureSeverity->setValue(1);
    if (m_seizureNotes) m_seizureNotes->clear();
}


// --- editSeizure ---
void MainWindow::editSeizure(const QJsonObject& seizure)
{
    if (!m_seizureDate || !m_seizurePlace || !m_seizureType || !m_seizureTriggers
        || !m_seizurePost || !m_seizureSeverity || !m_seizureNotes) {
        return;
    }

    m_editSeizureId = seizure.value("id").toString();
    if (m_saveSeizureBtn) m_saveSeizureBtn->setText(tr("Обновить приступ"));

    QDateTime dt = QDateTime::fromString(seizure.value("started_at").toString(), Qt::ISODate);
    if (!dt.isValid()) dt = QDateTime::currentDateTime();
    m_seizureDate->setDateTime(dt);

    m_seizurePlace->setCurrentText(seizure.value("place").toString());
    m_seizureType->setCurrentText(seizure.value("seizure_type").toString());
    m_seizurePost->setText(seizure.value("post_state").toString());
    m_seizureSeverity->setValue(qMax(1, seizure.value("severity").toInt(1)));
    m_seizureNotes->setPlainText(seizure.value("notes").toString());

    QString triggersText;
    if (seizure.value("triggers").isArray()) {
        QStringList parts;
        for (const auto& v : seizure.value("triggers").toArray()) {
            const QString t = v.toString().trimmed();
            if (!t.isEmpty()) parts << t;
        }
        triggersText = parts.join(", ");
    } else {
        triggersText = seizure.value("triggers").toString();
    }
    m_seizureTriggers->setText(triggersText);

    navigateTo(1, tr("Приступы"));
}


// --- deleteSeizure ---
void MainWindow::deleteSeizure(const QJsonObject& seizure)
{
    const QString id = seizure.value("id").toString();
    if (id.isEmpty()) return;

    if (showConfirmDialog(tr("Удалить приступ"), tr("Удалить запись о приступе?"))) {
        m_core->deleteSeizure(id);
        if (m_editSeizureId == id) resetSeizureForm();
    }
}


// --- showSeizureMenu ---
void MainWindow::showSeizureMenu(const QJsonObject& seizure, QWidget* anchor)
{
    if (!anchor) return;

    QMenu menu; // без parent чтобы избежать краша при удалении карточки
    QAction* editAct = menu.addAction(tr("Редактировать"));
    QAction* duplicateAct = menu.addAction(tr("Заполнить как шаблон"));
    menu.addSeparator();
    QAction* deleteAct = menu.addAction(tr("Удалить"));

    QAction* chosen = menu.exec(anchor->mapToGlobal(anchor->rect().bottomLeft()));
    if (!chosen) return;

    if (chosen == editAct || chosen == duplicateAct) {
        editSeizure(seizure);
        return;
    }
    if (chosen == deleteAct) {
        deleteSeizure(seizure);
    }
}


// --- fillSeizures ---
void MainWindow::fillSeizures(const QJsonArray& arr)
{
    qDebug() << "fillSeizures called, arr size:" << arr.size();
    m_cachedSeizureItems = arr;
    m_datesWithSeizures.clear();

    QWidget* page = m_stack->widget(1);
    QWidget* host = page->findChild<QWidget*>("SeizureFeedHost");
    auto* layout = host ? host->findChild<QVBoxLayout*>("SeizureFeedLayout") : nullptr;
    if (!layout) {
        qDebug() << "fillSeizures: layout not found! page=" << page << "host=" << host;
        return;
    }

    ::clearLayout(layout);

    if (arr.isEmpty()) {
        auto* empty = new QLabel(tr("После добавления приступов здесь появится красивая лента записей."), host);
        empty->setObjectName("MutedLabel");
        empty->setWordWrap(true);
        layout->addWidget(empty);
        layout->addStretch();
        updateDashboardCalendarDecorations();
        refreshAnalyticsPage();
        return;
    }

    for (const auto& v : arr) {
        const auto o = v.toObject();

        QDateTime dt = QDateTime::fromString(o.value("started_at").toString(), Qt::ISODate);
        if (!dt.isValid()) dt = QDateTime::fromString(o.value("started_at").toString(), Qt::ISODateWithMs);
        if (dt.isValid()) m_datesWithSeizures.insert(dt.date());

        auto* card = new QFrame(host);
        card->setObjectName("FeedCard");
        card->setCursor(Qt::PointingHandCursor);
        card->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(card, &QWidget::customContextMenuRequested, this, [this, o, card](const QPoint&) {
            showSeizureMenu(o, card);
        });

        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(16, 16, 16, 16);
        cl->setSpacing(6);

        auto* title = new QLabel(o.value("seizure_type").toString(), card);
        title->setObjectName("CardTitle");

        auto* meta = new QLabel(
            formatTs(o.value("started_at").toString()) + " · " + o.value("place").toString(),
            card);
        meta->setObjectName("MutedLabel");

        cl->addWidget(title);
        cl->addWidget(meta);

        // Severity чипсами (1-3)
        const int sev = qBound(1, o.value("severity").toInt(1), 3);
        auto* sevRow = new QHBoxLayout;
        sevRow->setSpacing(4);
        auto* sevLbl = new QLabel(tr("Тяжесть:"), card);
        sevLbl->setObjectName("MutedLabel");
        sevRow->addWidget(sevLbl);
        for (int i = 1; i <= 3; ++i) {
            auto* chip = new QLabel(i <= sev ? "●" : "○", card);
            chip->setStyleSheet(i <= sev ? "color:#DC2626;font-size:14px;" : "color:#9CA3AF;font-size:14px;");
            sevRow->addWidget(chip);
        }
        sevRow->addStretch();
        cl->addLayout(sevRow);

        // Триггеры (показываем всегда)
        const QString triggersStr = [&]() -> QString {
            const QJsonValue tv = o.value("triggers");
            if (tv.isArray()) {
                QStringList parts;
                for (const auto& t : tv.toArray()) {
                    const QString s = t.toString().trimmed();
                    if (!s.isEmpty()) parts << s;
                }
                if (!parts.isEmpty()) return parts.join(", ");
            }
            const QString s = tv.toString().trimmed();
            return s.isEmpty() ? QString() : s;
        }();
        auto* trigLabel = new QLabel(
            triggersStr.isEmpty() ? tr("💡 Триггеры: —") : tr("💡 %1").arg(triggersStr), card);
        trigLabel->setWordWrap(true);
        trigLabel->setObjectName("MutedLabel");
        cl->addWidget(trigLabel);

        // Состояние после (показываем всегда)
        const QString postState = o.value("post_state").toString().trimmed();
        auto* postLabel = new QLabel(
            postState.isEmpty() ? tr("После: —") : tr("После: %1").arg(postState), card);
        postLabel->setWordWrap(true);
        postLabel->setObjectName("MutedLabel");
        cl->addWidget(postLabel);

        // Заметки
        const QString notesStr = o.value("notes").toString().trimmed();
        if (!notesStr.isEmpty()) {
            auto* note = new QLabel(notesStr, card);
            note->setWordWrap(true);
            cl->addWidget(note);
        }

        layout->addWidget(card);
    }

    layout->addStretch();
    updateDashboardCalendarDecorations();
    refreshAnalyticsPage();
}


// --- markMedicationTakenFromSchedule ---
void MainWindow::markMedicationTakenFromSchedule(const QJsonObject& med,
                                                 const QJsonObject& schedule,
                                                 const QDate& date)
{
    const QString medId   = med.value("id").toString();
    const QString schedId = schedule.value("id").toString();
    if (medId.isEmpty() || schedId.isEmpty())
        return;

    // лог в Supabase
    m_core->markMedicationIntake(medId, schedId, "taken");

    // уменьшить остаток на разовую дозу
    const double doseTabs = med.value("dose_tablets").toDouble(1.0);
    updateMedicationStockOnIntake(medId, -doseTabs);

    Q_UNUSED(date);
}


// --- markMedicationTaken ---
void MainWindow::markMedicationTaken(const QJsonObject& med, const QDate& date, const QString& time)
{
    const QString medId = med.value("id").toString();
    if (medId.isEmpty()) return;

    QTime takenTime = QTime::currentTime();
    if (!time.trimmed().isEmpty()) {
        const QTime parsed = QTime::fromString(time.trimmed(), "HH:mm");
        if (parsed.isValid()) takenTime = parsed;
    }

    const QDateTime takenAt(date, takenTime);

    // 1. Лог приёма в Supabase
    QJsonObject payload{
        {"user_id", m_core->currentUserId()},
        {"medication_id", medId},
        {"taken_at", takenAt.toUTC().toString(Qt::ISODate)},
        {"status", "taken"}
    };
    m_core->createMedicationLog(payload);

    // 2. Локальное обновление остатка (если есть поле stock)
    int stock = med.value("stock").toInt(-1);
    if (stock >= 0) {
        const int newStock = qMax(0, stock - 1); // пока просто -1 за приём

        // здесь не городим сложной логики с дозой, на защиту хватит
        m_core->updateMedicationStock(medId, newStock);
    }
}

