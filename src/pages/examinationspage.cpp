// Examinations page implementation for MainWindow
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

// --- createExamsPage ---
QWidget* MainWindow::createExamsPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(16);

    // Форма добавления
    auto* addForm = new QWidget(page);
    addForm->setObjectName("ExamFormHost");
    auto* grid = new QGridLayout(addForm);
    grid->setSpacing(12);

    m_examType = new QComboBox(addForm);
    m_examType->addItems({
        tr("МРТ"),
        tr("КТ"),
        tr("ЭЭГ"),
        tr("ЭКГ"),
        tr("Рентген"),
        tr("СМАД"),
        tr("Холтер ЭЭГ"),
        tr("Холтер ЭКГ"),
        tr("Приём врача")
    });

    m_examSubtype = new QComboBox(addForm);
    m_examDate = new QDateTimeEdit(QDateTime::currentDateTime(), addForm);
    m_examDate->setCalendarPopup(true);
    m_examNotes = new QTextEdit(addForm);
    m_examNotes->setFixedHeight(100);
    m_examNotes->setPlaceholderText(tr("Краткая сводка, результаты или комментарии"));

    auto* upload = new QPushButton(tr("Выбрать файл"), addForm);
    upload->setObjectName("SecondaryButton");

    m_examDropArea = new ExamAttachmentDropArea(addForm);

    m_saveExamBtn = new QPushButton(tr("Сохранить обследование"), addForm);
    m_saveExamBtn->setObjectName("PrimaryButton");

    grid->addWidget(new QLabel(tr("Тип исследования")), 0, 0);
    grid->addWidget(m_examType, 0, 1);

    grid->addWidget(new QLabel(tr("Подтип")), 1, 0);
    grid->addWidget(m_examSubtype, 1, 1);

    grid->addWidget(new QLabel(tr("Дата и время")), 2, 0);
    grid->addWidget(m_examDate, 2, 1);

    grid->addWidget(m_examDropArea, 3, 0, 1, 2);
    grid->addWidget(upload,        4, 0, 1, 2);
    grid->addWidget(m_examNotes,   5, 0, 1, 2);
    grid->addWidget(m_saveExamBtn, 6, 0, 1, 2);

    // Прошедшие
    m_pastExamHost = new QWidget(page);
    m_pastExamHost->setObjectName("PastExamHost");
    m_pastExamLayout = new QVBoxLayout(m_pastExamHost);
    m_pastExamLayout->setContentsMargins(0, 0, 0, 0);
    m_pastExamLayout->setSpacing(10);
    auto* pastEmpty = new QLabel(tr("Пока нет прошедших обследований."), m_pastExamHost);
    pastEmpty->setObjectName("MutedLabel");
    m_pastExamLayout->addWidget(pastEmpty);
    m_pastExamLayout->addStretch();

    // Запланированные
    m_plannedExamHost = new QWidget(page);
    m_plannedExamHost->setObjectName("PlannedExamHost");
    m_plannedExamLayout = new QVBoxLayout(m_plannedExamHost);
    m_plannedExamLayout->setContentsMargins(0, 0, 0, 0);
    m_plannedExamLayout->setSpacing(10);
    auto* plannedEmpty = new QLabel(tr("Пока нет запланированных обследований."), m_plannedExamHost);
    plannedEmpty->setObjectName("MutedLabel");
    m_plannedExamLayout->addWidget(plannedEmpty);
    m_plannedExamLayout->addStretch();

    auto* pastScroll = new QScrollArea(page);
    pastScroll->setWidgetResizable(true);
    pastScroll->setFrameShape(QFrame::NoFrame);
    pastScroll->setWidget(m_pastExamHost);

    auto* plannedScroll = new QScrollArea(page);
    plannedScroll->setWidgetResizable(true);
    plannedScroll->setFrameShape(QFrame::NoFrame);
    plannedScroll->setWidget(m_plannedExamHost);

    auto* columns = new QHBoxLayout;
    columns->setSpacing(16);
    columns->addWidget(makeSection(tr("Прошедшие"), pastScroll, "PastExamSection"), 1);
    columns->addWidget(makeSection(tr("Запланированные"), plannedScroll, "PlannedExamSection"), 1);

    root->addWidget(makeSection(tr("Добавить обследование"), addForm));
    root->addLayout(columns, 1);

    populateExamSubtypeOptions();

    connect(m_examType, &QComboBox::currentIndexChanged, this, [this](int) {
        populateExamSubtypeOptions();
    });

    connect(m_examDropArea, &ExamAttachmentDropArea::filesChanged,
            this, [this](const QStringList& files) {
                m_pendingExamFiles = files;
            });

    connect(upload, &QPushButton::clicked, this, &MainWindow::pickExamFile);
    connect(m_saveExamBtn, &QPushButton::clicked, this, &MainWindow::submitExaminationForm);

    // при создании страницы, после создания m_examDropArea:
    connect(m_examDropArea, &ExamAttachmentDropArea::fileRemoved,
        this, [this](const QString& fileId) {
            if (!fileId.isEmpty()) {
                // Удаляем файл из БД
                m_core->deleteAttachment(fileId);  // нужно добавить этот метод в AppCore
                // Убираем из локального списка
                for (int i = 0; i < m_editExamAttachments.size(); ++i) {
                    if (m_editExamAttachments[i].id == fileId) {
                        m_editExamAttachments.removeAt(i);
                        break;
                    }
                }
            }
        });

    return page;
}







// --- submitExaminationForm ---
void MainWindow::submitExaminationForm()
{
    const QString type    = m_examType ? m_examType->currentText().trimmed() : QString();
    const QString subtype = m_examSubtype ? m_examSubtype->currentText().trimmed() : QString();
    const QString notes   = m_examNotes ? m_examNotes->toPlainText().trimmed() : QString();

    if (type.isEmpty()) {
        showToast(m_central, tr("Укажите тип обследования"), ToastKind::Error, 2600);
        return;
    }

    if (!m_examDate || !m_examDate->dateTime().isValid()) {
        showToast(m_central, tr("Укажите дату обследования"), ToastKind::Error, 2600);
        return;
    }

    QJsonObject payload;
    payload["user_id"]      = m_core->currentUserId();
    payload["exam_type"]    = type;
    payload["exam_subtype"] = subtype;
    payload["planned_at"]   = m_examDate->dateTime().toUTC().toString(Qt::ISODate);
    payload["notes"]        = notes;

    // Запоминаем тип и дату для генерации имён файлов
    m_pendingExamType = type;
    m_pendingExamSubtype = subtype;
    m_pendingExamDate = m_examDate->dateTime();

    if (m_editExaminationId.isEmpty()) {
        // НОВОЕ ОБСЛЕДОВАНИЕ
        m_pendingExamFilesForNewExam = m_pendingExamFiles;
        m_core->createExamination(payload);
    } else {
        // РЕДАКТИРОВАНИЕ СУЩЕСТВУЮЩЕГО
        m_core->updateExamination(m_editExaminationId, payload);

        // Собираем новые файлы для загрузки
        QStringList newFiles;
        for (const QString &path : std::as_const(m_pendingExamFiles)) {
            // Пропускаем виртуальные пути существующих файлов
            if (!path.startsWith("ATTACHMENT://")) {
                newFiles << path;
            }
        }

        // Загружаем новые файлы
        int fileIndex = m_editExamAttachments.size() + 1;
        for (const QString &path : newFiles) {
            if (path.isEmpty()) continue;

            QString extension = QFileInfo(path).suffix().toLower();
            if (!extension.isEmpty()) {
                extension = "." + extension;
            }

            QString typeNorm = m_pendingExamType.toLower().replace(' ', '_');
            QString subtypeNorm = m_pendingExamSubtype.toLower().replace(' ', '_');
            if (subtypeNorm.isEmpty()) subtypeNorm = "general";

            QString dateStr = m_pendingExamDate.toString("yyyy-MM-dd");

            QString baseName = QString("%1_%2_%3_%4")
                .arg(typeNorm)
                .arg(subtypeNorm)
                .arg(dateStr)
                .arg(fileIndex++);

            QString fileName = baseName + extension;
            QString storagePath = QString("%1/exams/%2/%3")
                .arg(m_core->currentUserId(), m_editExaminationId, fileName);

            m_core->uploadAttachment(path, storagePath, m_editExaminationId);
        }

        // Очищаем временные данные
        m_pendingExamFiles.clear();
        m_pendingExamFilesForNewExam.clear();
        m_editExamAttachments.clear();
    }

    // Сбрасываем форму
    m_editExaminationId.clear();
    resetExaminationForm();

    // Обновляем список обследований
    m_core->fetchExaminations();
    m_core->fetchAttachments();
}


// --- handleAttachmentsReady ---
void MainWindow::handleAttachmentsReady(const QJsonArray& attachments)
{
    m_cachedAttachments = attachments;
    fillExaminations(m_cachedExaminations);
    // Можно сразу перестроить список обследований, если хочешь, чтобы файлы появлялись
}




// --- resetExaminationForm ---
void MainWindow::resetExaminationForm()
{
    m_editExaminationId.clear();
    if (m_saveExamBtn)
        m_saveExamBtn->setText(tr("Сохранить обследование"));

    if (m_examType)
    {
        m_examType->setCurrentIndex(0);
        populateExamSubtypeOptions();
    }

    if (m_examDate)
        m_examDate->setDateTime(QDateTime::currentDateTime());

    if (m_examNotes)
        m_examNotes->clear();

    m_pendingExamFiles.clear();
}


// --- editExamination ---
void MainWindow::editExamination(const QJsonObject& exam)
{
    if (!m_examType || !m_examSubtype || !m_examDate || !m_examNotes)
        return;

    m_editExaminationId = exam.value("id").toString();
    if (m_saveExamBtn)
        m_saveExamBtn->setText(tr("Обновить обследование"));

    m_examType->setCurrentText(exam.value("exam_type").toString());
    populateExamSubtypeOptions();
    m_examSubtype->setCurrentText(exam.value("exam_subtype").toString());

    QDateTime dt = QDateTime::fromString(exam.value("planned_at").toString(), Qt::ISODate);
    if (!dt.isValid())
        dt = QDateTime::fromString(exam.value("completed_at").toString(), Qt::ISODate);
    if (!dt.isValid())
        dt = QDateTime::currentDateTime();
    m_examDate->setDateTime(dt);

    const QString notes = exam.value("result_note").toString().trimmed().isEmpty()
                              ? exam.value("notes").toString()
                              : exam.value("result_note").toString();
    m_examNotes->setPlainText(notes);

    // 🔥 ЗАГРУЖАЕМ ФАЙЛЫ В DRAG & DROP ЗОНУ
    QStringList filePaths;
    const QString examId = m_editExaminationId;

    for (const auto& v : std::as_const(m_cachedAttachments)) {
        const QJsonObject a = v.toObject();
        if (a.value("examination_id").toString() != examId)
            continue;

        const QString storagePath = a.value("storage_path").toString();
        const QString fileName = a.value("file_name").toString();

        // Сохраняем метаданные для каждого файла
        // Нам нужен оригинальный storagePath, чтобы потом удалить/обновить
        FileAttachmentInfo info;
        info.id = a.value("id").toString();
        info.storagePath = storagePath;
        info.fileName = fileName;
        m_editExamAttachments.append(info);

        // Показываем в зоне дропа (виртуальный путь, который мы запомним)
        filePaths << QString("ATTACHMENT://%1|%2").arg(info.id, fileName);
    }

    // Обновляем дроп-зону
    if (m_examDropArea) {
        m_examDropArea->setFiles(filePaths);
        m_pendingExamFiles = filePaths;
    }

    // Запоминаем тип и дату для новых файлов
    m_pendingExamType = m_examType->currentText().trimmed();
    m_pendingExamSubtype = m_examSubtype->currentText().trimmed();
    m_pendingExamDate = m_examDate->dateTime();

    navigateTo(3, tr("Обследования"));
}


// --- deleteExamination ---
void MainWindow::deleteExamination(const QJsonObject& exam)
{
    const QString id = exam.value("id").toString();
    if (id.isEmpty()) return;

    if (showConfirmDialog(tr("Удалить обследование"),
                          tr("Удалить обследование \"%1\"?").arg(exam.value("exam_type").toString()))) {
        m_core->deleteExamination(id);
        if (m_editExaminationId == id) resetExaminationForm();
    }
}


// --- showExaminationMenu ---
void MainWindow::showExaminationMenu(const QJsonObject& exam, QWidget* anchor)
{
    if (!anchor) return;

    QMenu menu(anchor);
    QAction* editAct = menu.addAction(tr("Редактировать"));
    QAction* duplicateAct = menu.addAction(tr("Заполнить как шаблон"));
    menu.addSeparator();
    QAction* deleteAct = menu.addAction(tr("Удалить"));

    QAction* chosen = menu.exec(anchor->mapToGlobal(anchor->rect().bottomLeft()));
    if (!chosen) return;

    if (chosen == editAct || chosen == duplicateAct) {
        editExamination(exam);
        return;
    }

    if (chosen == deleteAct) {
        deleteExamination(exam);
    }
}


// --- populateExamSubtypeOptions ---
void MainWindow::populateExamSubtypeOptions()
{
    if (!m_examType || !m_examSubtype) return;

    m_examSubtype->clear();
    const QString type = m_examType->currentText();

    if (type == tr("МРТ")) {
        m_examSubtype->addItems({tr("Без контраста"), tr("С контрастом"), tr("Функциональная"), tr("Анатомическая")});
    } else if (type == tr("КТ")) {
        m_examSubtype->addItems({tr("Без контраста"), tr("С контрастом"), tr("Ангиография")});
    } else if (type == tr("ЭЭГ")) {
        m_examSubtype->addItems({tr("Рутинная"), tr("Ночная"), tr("С видеомониторингом")});
    } else if (type == tr("ЭКГ")) {
        m_examSubtype->addItems({tr("Стандартная"), tr("С нагрузкой")});
    } else if (type == tr("Прием врача")) {
        m_examSubtype->addItems({tr("Эпилептолог"), tr("Невролог"), tr("Терапевт")});
    } else {
        m_examSubtype->addItems({tr("Стандартное")});
    }
}



// --- pickExamFile ---
    void MainWindow::pickExamFile()
    {
        const QStringList paths = QFileDialog::getOpenFileNames(
            this,
            tr("Выбрать файлы обследования"),
            QString(),
            tr("Изображения и PDF (*.png *.jpg *.jpeg *.pdf);;Все файлы (*.*)")
        );

        if (paths.isEmpty())
            return;

        // 🔥 Запоминаем тип и дату для генерации нормальных имён
        m_pendingExamType = m_examType->currentText().trimmed();
        m_pendingExamSubtype = m_examSubtype->currentText().trimmed();
        m_pendingExamDate = m_examDate->dateTime();

        for (const QString &p : paths) {
            if (!p.isEmpty() && !m_pendingExamFiles.contains(p))
                m_pendingExamFiles.append(p);
        }

        if (m_examDropArea)
            m_examDropArea->setFiles(m_pendingExamFiles);

        if (m_saveExamBtn) {
            if (m_pendingExamFiles.isEmpty()) {
                m_saveExamBtn->setText(tr("Сохранить обследование"));
            } else if (m_pendingExamFiles.size() == 1) {
                m_saveExamBtn->setText(tr("Сохранить обследование (1 файл)"));
            } else {
                m_saveExamBtn->setText(tr("Сохранить обследование (%1 файлов)").arg(m_pendingExamFiles.size()));
            }
        }
    }


// --- fillExaminations ---
void MainWindow::fillExaminations(const QJsonArray& arr)
{
    m_cachedExaminations = arr;

    auto clearLayout = [](QVBoxLayout* lay) {
        if (!lay) return;
        while (QLayoutItem* item = lay->takeAt(0)) {
            if (QWidget* w = item->widget())
                w->deleteLater();
            delete item;
        }
    };

    ::clearLayout(m_pastExamLayout);
    ::clearLayout(m_plannedExamLayout);

    const QDateTime now = QDateTime::currentDateTime();

    int pastCount = 0;
    int plannedCount = 0;

    for (const auto& v : arr) {
        const QJsonObject o = v.toObject();
        const QString examId  = o.value("id").toString();
        const QString type    = o.value("exam_type").toString();
        const QString subtype = o.value("exam_subtype").toString();

        QDateTime dt = QDateTime::fromString(o.value("planned_at").toString(), Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(o.value("completed_at").toString(), Qt::ISODate);
        if (dt.isValid())
            dt = dt.toLocalTime();

        const bool isPast = dt.isValid() ? (dt <= now) : false;

        // --- карточка ---

        auto* card = new QFrame(isPast ? m_pastExamHost : m_plannedExamHost);
        card->setObjectName("FeedCard");
        card->setContextMenuPolicy(Qt::CustomContextMenu);

        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(16, 16, 16, 16);
        cl->setSpacing(8);

        const QString titleText = subtype.isEmpty()
                                      ? type
                                      : QString("%1 — %2").arg(type, subtype);

        auto* title = new QLabel(titleText, card);
        title->setObjectName("CardTitle");

        QString dateText;
        if (dt.isValid())
            dateText = dt.toLocalTime().toString("dd.MM.yyyy HH:mm");
        else
            dateText = tr("Дата не указана");

        auto* dateLbl = new QLabel(dateText, card);
        dateLbl->setObjectName("MutedLabel");

        const QString noteText = o.value("result_note").toString().trimmed().isEmpty()
                                     ? o.value("notes").toString()
                                     : o.value("result_note").toString();

        cl->addWidget(title);
        cl->addWidget(dateLbl);

        if (!noteText.trimmed().isEmpty()) {
            auto* note = new QLabel(noteText, card);
            note->setWordWrap(true);
            note->setObjectName("MutedLabel");
            cl->addWidget(note);
        }

        // --- файлы: иконка + имя ---

        auto* filesRow = new QHBoxLayout;
        filesRow->setSpacing(8);

        for (const auto& av : std::as_const(m_cachedAttachments)) {
            const QJsonObject a = av.toObject();
            if (a.value("examination_id").toString() != examId)
                continue;

            const QString fileName    = a.value("file_name").toString();
            const QString storagePath = a.value("storage_path").toString();

            auto* btn = new QPushButton(card);
            btn->setObjectName("GhostButton");
            btn->setCursor(Qt::PointingHandCursor);
            btn->setIcon(QIcon(":/icons/description.svg"));
            btn->setText(fileName);
            btn->setToolTip(tr("Открыть вложение"));

            connect(btn, &QPushButton::clicked, this, [this, storagePath]() {
                QDesktopServices::openUrl(QUrl(m_core->makePublicAttachmentUrl(storagePath)));
            });

            filesRow->addWidget(btn);
        }

        if (filesRow->count() > 0) {
            cl->addSpacing(6);
            cl->addLayout(filesRow);
        }

        // --- контекстное меню ---

        connect(card, &QWidget::customContextMenuRequested,
                this, [this, o](const QPoint& pos) {
                    auto* cardWidget = qobject_cast<QWidget*>(sender());
                    if (!cardWidget)
                        return;

                    QMenu menu(cardWidget);
                    QAction* editAct   = menu.addAction(tr("Редактировать"));
                    QAction* deleteAct = menu.addAction(tr("Удалить"));

                    QAction* chosen = menu.exec(cardWidget->mapToGlobal(pos));
                    if (!chosen)
                        return;

                    if (chosen == editAct) {
                        editExamination(o);
                    } else if (chosen == deleteAct) {
                        deleteExamination(o);
                    }
                });

        // --- в нужный список ---

        if (isPast) {
            m_pastExamLayout->addWidget(card);
            ++pastCount;
        } else {
            m_plannedExamLayout->addWidget(card);
            ++plannedCount;
        }
    }

    if (pastCount == 0) {
        auto* empty = new QLabel(tr("Пока нет прошедших обследований."), m_pastExamHost);
        empty->setObjectName("MutedLabel");
        empty->setWordWrap(true);
        m_pastExamLayout->addWidget(empty);
    }
    m_pastExamLayout->addStretch();

    if (plannedCount == 0) {
        auto* empty = new QLabel(tr("Пока нет запланированных обследований."), m_plannedExamHost);
        empty->setObjectName("MutedLabel");
        empty->setWordWrap(true);
        m_plannedExamLayout->addWidget(empty);
    }
    m_plannedExamLayout->addStretch();

    updateDashboardCalendarDecorations();
    refreshDashboardDayDetails();
}



// --- makeExamCard ---
QWidget* MainWindow::makeExamCard(const QJsonObject& exam)
{
    auto* card = new QFrame;
    card->setObjectName("SectionCard");

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(6);

    const QString examId      = exam.value("id").toString();
    const QString type        = exam.value("exam_type").toString();
    const QString subtype     = exam.value("exam_subtype").toString();
    const QString notes       = exam.value("notes").toString().trimmed();
    const QString resultNote  = exam.value("result_note").toString().trimmed();

    QDateTime dt = QDateTime::fromString(exam.value("planned_at").toString(), Qt::ISODate);
    if (!dt.isValid())
        dt = QDateTime::fromString(exam.value("completed_at").toString(), Qt::ISODate);

    const QString titleText = subtype.isEmpty()
                                  ? type
                                  : QString("%1 — %2").arg(type, subtype);

    auto* titleLbl = new QLabel(titleText, card);
    titleLbl->setObjectName("CardTitle");

    QString when;
    if (dt.isValid())
        when = dt.toLocalTime().toString("dd.MM.yyyy HH:mm");
    else
        when = tr("Дата не указана");

    auto* whenLbl = new QLabel(when, card);
    whenLbl->setObjectName("MutedLabel");

    layout->addWidget(titleLbl);
    layout->addWidget(whenLbl);

    const QString textToShow = !resultNote.isEmpty() ? resultNote : notes;
    if (!textToShow.isEmpty()) {
        auto* notesLbl = new QLabel(textToShow, card);
        notesLbl->setObjectName("MutedLabel");
        notesLbl->setWordWrap(true);
        layout->addWidget(notesLbl);
    }

    // Вложения
    auto* filesRow = new QHBoxLayout;
    filesRow->setSpacing(6);

    for (const auto& v : std::as_const(m_cachedAttachments)) {
        const QJsonObject a = v.toObject();
        if (a.value("examinationid").toString() != examId)
            continue;

        const QString fileName    = a.value("filename").toString();
        const QString storagePath = a.value("storagepath").toString();

        auto* btn = new QPushButton(fileName, card);
        btn->setObjectName("GhostButton");
        btn->setToolTip(tr("Открыть вложение"));

        connect(btn, &QPushButton::clicked, this, [this, storagePath]() {
            const QString url = m_core->makePublicAttachmentUrl(storagePath);
            QDesktopServices::openUrl(QUrl(url));
        });

        filesRow->addWidget(btn);
    }

    if (filesRow->count() > 0) {
        layout->addSpacing(4);
        layout->addLayout(filesRow);
    }

    // Контекстное меню (редактировать / удалить)
    card->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(card, &QWidget::customContextMenuRequested, this, [this, exam](const QPoint& pos) {
        auto* cardWidget = qobject_cast<QWidget*>(sender());
        if (!cardWidget)
            return;

        QMenu menu(cardWidget);
        QAction* editAct = menu.addAction(tr("Редактировать"));
        QAction* deleteAct = menu.addAction(tr("Удалить"));

        QAction* chosen = menu.exec(cardWidget->mapToGlobal(pos));
        if (!chosen)
            return;

        if (chosen == editAct) {
            editExamination(exam);
        } else if (chosen == deleteAct) {
            deleteExamination(exam);
        }
    });

    return card;
}






