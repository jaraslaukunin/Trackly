#include "mainwindow.h"
#include "appcore.h"
#include "styles.h"
#include "tracklycalendar.h"
#include "examattachmentdroparea.h"

#include <QApplication>
#include <QCalendarWidget>
#include <QDateTimeEdit>
#include <QDesktopServices>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsBlurEffect>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSettings>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyleHints>
#include <QSvgRenderer>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QUuid>
#include <algorithm>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QPdfWriter>
#include <QPageSize>
#include <QPainter>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QPieSeries>
#include <QCheckBox>
#include <QProgressBar>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QDebug>
#include "widgets/toastwidget.h"
#include "utils.h"


MainWindow::MainWindow(AppCore* core, QWidget* parent)
    : QMainWindow(parent)
    , m_core(core)
{
    setWindowTitle("TRACKLY DEBUG BUILD !!!");

    buildUi();
    buildHelpOverlay();
    buildAuthOverlay();
    wire();
    retranslateUi();
    updateSidebarUi();
    updateThemeIcon();
    updateIcons();
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icons/pill.svg"));
    m_trayIcon->show();

    m_core->fetchMedications();
    m_core->fetchMedicationSchedules();
    m_core->fetchMedicationLogs();

    m_core->fetchDashboard();
    m_core->fetchProfile();
    m_core->fetchAttachments();
    startMedicationReminderLoop();

    connect(m_core, &AppCore::attachmentsReady,
            this, &MainWindow::handleAttachmentsReady);
}

QString MainWindow::formatTs(const QString& iso)
{
    return formatIsoToLocal(iso);
}

void MainWindow::clearLayout(QLayout* layout)
{
    clearLayoutHelper(layout);
}

bool MainWindow::isDarkTheme() const
{
    return qApp->styleSheet().contains("#17141E");
}

QIcon MainWindow::makeTintedIcon(const QString& path, const QColor& color, const QSize& size)
{
    QPixmap pm(size);
    pm.fill(Qt::transparent);

    QSvgRenderer renderer(path);
    QPainter painter(&pm);
    renderer.render(&painter);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pm.rect(), color);
    painter.end();

    return QIcon(pm);
}

QPushButton* MainWindow::makeNavButton(const QString& text, const QString& iconPath)
{
    auto* btn = new QPushButton(text, this);
    btn->setObjectName("SidebarNavButton");
    btn->setCheckable(true);
    btn->setProperty("iconPath", iconPath);
    btn->setProperty("fullText", text);

    const QSize iconSize(24, 24);
    const bool dark = isDarkTheme();
    const QColor iconColor = dark ? QColor("#F3EEFF") : QColor("#2B2144");

    btn->setIcon(makeTintedIcon(iconPath, iconColor, iconSize));
    btn->setIconSize(iconSize);

    // ← ЖЕСТЬ
    btn->setMinimumWidth(200);
    btn->setMaximumWidth(250);
    btn->setFixedHeight(48);
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    btn->setCursor(Qt::PointingHandCursor);

    return btn;
}

QWidget* MainWindow::makeSummaryCard(const QString& title, const QString& body, QLabel** valueLabel)
{
    auto* card = new QPushButton;
    card->setObjectName("SummaryCard");
    card->setFlat(true);
    card->setCursor(Qt::PointingHandCursor);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(8);

    auto* t = new QLabel(title, card);
    t->setObjectName("SummaryTitle");

    auto* v = new QLabel(body, card);
    v->setObjectName("SummaryValue");
    v->setWordWrap(true);

    auto* hint = new QLabel(tr("Открыть"), card);
    hint->setObjectName("SummaryHint");

    layout->addWidget(t);
    layout->addWidget(v);
    layout->addStretch();
    layout->addWidget(hint);

    *valueLabel = v;
    return card;
}

QWidget* MainWindow::makeSection(const QString& title, QWidget* inner, const QString& objectName)
{
    auto* frame = new QFrame;
    frame->setObjectName(objectName);
    frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(14);

    auto* t = new QLabel(title, frame);
    t->setObjectName("SectionTitle");
    t->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    if (inner) {
        inner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        inner->setAttribute(Qt::WA_StyledBackground, false);
    }

    layout->addWidget(t, 0, Qt::AlignTop);
    layout->addWidget(inner, 1);

    return frame;
}

QWidget* MainWindow::buildSidebar()
{
    qDebug() << "BUILDSIDEBAR CALLED";

    m_sidebar = new QFrame(this);
    m_sidebar->setObjectName("Sidebar");
    m_sidebar->setFixedWidth(250);

    auto* layout = new QVBoxLayout(m_sidebar);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    // Верх: логотип
    m_brandRow = new QWidget(m_sidebar);
    m_brandRow->setObjectName("BrandRow");
    auto* brandLayout = new QHBoxLayout(m_brandRow);
    brandLayout->setContentsMargins(0, 0, 0, 0);
    brandLayout->setSpacing(6);

    m_brandTrackly = new QLabel("Trackly", m_brandRow);
    m_brandTrackly->setObjectName("LogoTrackly");

    m_brandDesktop = new QLabel("Desktop", m_brandRow);
    m_brandDesktop->setObjectName("LogoDesktop");

    brandLayout->addWidget(m_brandTrackly);
    brandLayout->addWidget(m_brandDesktop);
    brandLayout->addStretch();

    // Навигационные кнопки
    const QSize navSize(20, 20);

    auto makeNav = [&](QPushButton*& outBtn,
                       const QString& text,
                       const QString& iconPath)
    {
        outBtn = new QPushButton(text, m_sidebar);
        outBtn->setObjectName("SidebarNavButton");
        outBtn->setCheckable(true);
        outBtn->setProperty("iconPath", iconPath);
        outBtn->setProperty("fullText", text);
        outBtn->setCursor(Qt::PointingHandCursor);
        outBtn->setMinimumHeight(48);
        outBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        outBtn->setIconSize(navSize);

        // Пока ставим сырую иконку, цвет иконок потом добьём в updateIcons
        outBtn->setIcon(QIcon(iconPath));
    };

    makeNav(m_navHome,      tr("Главная"),        QStringLiteral(":/icons/home.svg"));
    makeNav(m_navSeizures,  tr("Приступы"),       QStringLiteral(":/icons/seizure.svg"));
    makeNav(m_navMeds,      tr("Терапия"),        QStringLiteral(":/icons/pill.svg"));
    makeNav(m_navExams,     tr("Обследования"),   QStringLiteral(":/icons/description.svg"));
    makeNav(m_navAnalytics, tr("Аналитика"),      QStringLiteral(":/icons/monitoring.svg"));
    makeNav(m_navTriggers,  tr("Триггеры"),       QStringLiteral(":/icons/warning.svg"));
    makeNav(m_navProfile,   tr("Профиль"),        QStringLiteral(":/icons/person.svg"));

    // Нижние кнопки: тема и язык
    m_themeBtn = new QPushButton(tr("Тема"), m_sidebar);
    m_themeBtn->setObjectName("SidebarNavButton");
    m_themeBtn->setCheckable(false);
    m_themeBtn->setCursor(Qt::PointingHandCursor);
    m_themeBtn->setMinimumHeight(48);
    m_themeBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_themeBtn->setIconSize(navSize);
    m_themeBtn->setProperty("iconPath", QStringLiteral(":/icons/light_mode.svg"));
    m_themeBtn->setProperty("fullText", tr("Тема"));
    m_themeBtn->setIcon(QIcon(":/icons/light_mode.svg"));

    m_languageBtn = new QPushButton(tr("Язык"), m_sidebar);
    m_languageBtn->setObjectName("SidebarNavButton");
    m_languageBtn->setCheckable(false);
    m_languageBtn->setCursor(Qt::PointingHandCursor);
    m_languageBtn->setMinimumHeight(48);
    m_languageBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_languageBtn->setIconSize(navSize);
    m_languageBtn->setProperty("iconPath", QStringLiteral(":/icons/language.svg"));
    m_languageBtn->setProperty("fullText", tr("Язык"));
    m_languageBtn->setIcon(QIcon(":/icons/language.svg"));

    // Сборка layout
    layout->addWidget(m_brandRow);
    layout->addSpacing(8);
    layout->addWidget(m_navHome);
    layout->addWidget(m_navSeizures);
    layout->addWidget(m_navMeds);
    layout->addWidget(m_navExams);
    layout->addWidget(m_navAnalytics);
    layout->addWidget(m_navTriggers);
    layout->addWidget(m_navProfile);
    layout->addStretch();
    layout->addWidget(m_themeBtn);
    layout->addWidget(m_languageBtn);

    return m_sidebar;
}

QWidget* MainWindow::buildTopbar()
{
    auto* bar = new QWidget;
    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    m_title = new QLabel(tr("Дашборд"), bar);
    m_title->setObjectName("PageTitle");

    m_helpButton = new QToolButton(bar);
    m_helpButton->setObjectName("HelpButton");
    m_helpButton->setIconSize(QSize(18, 18));
    m_helpButton->setToolTip(tr("Памятка при приступе"));

    layout->addWidget(m_title);
    layout->addStretch();
    layout->addWidget(m_helpButton);

    return bar;
}

QWidget* MainWindow::createDashboardPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(16);

    auto* row = new QHBoxLayout;
    row->setSpacing(14);

    auto* c1 = qobject_cast<QPushButton*>(makeSummaryCard(tr("Приступы"), tr("0 за текущий месяц"), &m_summary1));
    auto* c2 = qobject_cast<QPushButton*>(makeSummaryCard(tr("Лекарства"), tr("0 приемов сегодня"), &m_summary2));
    auto* c3 = qobject_cast<QPushButton*>(makeSummaryCard(tr("Обследования"), tr("Нет ближайших записей"), &m_summary3));
    auto* c4 = qobject_cast<QPushButton*>(makeSummaryCard(tr("Врач / профиль"), tr("Не указан"), &m_summary4));

    m_dashboardReportBtn = new QPushButton(tr("Создать отчёт"), page);
    m_dashboardReportBtn->setObjectName("PrimaryButton");
    m_dashboardReportBtn->setIcon(QIcon(":/icons/description.svg"));
    root->addWidget(m_dashboardReportBtn, 0, Qt::AlignRight);
    connect(m_dashboardReportBtn, &QPushButton::clicked, this, &MainWindow::createPdfReport);

    row->addWidget(c1);
    row->addWidget(c2);
    row->addWidget(c3);
    row->addWidget(c4);
    root->addLayout(row);

    m_dashboardCalendar = new TracklyCalendar(page);
    m_dashboardCalendar->setObjectName("MainCalendar");

    m_dashboardExamHost = new QWidget(page);
    m_dashboardExamHost->setObjectName("DashboardExamHost");
    m_dashboardExamLayout = new QVBoxLayout(m_dashboardExamHost);
    m_dashboardExamLayout->setContentsMargins(0, 0, 0, 0);
    m_dashboardExamLayout->setSpacing(10);

    auto* empty = new QLabel(tr("Здесь появятся ближайшие обследования."), m_dashboardExamHost);
    empty->setObjectName("MutedLabel");
    empty->setWordWrap(true);
    m_dashboardExamLayout->addWidget(empty);
    m_dashboardExamLayout->addStretch();

    auto* bottom = new QHBoxLayout;
    bottom->setSpacing(16);
    bottom->addWidget(makeSection(tr("Календарь"), m_dashboardCalendar), 3);
    bottom->addWidget(makeSection(tr("Ближайшие обследования"), m_dashboardExamHost), 2);

    root->addLayout(bottom, 1);

    connect(c1, &QPushButton::clicked, this, [this]() { navigateTo(1, tr("Приступы")); });
    connect(c2, &QPushButton::clicked, this, [this]() { navigateTo(2, tr("Лекарства")); });
    connect(c3, &QPushButton::clicked, this, [this]() { navigateTo(3, tr("Обследования")); });
    connect(c4, &QPushButton::clicked, this, [this]() { navigateTo(6, tr("Профиль")); });

    return page;
}

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
    m_saveSeizureBtn->setIcon(QIcon(":/icons/add_circle.svg"));

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

    // Кнопка отметки приёма
    auto* takeBtn = new QPushButton(tr("Отметить приём"), card);
    takeBtn->setObjectName("PrimaryButton");
    takeBtn->setProperty("medId", id);

    layout->addWidget(titleLbl);
    layout->addWidget(innLbl);
    layout->addWidget(doseLbl);
    layout->addWidget(therapyLbl);
    if (timesLbl)
        layout->addWidget(timesLbl);
    layout->addWidget(stockBadge);
    layout->addStretch();
    layout->addWidget(takeBtn);

    // Диалог отметки приёма – берём первое время, если есть
    QString firstTime;
    if (therapy != "prn") {
        if (med.value("times").isArray()) {
            auto arr = med.value("times").toArray();
            if (!arr.isEmpty())
                firstTime = arr.first().toString();
        } else {
            firstTime = med.value("times").toString().split(",", Qt::SkipEmptyParts).value(0).trimmed();
        }
    }

    connect(takeBtn, &QPushButton::clicked, this, [this, med, firstTime]() {
        openMarkIntakeDialog(med, firstTime);
    });

    connect(card, &QWidget::customContextMenuRequested,
            this, &MainWindow::showMedicationMenuFromCard);

    return card;
}

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

void MainWindow::handleDeleteMedicationFromCard()
{
    if (m_contextMenuMedicationId.isEmpty())
        return;

    m_core->deleteMedication(m_contextMenuMedicationId);
}

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



void MainWindow::rebuildMedicationCards()
{
    clearLayoutHelper(m_medCardsLayout);
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

void MainWindow::onMedicationsReady(const QJsonArray& meds)
{
    Q_UNUSED(meds);
    rebuildMedicationCards();
    m_core->fetchMedicationSchedules();
    refreshDayMedicationSummary();
}

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
    m_medCalendar = new QCalendarWidget(page);
    m_medCalendar->setObjectName("MainCalendar");

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
    connect(m_medCalendar, &QCalendarWidget::selectionChanged,
            this, &MainWindow::refreshDayMedicationSummary);

    return page;
}

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
    }

    // therapy == prn → скрываем поля планирования
    auto updatePlanVisibility = [therapyCombo, timesPerDayLbl, timesPerDaySpin, timesLbl, timesEdit]() {
        const QString mode = therapyCombo->currentData().toString();
        const bool isPrn = (mode == "prn");
        timesPerDayLbl->setVisible(!isPrn);
        timesPerDaySpin->setVisible(!isPrn);
        timesLbl->setVisible(!isPrn);
        timesEdit->setVisible(!isPrn);
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
                    obj["times"] = QJsonArray(); // нет планов
                } else {
                    obj["times_per_day"] = timesPerDaySpin->value();

                    QJsonArray timesArray;
                    const QStringList parts = timesEdit->text().split(",", Qt::SkipEmptyParts);
                    for (QString t : parts) {
                        t = t.trimmed();
                        if (!t.isEmpty()) {
                            // приводим к HH:mm
                            const QTime ti = QTime::fromString(t, "HH:mm");
                            const QString out = ti.isValid() ? ti.toString("HH:mm") : t;
                            timesArray.append(out);
                        }
                    }
                    obj["times"] = timesArray;
                }

                if (existingId.isEmpty()) {
                    m_core->createMedication(obj);
                } else {
                    m_core->updateMedication(existingId, obj);
                }

                dlg.accept();
            });

    dlg.exec();

    if (central) {
        central->setGraphicsEffect(nullptr);
    }
}

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
    upload->setIcon(QIcon(":/icons/upload.svg"));

    m_examDropArea = new ExamAttachmentDropArea(addForm);

    m_saveExamBtn = new QPushButton(tr("Сохранить обследование"), addForm);
    m_saveExamBtn->setObjectName("PrimaryButton");
    m_saveExamBtn->setIcon(QIcon(":/icons/order_approve.svg"));

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




QWidget* MainWindow::createTriggersPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(16);

    // ==== Верх: форма ====

    auto* formHost = new QWidget(page);
    auto* formLayout = new QVBoxLayout(formHost);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(12);

    // 1. Настрой
    auto* moodCard = new QFrame(formHost);
    moodCard->setObjectName("SectionCard");
    auto* moodLayout = new QVBoxLayout(moodCard);
    moodLayout->setContentsMargins(18, 18, 18, 18);
    moodLayout->setSpacing(10);

    auto* moodTitle = new QLabel(tr("Сегодняшний настрой"), moodCard);
    moodTitle->setObjectName("SectionTitle");

    auto* moodRow = new QHBoxLayout;
    moodRow->setSpacing(10);

    m_moodScore = new QComboBox(moodCard);
    m_moodScore->addItems({
        tr("1 — очень плохо"),
        tr("2 — плохо"),
        tr("3 — нормально"),
        tr("4 — хорошо"),
        tr("5 — отлично")
    });
    m_moodScore->setCurrentIndex(2);

    m_triggerDate = new QDateTimeEdit(QDateTime::currentDateTime(), moodCard);
    m_triggerDate->setCalendarPopup(true);

    moodRow->addWidget(new QLabel(tr("Настрой"), moodCard));
    moodRow->addWidget(m_moodScore, 1);
    moodRow->addSpacing(12);
    moodRow->addWidget(new QLabel(tr("Дата/время"), moodCard));
    moodRow->addWidget(m_triggerDate, 1);

    auto* moodNoteLabel = new QLabel(tr("Коротко опиши своё состояние"), moodCard);
    moodNoteLabel->setObjectName("MutedLabel");

    m_triggerNote = new QTextEdit(moodCard);
    m_triggerNote->setPlaceholderText(tr("Например: чувствую усталость, немного тревожно"));
    m_triggerNote->setFixedHeight(80);

    moodLayout->addWidget(moodTitle);
    moodLayout->addLayout(moodRow);
    moodLayout->addWidget(moodNoteLabel);
    moodLayout->addWidget(m_triggerNote);

    // 2. Симптомы
    auto* symptomsCard = new QFrame(formHost);
    symptomsCard->setObjectName("SectionCard");
    auto* symptomsLayout = new QVBoxLayout(symptomsCard);
    symptomsLayout->setContentsMargins(18, 18, 18, 18);
    symptomsLayout->setSpacing(10);

    auto* symptomsTitle = new QLabel(tr("Симптомы"), symptomsCard);
    symptomsTitle->setObjectName("SectionTitle");

    auto* symptomsChips = new QWidget(symptomsCard);
    auto* symptomsChipsLayout = new QHBoxLayout(symptomsChips);
    symptomsChipsLayout->setContentsMargins(0, 0, 0, 0);
    symptomsChipsLayout->setSpacing(8);

    const QStringList symptomOptions = {
        tr("Головная боль"),
        tr("Головокружение"),
        tr("Тошнота"),
        tr("Сонливость"),
        tr("Тревога"),
        tr("Раздражительность")
    };

    QList<QCheckBox*> symptomChecks;
    for (const QString& label : symptomOptions) {
        auto* cb = new QCheckBox(label, symptomsChips);
        cb->setObjectName("ChipCheckBox");
        symptomChecks << cb;
        symptomsChipsLayout->addWidget(cb);
    }

    auto* otherSymptomRow = new QHBoxLayout;
    otherSymptomRow->setSpacing(8);
    auto* otherSymptomLabel = new QLabel(tr("Другой симптом"), symptomsCard);
    auto* otherSymptomEdit = new QLineEdit(symptomsCard);
    otherSymptomEdit->setPlaceholderText(tr("Например: онемение, шум в ушах"));

    otherSymptomRow->addWidget(otherSymptomLabel);
    otherSymptomRow->addWidget(otherSymptomEdit, 1);

    symptomsLayout->addWidget(symptomsTitle);
    symptomsLayout->addWidget(symptomsChips);
    symptomsLayout->addLayout(otherSymptomRow);

    // 3. Триггеры
    auto* triggersCard = new QFrame(formHost);
    triggersCard->setObjectName("SectionCard");
    auto* triggersLayout = new QVBoxLayout(triggersCard);
    triggersLayout->setContentsMargins(18, 18, 18, 18);
    triggersLayout->setSpacing(10);

    auto* triggersTitle = new QLabel(tr("Триггеры"), triggersCard);
    triggersTitle->setObjectName("SectionTitle");

    auto* triggerChips = new QWidget(triggersCard);
    auto* triggerChipsLayout = new QHBoxLayout(triggerChips);
    triggerChipsLayout->setContentsMargins(0, 0, 0, 0);
    triggerChipsLayout->setSpacing(8);

    const QStringList triggerOptions = {
        tr("Недосып"),
        tr("Стресс"),
        tr("Пропуск лекарства"),
        tr("Яркий свет"),
        tr("Шум"),
        tr("Алкоголь")
    };

    QList<QCheckBox*> triggerChecks;
    for (const QString& label : triggerOptions) {
        auto* cb = new QCheckBox(label, triggerChips);
        cb->setObjectName("ChipCheckBox");
        triggerChecks << cb;
        triggerChipsLayout->addWidget(cb);
    }

    auto* otherTriggerRow = new QHBoxLayout;
    otherTriggerRow->setSpacing(8);
    auto* otherTriggerLabel = new QLabel(tr("Другой триггер"), triggersCard);
    auto* otherTriggerEdit = new QLineEdit(triggersCard);
    otherTriggerEdit->setPlaceholderText(tr("Например: жара, сильные запахи"));

    otherTriggerRow->addWidget(otherTriggerLabel);
    otherTriggerRow->addWidget(otherTriggerEdit, 1);

    triggersLayout->addWidget(triggersTitle);
    triggersLayout->addWidget(triggerChips);
    triggersLayout->addLayout(otherTriggerRow);

    // Кнопка сохранения
    m_saveTriggerBtn = new QPushButton(tr("Сохранить запись"), formHost);
    m_saveTriggerBtn->setObjectName("PrimaryButton");
    m_saveTriggerBtn->setIcon(QIcon(":/icons/add_circle.svg"));

    formLayout->addWidget(moodCard);
    formLayout->addWidget(symptomsCard);
    formLayout->addWidget(triggersCard);
    formLayout->addWidget(m_saveTriggerBtn, 0, Qt::AlignRight);

    // ==== Низ: история ====

    m_triggerListHost = new QWidget(page);
    m_triggerListHost->setObjectName("TriggerListHost");
    m_triggerListLayout = new QVBoxLayout(m_triggerListHost);
    m_triggerListLayout->setContentsMargins(0, 0, 0, 0);
    m_triggerListLayout->setSpacing(10);

    auto* placeholder = new QLabel(tr("После добавления записей здесь появится история самочувствия."), m_triggerListHost);
    placeholder->setObjectName("MutedLabel");
    placeholder->setWordWrap(true);
    m_triggerListLayout->addWidget(placeholder);
    m_triggerListLayout->addStretch();

    auto* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(m_triggerListHost);

    auto* contentRow = new QHBoxLayout;
    contentRow->setSpacing(16);
    contentRow->addWidget(formHost, 2);
    contentRow->addWidget(scroll, 3);

    root->addLayout(contentRow, 1);

    // Связываем: сохраняем выбранные чекбоксы через лямбда в submitTrigger
    connect(m_saveTriggerBtn, &QPushButton::clicked, this, [this, symptomChecks, otherSymptomEdit, triggerChecks, otherTriggerEdit]() {
        QStringList symptoms;
        for (auto* cb : symptomChecks) {
            if (cb->isChecked()) symptoms << cb->text().trimmed();
        }
        if (!otherSymptomEdit->text().trimmed().isEmpty())
            symptoms << otherSymptomEdit->text().trimmed();

        QStringList triggers;
        for (auto* cb : triggerChecks) {
            if (cb->isChecked()) triggers << cb->text().trimmed();
        }
        if (!otherTriggerEdit->text().trimmed().isEmpty())
            triggers << otherTriggerEdit->text().trimmed();

        submitTriggerWithDetails(symptoms, triggers);
    });

    return page;
}

void MainWindow::submitTriggerWithDetails(const QStringList& symptoms,
                                          const QStringList& triggers)
{
    const QString note = m_triggerNote ? m_triggerNote->toPlainText().trimmed() : QString();

    if (!m_moodScore) {
        showToast(m_central, tr("Форма настроения ещё не готова"), ToastKind::Error, 2600);
        return;
    }

    const int moodValue = m_moodScore->currentIndex() + 1;
    if (moodValue < 1 || moodValue > 5) {
        showToast(m_central, tr("Выбери настрой от 1 до 5"), ToastKind::Error, 2600);
        return;
    }

    if (symptoms.isEmpty() && triggers.isEmpty() && note.isEmpty()) {
        showToast(m_central, tr("Добавь хотя бы симптом, триггер или комментарий"), ToastKind::Error, 2600);
        return;
    }

    QJsonArray symptomsArray;
    for (const auto& s : symptoms) {
        const auto st = s.trimmed();
        if (!st.isEmpty())
            symptomsArray.append(st);
    }

    QJsonArray triggersArray;
    for (const auto& t : triggers) {
        const auto tt = t.trimmed();
        if (!tt.isEmpty())
            triggersArray.append(tt);
    }

    QJsonObject payload{
        {"user_id", m_core->currentUserId()},
        {"logged_at", m_triggerDate->dateTime().toString(Qt::ISODate)},
        {"mood", moodValue},
        {"note", note},
        {"symptoms", symptomsArray},
        {"triggers", triggersArray}
    };

    m_core->createTrigger(payload);

    m_triggerDate->setDateTime(QDateTime::currentDateTime());
    m_moodScore->setCurrentIndex(2);
    if (m_triggerNote) m_triggerNote->clear();
}

QWidget* MainWindow::createProfilePage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(16);

    auto* top = new QHBoxLayout;
    top->setSpacing(16);

    auto* avatarCard = new QWidget(page);
    avatarCard->setObjectName("AvatarCardHost");
    auto* avatarLayout = new QVBoxLayout(avatarCard);
    avatarLayout->setContentsMargins(0, 0, 0, 0);
    avatarLayout->setSpacing(12);

    m_avatarPreview = new QLabel(page);
    m_avatarPreview->setObjectName("AvatarPreview");
    m_avatarPreview->setAlignment(Qt::AlignCenter);
    m_avatarPreview->setMinimumSize(132, 132);
    m_avatarPreview->setMaximumSize(132, 132);
    m_avatarPreview->setText(tr("Фото"));

    auto* uploadAvatarBtn = new QPushButton(tr("Загрузить фото"), page);
    uploadAvatarBtn->setObjectName("SecondaryButton");
    uploadAvatarBtn->setIcon(QIcon(":/icons/upload.svg"));

    avatarLayout->addWidget(m_avatarPreview, 0, Qt::AlignLeft);
    avatarLayout->addWidget(uploadAvatarBtn, 0, Qt::AlignLeft);
    avatarLayout->addStretch();

    auto* form = new QWidget(page);
    form->setObjectName("ProfileFormHost");
    auto* grid = new QGridLayout(form);
    grid->setSpacing(12);

    m_profileFullName = new QLineEdit(form);
    m_profileBirthDate = new QLineEdit(form);
    m_profileGender = new QComboBox(form);
    m_profileGender->addItems({tr("Не указан"), tr("Мужской"), tr("Женский")});
    m_profilePhone = new QLineEdit(form);
    m_profileEmail = new QLineEdit(form);
    m_profileEmail->setReadOnly(true);
    m_profileEmail->setFocusPolicy(Qt::NoFocus);
    m_profileEmail->setPlaceholderText(tr("Email"));
    m_profileEmail->setToolTip(tr("Email используется как логин и не редактируется"));
    m_profileAddress = new QLineEdit(form);
    m_profileEmergency = new QLineEdit(form);
    m_profileDoctor = new QLineEdit(form);
    m_profileDiagnosis = new QTextEdit(form);
    m_profileDiagnosis->setFixedHeight(90);

    auto* saveProfileBtn = new QPushButton(tr("Сохранить профиль"), form);
    saveProfileBtn->setObjectName("PrimaryButton");
    saveProfileBtn->setIcon(QIcon(":/icons/order_approve.svg"));

    auto* logoutBtn = new QPushButton(tr("Выйти"), form);
    logoutBtn->setObjectName("DangerButton");
    logoutBtn->setIcon(QIcon(":/icons/logout.svg"));

    grid->addWidget(new QLabel(tr("ФИО")), 0, 0);
    grid->addWidget(m_profileFullName, 0, 1);
    grid->addWidget(new QLabel(tr("Дата рождения")), 1, 0);
    grid->addWidget(m_profileBirthDate, 1, 1);
    grid->addWidget(new QLabel(tr("Пол")), 2, 0);
    grid->addWidget(m_profileGender, 2, 1);
    grid->addWidget(new QLabel(tr("Телефон")), 3, 0);
    grid->addWidget(m_profilePhone, 3, 1);
    grid->addWidget(new QLabel(tr("Email")), 4, 0);
    grid->addWidget(m_profileEmail, 4, 1);
    grid->addWidget(new QLabel(tr("Адрес")), 5, 0);
    grid->addWidget(m_profileAddress, 5, 1);
    grid->addWidget(new QLabel(tr("Экстренный контакт")), 6, 0);
    grid->addWidget(m_profileEmergency, 6, 1);
    grid->addWidget(new QLabel(tr("Лечащий врач")), 7, 0);
    grid->addWidget(m_profileDoctor, 7, 1);
    grid->addWidget(new QLabel(tr("Диагноз / комментарий")), 8, 0);
    grid->addWidget(m_profileDiagnosis, 8, 1);

    auto* actions = new QHBoxLayout;
    actions->setSpacing(10);
    actions->addWidget(saveProfileBtn);
    actions->addWidget(logoutBtn);
    actions->addStretch();

    grid->addLayout(actions, 9, 0, 1, 2);

    top->addWidget(makeSection(tr("Аватар"), avatarCard), 1);
    top->addWidget(makeSection(tr("Личные данные"), form), 3);

    root->addLayout(top);
    root->addStretch();

    connect(uploadAvatarBtn, &QPushButton::clicked, this, &MainWindow::pickAvatar);

    connect(saveProfileBtn, &QPushButton::clicked, this, [this]() {
        m_core->updateProfile({
            {"id", m_core->currentUserId()},
            {"email", m_core->currentEmail()},
            {"full_name", m_profileFullName->text().trimmed()},
            {"birth_date", m_profileBirthDate->text().trimmed()},
            {"gender", m_profileGender->currentText()},
            {"phone", m_profilePhone->text().trimmed()},
            {"address", m_profileAddress->text().trimmed()},
            {"emergency_phone", m_profileEmergency->text().trimmed()},
            {"doctor_name", m_profileDoctor->text().trimmed()},
            {"diagnosis", m_profileDiagnosis->toPlainText().trimmed()}
        });
    });

    connect(logoutBtn, &QPushButton::clicked, this, [this]() {
        m_core->signOut();
    });

    return page;
}

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
        {"started_at", m_seizureDate->dateTime().toString(Qt::ISODate)},
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

void MainWindow::openMarkIntakeDialog(const QJsonObject& med,
                                      const QString& plannedTime)
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Отметить приём"));

    auto* root = new QVBoxLayout(&dlg);
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

    if (dlg.exec() != QDialog::Accepted)
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

void MainWindow::handleAttachmentsReady(const QJsonArray& attachments)
{
    m_cachedAttachments = attachments;
    fillExaminations(m_cachedExaminations);
    // Можно сразу перестроить список обследований, если хочешь, чтобы файлы появлялись
}



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

void MainWindow::deleteSeizure(const QJsonObject& seizure)
{
    const QString id = seizure.value("id").toString();
    if (id.isEmpty()) return;

    const auto res = QMessageBox::question(
        this, tr("Удалить приступ"), tr("Удалить запись о приступе?"),
        QMessageBox::Yes | QMessageBox::No);

    if (res == QMessageBox::Yes) {
        m_core->deleteSeizure(id);
        if (m_editSeizureId == id) resetSeizureForm();
    }
}

void MainWindow::deleteMedication(const QJsonObject& med)
{
    const QString id = med.value("id").toString();
    if (id.isEmpty()) return;

    const auto res = QMessageBox::question(
        this, tr("Удалить препарат"),
        tr("Удалить \"%1\"?").arg(med.value("title").toString()),
        QMessageBox::Yes | QMessageBox::No);

    if (res == QMessageBox::Yes) {
        m_core->deleteMedication(id);
        if (m_editMedicationId == id) resetMedicationForm();
    }
}

void MainWindow::deleteExamination(const QJsonObject& exam)
{
    const QString id = exam.value("id").toString();
    if (id.isEmpty()) return;

    const auto res = QMessageBox::question(
        this, tr("Удалить обследование"),
        tr("Удалить обследование \"%1\"?").arg(exam.value("exam_type").toString()),
        QMessageBox::Yes | QMessageBox::No);

    if (res == QMessageBox::Yes) {
        m_core->deleteExamination(id);
        if (m_editExaminationId == id) resetExaminationForm();
    }
}

void MainWindow::showSeizureMenu(const QJsonObject& seizure, QWidget* anchor)
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
        editSeizure(seizure);
        return;
    }
    if (chosen == deleteAct) {
        deleteSeizure(seizure);
    }
}

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
        const auto res = QMessageBox::question(
            this,
            tr("Удалить препарат"),
            tr("Удалить препарат?"),
            QMessageBox::Yes | QMessageBox::No);
        if (res == QMessageBox::Yes) {
            m_core->deleteMedication(medId);
            if (m_editMedicationId == medId)
                resetMedicationForm();
        }
    }
}

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

void MainWindow::buildUi()
{
    setWindowTitle("Trackly Desktop");
    resize(1480, 920);
    setMinimumSize(1180, 760);

    m_central = new QWidget(this);
    m_central->setObjectName("CentralWidget");
    setCentralWidget(m_central);

    auto* root = new QHBoxLayout(m_central);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(16);

    root->addWidget(buildSidebar());

    m_content = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(m_content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(14);

    contentLayout->addWidget(buildTopbar());

    m_stack = new QStackedWidget(m_content);
    m_stack->addWidget(createDashboardPage());   // 0
    m_stack->addWidget(createSeizuresPage());    // 1
    m_stack->addWidget(createMedicinesPage());   // 2
    m_stack->addWidget(createExamsPage());       // 3
    m_stack->addWidget(createAnalyticsPage());   // 4
    m_stack->addWidget(createTriggersPage());    // 5
    m_stack->addWidget(createProfilePage());     // 6

    contentLayout->addWidget(m_stack, 1);
    root->addWidget(m_content, 1);

    navigateTo(0, tr("Главная"));
}

void MainWindow::setActiveNav(QPushButton* active)
{
    const QList<QPushButton*> navs = {
        m_navHome,
        m_navSeizures,
        m_navMeds,
        m_navExams,
        m_navAnalytics,
        m_navTriggers,
        m_navProfile
    };

    for (auto* btn : navs) {
        if (btn) {
            btn->setChecked(btn == active);
        }
    }
}

void MainWindow::navigateTo(int index, const QString& title)
{
    m_stack->setCurrentIndex(index);
    m_title->setText(title);

    if (index == 0) setActiveNav(m_navHome);
    else if (index == 1) setActiveNav(m_navSeizures);
    else if (index == 2) setActiveNav(m_navMeds);
    else if (index == 3){
        setActiveNav(m_navExams);
        m_core->fetchAttachments();
        m_core->fetchExaminations();
    }
    else if (index == 4) setActiveNav(m_navAnalytics);
    else if (index == 5) setActiveNav(m_navTriggers);
    else if (index == 6) setActiveNav(m_navProfile);
}

void MainWindow::updateSidebarUi()
{
    if (!m_sidebar) return;

    m_sidebar->setFixedWidth(250);

    if (m_brandRow) m_brandRow->setVisible(true);
    if (m_brandTrackly) m_brandTrackly->setVisible(true);
    if (m_brandDesktop) m_brandDesktop->setVisible(true);

    if (m_languageBtn) m_languageBtn->setText(tr("Язык"));
    if (m_themeBtn) m_themeBtn->setText(tr("Тема"));

    const QList<QPushButton*> navs = {
        m_navHome,
        m_navSeizures,
        m_navMeds,
        m_navExams,
        m_navAnalytics,
        m_navTriggers,
        m_navProfile
    };

    for (auto* btn : navs) {
        if (!btn) continue;
        const QString full = btn->property("fullText").toString().isEmpty()
                                 ? btn->text()
                                 : btn->property("fullText").toString();
        btn->setProperty("fullText", full);
        btn->setText(full);
        btn->setToolTip(full);
    }
}

void MainWindow::updateThemeIcon()
{
    if (!m_themeBtn) return;
    const bool dark = isDarkTheme();
    m_themeBtn->setProperty("iconPath", dark ? ":/icons/light_mode.svg" : ":/icons/dark_mode.svg");
}

void MainWindow::updateIcons()
{
    const bool dark = isDarkTheme();
    qDebug() << "UPDATE ICONS CALLED";

    const QColor iconColor  = dark ? QColor("#F3EEFF") : QColor("#2B2144");
    const QColor helpColor  = dark ? QColor("#F97373") : QColor("#B91C1C");
    const QSize  navSize(20, 20);
    const QSize  helpSize(18, 18);

    // Навигация
    if (m_navHome) {
        m_navHome->setIcon(makeTintedIcon(":/icons/home.svg", iconColor, navSize));
        m_navHome->setIconSize(navSize);
    }
    if (m_navSeizures) {
        m_navSeizures->setIcon(makeTintedIcon(":/icons/seizure.svg", iconColor, navSize));
        m_navSeizures->setIconSize(navSize);
    }
    if (m_navMeds) {
        m_navMeds->setIcon(makeTintedIcon(":/icons/pill.svg", iconColor, navSize));
        m_navMeds->setIconSize(navSize);
    }
    if (m_navExams) {
        m_navExams->setIcon(makeTintedIcon(":/icons/description.svg", iconColor, navSize));
        m_navExams->setIconSize(navSize);
    }
    if (m_navAnalytics) {
        m_navAnalytics->setIcon(makeTintedIcon(":/icons/monitoring.svg", iconColor, navSize));
        m_navAnalytics->setIconSize(navSize);
    }
    if (m_navTriggers) {
        m_navTriggers->setIcon(makeTintedIcon(":/icons/warning.svg", iconColor, navSize));
        m_navTriggers->setIconSize(navSize);
    }
    if (m_navProfile) {
        m_navProfile->setIcon(makeTintedIcon(":/icons/person.svg", iconColor, navSize));
        m_navProfile->setIconSize(navSize);
    }

    // Язык и тема в сайдбаре
    if (m_languageBtn) {
        m_languageBtn->setIcon(makeTintedIcon(":/icons/language.svg", iconColor, navSize));
        m_languageBtn->setIconSize(navSize);
    }

    if (m_themeBtn) {
        const QString themeIconPath = dark
                                          ? QStringLiteral(":/icons/light_mode.svg")
                                          : QStringLiteral(":/icons/dark_mode.svg");
        m_themeBtn->setIcon(makeTintedIcon(themeIconPath, iconColor, navSize));
        m_themeBtn->setIconSize(navSize);
    }

    // Help в топбаре
    if (m_helpButton) {
        m_helpButton->setIcon(makeTintedIcon(":/icons/emergency.svg", helpColor, helpSize));
        m_helpButton->setIconSize(helpSize);
    }
}

void MainWindow::buildHelpOverlay()
{
    m_helpOverlay = new QWidget(m_central);
    m_helpOverlay->setObjectName("HelpOverlay");
    m_helpOverlay->hide();
    m_helpOverlay->raise();

    auto* layout = new QVBoxLayout(m_helpOverlay);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->addStretch();

    auto* row = new QHBoxLayout;
    row->addStretch();

    m_helpCard = new QFrame(m_helpOverlay);
    m_helpCard->setObjectName("HelpOverlayCard");
    m_helpCard->setFixedWidth(520);

    auto* cardLayout = new QVBoxLayout(m_helpCard);
    cardLayout->setContentsMargins(24, 24, 24, 24);
    cardLayout->setSpacing(12);

    auto* title = new QLabel(tr("Памятка при приступе"), m_helpCard);
    title->setObjectName("SectionTitle");

    m_helpTextLabel = new QLabel(
        tr("1. Уберите опасные предметы и поддержите голову.\n"
           "2. Не сдерживайте судороги и ничего не кладите в рот.\n"
           "3. Поверните человека на бок после окончания приступа.\n"
           "4. Засеките длительность приступа.\n"
           "5. Вызывайте 103/112, если приступ длится более 5 минут, повторяется подряд "
           "или человек получил травму."),
        m_helpCard);
    m_helpTextLabel->setObjectName("HelpOverlayText");
    m_helpTextLabel->setWordWrap(true);

    auto* actions = new QHBoxLayout;
    actions->setSpacing(10);

    m_helpCallBtn = new QPushButton(tr("Позвонить"), m_helpCard);
    m_helpCallBtn->setObjectName("SecondaryButton");

    m_helpSmsBtn = new QPushButton(tr("SMS"), m_helpCard);
    m_helpSmsBtn->setObjectName("SecondaryButton");

    m_helpMailBtn = new QPushButton(tr("Email"), m_helpCard);
    m_helpMailBtn->setObjectName("PrimaryButton");

    auto* closeBtn = new QPushButton(tr("Понятно"), m_helpCard);
    closeBtn->setObjectName("SecondaryButton");

    actions->addWidget(m_helpCallBtn);
    actions->addWidget(m_helpSmsBtn);
    actions->addWidget(m_helpMailBtn);
    actions->addStretch();
    actions->addWidget(closeBtn);

    cardLayout->addWidget(title);
    cardLayout->addWidget(m_helpTextLabel);
    cardLayout->addSpacing(8);
    cardLayout->addLayout(actions);

    row->addWidget(m_helpCard);
    row->addStretch();

    layout->addLayout(row);
    layout->addStretch();

    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::hideHelpOverlay);
    connect(m_helpCallBtn, &QPushButton::clicked, this, &MainWindow::openEmergencyCall);
    connect(m_helpSmsBtn, &QPushButton::clicked, this, &MainWindow::openEmergencySms);
    connect(m_helpMailBtn, &QPushButton::clicked, this, &MainWindow::openEmergencyEmail);

    m_helpOverlay->setGeometry(m_central->rect());
}

void MainWindow::showHelpOverlay()
{
    m_helpOverlay->setGeometry(m_central->rect());
    m_helpOverlay->show();
    m_helpOverlay->raise();
    applyBlur(true);
}

void MainWindow::hideHelpOverlay()
{
    m_helpOverlay->hide();
    applyBlur(false);
}

void MainWindow::buildAuthOverlay()
{
    if (mauthOverlay)
        return;

    mauthOverlay = new QWidget(m_central);
    mauthOverlay->setObjectName("AuthOverlay");
    mauthOverlay->hide();
    mauthOverlay->raise();

    auto overlayLayout = new QVBoxLayout(mauthOverlay);
    overlayLayout->setContentsMargins(24, 24, 24, 24);
    overlayLayout->setSpacing(0);
    overlayLayout->addStretch();

    auto row = new QHBoxLayout;
    row->setSpacing(0);
    row->addStretch();

    mauthCard = new QFrame(mauthOverlay);
    mauthCard->setObjectName("AuthOverlayCard");
    mauthCard->setMinimumWidth(460);
    mauthCard->setMaximumWidth(520);

    auto cardLayout = new QVBoxLayout(mauthCard);
    cardLayout->setContentsMargins(28, 28, 28, 24);
    cardLayout->setSpacing(14);

    auto headerRow = new QHBoxLayout;
    headerRow->setSpacing(10);

    mauthLogoLabel = new QLabel(mauthCard);
    mauthLogoLabel->setObjectName("AuthOverlayLogo");
    mauthLogoLabel->setFixedSize(56, 56);
    mauthLogoLabel->setAlignment(Qt::AlignCenter);

    auto renderAuthLogo = [this]() -> QPixmap {
        const QString path = isDarkTheme()
        ? QStringLiteral(":/icons/logo-trackly.svg")
        : QStringLiteral(":/icons/logo-trackly.svg");

        const QSize targetSize(56, 56);
        QPixmap pm(targetSize);
        pm.fill(Qt::transparent);

        QSvgRenderer renderer(path);
        if (!renderer.isValid())
            return QPixmap();

        QPainter painter(&pm);
        renderer.render(&painter, QRectF(0, 0, targetSize.width(), targetSize.height()));
        painter.end();
        return pm;
    };

    {
        const QPixmap logoPx = renderAuthLogo();
        if (!logoPx.isNull())
            mauthLogoLabel->setPixmap(logoPx);
    }

    auto headerTextCol = new QVBoxLayout;
    headerTextCol->setSpacing(2);

    mauthTitle = new QLabel(tr("Вход в Trackly"), mauthCard);
    mauthTitle->setObjectName("AuthOverlayTitle");

    mauthSubtitle = new QLabel(tr("Введите email и пароль, чтобы продолжить."), mauthCard);
    mauthSubtitle->setObjectName("AuthOverlaySubtitle");
    mauthSubtitle->setWordWrap(true);

    headerTextCol->addWidget(mauthTitle);
    headerTextCol->addWidget(mauthSubtitle);

    mauthLangBtn = new QToolButton(mauthCard);
    mauthLangBtn->setObjectName("AuthLangButton");
    mauthLangBtn->setIconSize(QSize(18, 18));
    mauthLangBtn->setCursor(Qt::PointingHandCursor);
    mauthLangBtn->setToolTip(tr("Сменить язык"));
    mauthLangBtn->setAutoRaise(true);

    // Делаем кнопку строгим квадратом
    mauthLangBtn->setFixedSize(32, 32);
    mauthLangBtn->setStyleSheet("QToolButton#AuthLangButton { padding: 0; border: none; }");

    headerRow->addWidget(mauthLogoLabel, 0, Qt::AlignTop);
    headerRow->addLayout(headerTextCol, 1);
    headerRow->addWidget(mauthLangBtn, 0, Qt::AlignTop);

    mauthNameEdit = new QLineEdit(mauthCard);
    mauthNameEdit->setPlaceholderText(tr("Имя и фамилия"));
    mauthNameEdit->hide();

    mauthEmailEdit = new QLineEdit(mauthCard);
    mauthEmailEdit->setPlaceholderText(tr("E-mail..."));

    mauthPasswordEdit = new QLineEdit(mauthCard);
    mauthPasswordEdit->setPlaceholderText(tr("Пароль..."));
    mauthPasswordEdit->setEchoMode(QLineEdit::Password);

    auto consentHost = new QWidget(mauthCard);
    consentHost->setObjectName("AuthConsentHost");

    auto consentLayout = new QVBoxLayout(consentHost);
    consentLayout->setContentsMargins(0, 0, 0, 0);
    consentLayout->setSpacing(8);

    auto policyRow = new QHBoxLayout;
    policyRow->setContentsMargins(0, 0, 0, 0);
    policyRow->setSpacing(8);

    mauthPolicyCheckbox = new QCheckBox(consentHost);
    mauthPolicyCheckbox->setObjectName("AuthConsentCheckbox");

    mauthPolicyLinksLabel = new QLabel(consentHost);
    mauthPolicyLinksLabel->setObjectName("AuthConsentText");
    mauthPolicyLinksLabel->setWordWrap(true);
    mauthPolicyLinksLabel->setTextFormat(Qt::RichText);
    mauthPolicyLinksLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    mauthPolicyLinksLabel->setOpenExternalLinks(true);

    policyRow->addWidget(mauthPolicyCheckbox, 0, Qt::AlignTop);
    policyRow->addWidget(mauthPolicyLinksLabel, 1);

    auto personalRow = new QHBoxLayout;
    personalRow->setContentsMargins(0, 0, 0, 0);
    personalRow->setSpacing(8);

    mauthPersonalDataCheckbox = new QCheckBox(consentHost);
    mauthPersonalDataCheckbox->setObjectName("AuthConsentCheckbox");

    mauthPersonalDataLinkLabel = new QLabel(consentHost);
    mauthPersonalDataLinkLabel->setObjectName("AuthConsentText");
    mauthPersonalDataLinkLabel->setWordWrap(true);
    mauthPersonalDataLinkLabel->setTextFormat(Qt::RichText);
    mauthPersonalDataLinkLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    mauthPersonalDataLinkLabel->setOpenExternalLinks(true);

    personalRow->addWidget(mauthPersonalDataCheckbox, 0, Qt::AlignTop);
    personalRow->addWidget(mauthPersonalDataLinkLabel, 1);

    consentLayout->addLayout(policyRow);
    consentLayout->addLayout(personalRow);
    consentHost->hide();

    mauthSubmit = new QPushButton(tr("Войти"), mauthCard);
    mauthSubmit->setObjectName("PrimaryButton");

    mauthSwitch = new QPushButton(tr("Создать аккаунт"), mauthCard);
    mauthSwitch->setObjectName("SecondaryButton");

    cardLayout->addLayout(headerRow);
    cardLayout->addSpacing(8);
    cardLayout->addWidget(mauthNameEdit);
    cardLayout->addWidget(mauthEmailEdit);
    cardLayout->addWidget(mauthPasswordEdit);
    cardLayout->addWidget(consentHost);
    cardLayout->addSpacing(8);
    cardLayout->addWidget(mauthSubmit);
    cardLayout->addWidget(mauthSwitch);

    row->addWidget(mauthCard);
    row->addStretch();

    overlayLayout->addLayout(row);
    overlayLayout->addStretch();

    mauthOverlay->setGeometry(m_central->rect());

    const bool dark = isDarkTheme();
    // в тёмной теме — светлая, в светлой — почти чёрная
    const QColor iconColor = dark ? QColor("#F3EEFF") : QColor("#111827");
    mauthLangBtn->setIcon(makeTintedIcon(":/icons/language.svg", iconColor, QSize(18, 18)));

    connect(mauthLangBtn, &QToolButton::clicked, this, [this, renderAuthLogo]() {
        const QString now = m_core->language();

        if (now == QStringLiteral("ru"))
            m_core->applyLanguage(QStringLiteral("be"));
        else if (now == QStringLiteral("be"))
            m_core->applyLanguage(QStringLiteral("be-Latn"));
        else if (now == QStringLiteral("be-Latn"))
            m_core->applyLanguage(QStringLiteral("en"));
        else
            m_core->applyLanguage(QStringLiteral("ru"));

        retranslateUi();

        const bool darkNow = isDarkTheme();
        const QColor iconColorNow = darkNow ? QColor("#F3EEFF") : QColor("#2B2144");
        if (mauthLangBtn)
            mauthLangBtn->setIcon(makeTintedIcon(":/icons/language.svg", iconColorNow, QSize(18, 18)));

        if (mauthLogoLabel) {
            const QPixmap logoPx = renderAuthLogo();
            if (!logoPx.isNull())
                mauthLogoLabel->setPixmap(logoPx);
        }
    });

    connect(mauthSwitch, &QPushButton::clicked, this, [this, consentHost]() {
        setAuthRegisterMode(!mauthRegisterMode);
        consentHost->setVisible(mauthRegisterMode);
    });

    connect(mauthSubmit, &QPushButton::clicked, this, [this]() {
        const QString email = mauthEmailEdit ? mauthEmailEdit->text().trimmed() : QString();
        const QString password = mauthPasswordEdit ? mauthPasswordEdit->text() : QString();

        if (email.isEmpty() || password.isEmpty()) {
            showToast(m_central, tr("Заполните email и пароль."), ToastKind::Error, 2600);
            return;
        }

        if (mauthRegisterMode) {
            const QString fullName = mauthNameEdit ? mauthNameEdit->text().trimmed() : QString();
            if (fullName.isEmpty()) {
                showToast(m_central, tr("Укажите имя."), ToastKind::Error, 2600);
                return;
            }

            if (!mauthPolicyCheckbox || !mauthPolicyCheckbox->isChecked()) {
                showToast(m_central, tr("Подтвердите принятие политики конфиденциальности и условий использования."), ToastKind::Error, 3200);
                return;
            }

            if (!mauthPersonalDataCheckbox || !mauthPersonalDataCheckbox->isChecked()) {
                showToast(m_central, tr("Подтвердите согласие на обработку персональных данных."), ToastKind::Error, 3200);
                return;
            }

            m_core->signUp(email, password, fullName);
            return;
        }

        m_core->signIn(email, password);
    });

    connect(m_core, &AppCore::authSucceeded, this, [this]() {
        if (mauthRegisterMode) {
            QJsonObject payload{
                { "id", m_core->currentUserId() },
                { "email", m_core->currentEmail() },
                { "fullname", mauthNameEdit ? mauthNameEdit->text().trimmed() : QString() },
                { "accepted_privacy_policy", true },
                { "accepted_terms_of_service", true },
                { "accepted_personal_data_processing", true },
                { "consent_accepted_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate) }
            };
            m_core->updateProfile(payload);
        }

        hideAuthOverlay();
        showToast(m_central, tr("Добро пожаловать в Trackly."), ToastKind::Success, 2200);
        m_core->fetchDashboard();
        m_core->fetchProfile();
        m_core->fetchTriggers();
        navigateTo(0, tr("Главная"));
    });

    connect(m_core, &AppCore::authFailed, this, [this](const QString& e) {
        showToast(m_central, prettifyError(e), ToastKind::Error, 4200);
    });

    setAuthRegisterMode(false);
    consentHost->setVisible(false);

    mauthPolicyLinksLabel->setText(
        tr("Я принимаю <a href=\"https://tracklyapp.xyz/privacy.html\">Политику конфиденциальности</a> и "
           "<a href=\"https://tracklyapp.xyz/terms.html\">Условия использования</a>."));

    mauthPersonalDataLinkLabel->setText(
        tr("Я даю <a href=\"https://tracklyapp.xyz/consent.html\">согласие на обработку персональных данных</a>."));
}

void MainWindow::updateAuthLogo()
{
    if (!mauthLogoLabel)
        return;

    const QString path = isDarkTheme()
                             ? QStringLiteral(":/icons/logo-trackly-dark.svg")
                             : QStringLiteral(":/icons/logo-trackly-light.svg");

    const QSize targetSize(56, 56);
    QPixmap pm(targetSize);
    pm.fill(Qt::transparent);

    QSvgRenderer renderer(path);
    if (!renderer.isValid()) {
        mauthLogoLabel->clear();
        return;
    }

    QPainter painter(&pm);
    renderer.render(&painter, QRectF(0, 0, targetSize.width(), targetSize.height()));
    painter.end();

    mauthLogoLabel->setPixmap(pm);
}

void MainWindow::showAuthOverlay(bool forceLogin)
{
    Q_UNUSED(forceLogin);
    mauthOverlay->setGeometry(m_central->rect());
    mauthOverlay->show();
    mauthOverlay->raise();

    m_stack->setEnabled(false);
    m_sidebar->setEnabled(false);
    m_helpButton->setEnabled(false);

    setAuthRegisterMode(false);
    mauthEmailEdit->clear();
    mauthPasswordEdit->clear();
    mauthNameEdit->clear();
    mauthEmailEdit->setFocus();

    applyBlur(true);
}

void MainWindow::hideAuthOverlay()
{
    mauthOverlay->hide();
    m_stack->setEnabled(true);
    m_sidebar->setEnabled(true);
    m_helpButton->setEnabled(true);
    applyBlur(false);
}

void MainWindow::setAuthRegisterMode(bool enabled)
{
    mauthRegisterMode = enabled;

    if (mauthNameEdit) {
        mauthNameEdit->setVisible(enabled);
        mauthNameEdit->setEnabled(enabled);   // <-- ВАЖНО
    }

    if (mauthPolicyCheckbox)
        mauthPolicyCheckbox->setChecked(false);
    if (mauthPersonalDataCheckbox)
        mauthPersonalDataCheckbox->setChecked(false);

    if (auto consentHost = mauthPolicyCheckbox ? mauthPolicyCheckbox->parentWidget() : nullptr)
        consentHost->setVisible(enabled);

    if (mauthTitle)
        mauthTitle->setText(enabled ? tr("Создание аккаунта Trackly") : tr("Вход в Trackly"));

    if (mauthSubtitle)
        mauthSubtitle->setText(enabled
                                   ? tr("Присоединяйся и отслеживай своё состояние и прогресс лечения.")
                                   : tr("Возвращайся и отслеживай свое здоровье. Мы всегда здесь."));

    if (mauthSubmit)
        mauthSubmit->setText(enabled ? tr("Зарегистрироваться") : tr("Войти"));

    if (mauthSwitch)
        mauthSwitch->setText(enabled ? tr("У меня уже есть аккаунт") : tr("Создать аккаунт"));
}

void MainWindow::setAuthBusy(bool busy)
{
    mauthBusy = busy;

    if (mauthSubmit) mauthSubmit->setEnabled(!busy);
    if (mauthSwitch) mauthSwitch->setEnabled(!busy);
    if (mauthEmailEdit) mauthEmailEdit->setEnabled(!busy);
    if (mauthPasswordEdit) mauthPasswordEdit->setEnabled(!busy);
    if (mauthNameEdit) mauthNameEdit->setEnabled(mauthRegisterMode && !busy);

    if (!mauthStatusLabel)
        return;

    if (busy) {
        mauthStatusLabel->setText(tr("Подождите, проверяем данные..."));
        mauthStatusLabel->show();

        auto effect = new QGraphicsOpacityEffect(mauthStatusLabel);
        mauthStatusLabel->setGraphicsEffect(effect);

        auto anim = new QPropertyAnimation(effect, "opacity", mauthStatusLabel);
        anim->setDuration(800);
        anim->setStartValue(0.3);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::InOutSine);
        anim->setLoopCount(-1);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        mauthStatusLabel->hide();
        if (auto eff = mauthStatusLabel->graphicsEffect()) {
            eff->deleteLater();
            mauthStatusLabel->setGraphicsEffect(nullptr);
        }
    }
}

void MainWindow::applyBlur(bool enabled)
{
    auto applyToWidget = [enabled](QWidget* w) {
        if (!w) return;

        if (!enabled) {
            if (auto* old = w->graphicsEffect()) {
                old->setEnabled(false);
                old->deleteLater();
            }
            w->setGraphicsEffect(nullptr);
            return;
        }

        auto* effect = qobject_cast<QGraphicsBlurEffect*>(w->graphicsEffect());
        if (!effect) {
            effect = new QGraphicsBlurEffect(w);
            effect->setBlurRadius(18.0);
            w->setGraphicsEffect(effect);
        }
        effect->setEnabled(true);
    };

    applyToWidget(m_sidebar);
    applyToWidget(m_content);

    if (m_helpOverlay) m_helpOverlay->raise();
    if (mauthOverlay) mauthOverlay->raise();
}

void MainWindow::wire()
{
    connect(m_helpButton, &QToolButton::clicked, this, &MainWindow::showHelpOverlay);

    connect(m_navHome, &QPushButton::clicked, this, [this]() {
        navigateTo(0, tr("Главная"));
    });
    connect(m_navSeizures, &QPushButton::clicked, this, [this]() {
        navigateTo(1, tr("Приступы"));
    });
    connect(m_navMeds, &QPushButton::clicked, this, [this]() {
        navigateTo(2, tr("Лекарства"));
    });
    connect(m_navExams, &QPushButton::clicked, this, [this]() {
        navigateTo(3, tr("Обследования"));
    });
    connect(m_navAnalytics, &QPushButton::clicked, this, [this]() {
        navigateTo(4, tr("Аналитика"));
        refreshAnalyticsPage();
    });
    connect(m_navTriggers, &QPushButton::clicked, this, [this]() {
        navigateTo(5, tr("Самочувствие"));
    });
    connect(m_navProfile, &QPushButton::clicked, this, [this]() {
        navigateTo(6, tr("Профиль"));
        m_core->fetchProfile();
    });

    connect(m_themeBtn, &QPushButton::clicked, this, [this]() {
        AppCore::ThemeMode next = AppCore::ThemeMode::Light;
        if (m_core->themeMode() == AppCore::ThemeMode::Light)
            next = AppCore::ThemeMode::Dark;
        else if (m_core->themeMode() == AppCore::ThemeMode::Dark)
            next = AppCore::ThemeMode::Auto;

        m_core->applyTheme(next);
        const bool dark = next == AppCore::ThemeMode::Dark;
        qApp->setStyleSheet(Styles::buildStylesheet(dark));
        updateThemeIcon();
        updateIcons();
    });

    connect(m_core, &AppCore::medicationSchedulesLoaded,
            this, [this](const QJsonArray& arr) {
                m_cachedMedicationItems = arr;
                refreshDayMedicationSummary();
            });

    connect(m_core, &AppCore::medicationLogsReady,
            this, [this](const QJsonArray& arr) {
                m_cachedMedicationLogs = arr;
                refreshDayMedicationSummary();
            });
    connect(m_core, &AppCore::saveSucceeded,
            this, [this](const QString& key) {
                if (key == "medication_log" || key == "medication_log_cancelled") {
                    m_core->fetchMedicationLogs();
                }
            });

    connect(m_core, &AppCore::medicationLogCancelled,
            this, [this](const QString&) {
                m_core->fetchMedicationLogs();
            });

    connect(m_languageBtn, &QPushButton::clicked, this, [this]() {
        const QString now = m_core->language();
        if (now == "ru") m_core->applyLanguage("be");
        else if (now == "be") m_core->applyLanguage("be-Latn");
        else if (now == "be-Latn") m_core->applyLanguage("en");
        else m_core->applyLanguage("ru");
        retranslateUi();
    });

    connect(m_core, &AppCore::attachmentsReady,
            this, &MainWindow::handleAttachmentsReady);

    connect(m_core, &AppCore::examinationCreated,
        this, [this](const QJsonObject& exam) {
            const QString examId = exam.value("id").toString();
            if (examId.isEmpty()) return;
               // 🔥 Генерируем нормальные имена для файлов
            int fileIndex = 1;
             for (const QString& path : std::as_const(m_pendingExamFilesForNewExam)) {
                  // Получаем расширение
                QString extension = QFileInfo(path).suffix().toLower();
                if (!extension.isEmpty()) {
                     extension = "." + extension;
                }

                    // Формируем красивое имя: Тип_Подтип_Дата_Номер.расширение
                QString typeNorm = m_pendingExamType.toLower().replace(' ', '_');
                QString subtypeNorm = m_pendingExamSubtype.toLower().replace(' ', '_');
                if (subtypeNorm.isEmpty()) subtypeNorm = "general";

                QString dateStr = m_pendingExamDate.toString("yyyy-MM-dd");

                QString baseName = QString("%1_%2_%3_%4")
                    .arg(typeNorm)
                    .arg(subtypeNorm)
                    .arg(dateStr)
                    .arg(fileIndex++);

                // Ограничиваем длину
                if (baseName.length() > 80) {
                    baseName = baseName.left(80);
                }

                QString fileName = baseName + extension;
                QString storagePath = QString("%1/exams/%2/%3")
                    .arg(m_core->currentUserId(), examId, fileName);

                    m_core->uploadAttachment(path, storagePath, examId);
                }

                m_pendingExamFilesForNewExam.clear();
                m_pendingExamFiles.clear();
                m_pendingExamType.clear();
                m_pendingExamSubtype.clear();

                if (m_examDropArea)
                    m_examDropArea->setFiles({});
            });

    connect(mauthSubmit, &QPushButton::clicked, this, [this] {
        if (mauthRegisterMode) {
            if (mauthAgreeCheckbox && !mauthAgreeCheckbox->isChecked()) {
                showToast(
                    m_central,
                    tr("Отметьте согласие с политикой перед регистрацией."),
                    ToastKind::Error,
                    2800
                    );
                return;
            }
        }

        setAuthBusy(true);

        if (mauthRegisterMode) {
            m_core->signUp(
                mauthEmailEdit->text(),
                mauthPasswordEdit->text(),
                mauthNameEdit->text()
                );
        } else {
            m_core->signIn(
                mauthEmailEdit->text(),
                mauthPasswordEdit->text()
                );
        }
    });


    connect(m_core, &AppCore::authSucceeded, this, [this] {
        setAuthBusy(false);
        hideAuthOverlay();
        showToast(m_central, tr("Добро пожаловать в Trackly!"),
                          ToastKind::Success, 2200);
    });

    connect(m_core, &AppCore::medicationSchedulesLoaded,
            this, [this](const QJsonArray& arr) {
                m_cachedMedicationItems = arr;
                refreshDayMedicationSummary();
            });
    connect(m_core, &AppCore::medicationLogsReady,
            this, [this](const QJsonArray&) {
                refreshTodayIntakesList();
            });

    connect(m_core, &AppCore::saveSucceeded,
            this, [this](const QString& key) {
                if (key == "medication_log" || key == "medication_log_cancelled") {
                    m_core->fetchMedicationLogs();
                }
            });

    connect(m_core, &AppCore::medicationLogCancelled,
            this, [this](const QString&) {
                m_core->fetchMedicationLogs();
            });

    connect(m_core, &AppCore::medicationCreated,
            this, [this](const QJsonObject& med) {
                createMedicationSchedulesFor(med);
            });

    connect(m_core, &AppCore::authFailed, this, [this](const QString& e) {
        setAuthBusy(false);
        showToast(m_central, prettifyError(e),
                          ToastKind::Error, 4200);
    });

    connect(m_core, &AppCore::medicationSchedulesLoaded,
            this, [this](const QJsonArray& arr) {
                m_cachedMedicationItems = arr;
                refreshDayMedicationSummary();
            });

    connect(m_core, &AppCore::dashboardReady, this, &MainWindow::updateDashboard);
    connect(m_core, &AppCore::seizuresReady, this, &MainWindow::fillSeizures);
    connect(m_core, &AppCore::medicationsReady, this, &MainWindow::fillMedications);
    connect(m_core, &AppCore::examinationsReady, this, &MainWindow::fillExaminations);
    connect(m_core, &AppCore::profileReady, this, &MainWindow::fillProfile);
    connect(m_core, &AppCore::medicationLogsReady, this, &MainWindow::fillMedicationLogs);
    connect(m_core, &AppCore::triggersReady, this, &MainWindow::fillTriggers);

    connect(m_core, &AppCore::saveSucceeded, this, [this](const QString& key) {
        showToast(m_central, successMessageFor(key), ToastKind::Success, 2600);
        m_core->fetchDashboard();
        m_core->fetchProfile();
        m_core->fetchTriggers();
        refreshDayMedicationSummary();
    });

    connect(m_core, &AppCore::networkError,
            this, [this](const QString& raw) {
                qDebug() << "Supabase error raw:" << raw;   // <‑‑ добавить эту строку

                showToast(m_central,
                                  prettifyError(raw),
                                  ToastKind::Error,
                                  4000);
            });

    connect(m_core, &AppCore::signedOut, this, [this]() {
        navigateTo(0, tr("Главная"));
        showAuthOverlay(true);
    });

        // После того как настроены все соединения, добавь:
    connect(m_core, &AppCore::attachmentsReady, this, [this](const QJsonArray& attachments) {
        m_cachedAttachments = attachments;
           // 🔥 Перерисовываем обследования при загрузке аттачей
        fillExaminations(m_cachedExaminations);
    });

    connect(m_core, &AppCore::medicationLogCancelled,
            this, [this](const QString&) {
                refreshDayMedicationSummary();
            });

    connect(m_core, &AppCore::medicationsReady,
            this, &MainWindow::onMedicationsReady);

    if (!m_core->hasSavedSession() || !m_core->isAuthenticated())
        showAuthOverlay(true);
}

void MainWindow::retranslateUi()
{
    if (m_title && m_stack && m_stack->currentIndex() == 0) {
        m_title->setText(tr("Дашборд"));
    }

    if (m_navHome) m_navHome->setText(tr("Главная"));
    if (m_navSeizures) m_navSeizures->setText(tr("Приступы"));
    if (m_navMeds) m_navMeds->setText(tr("Лекарства"));
    if (m_navExams) m_navExams->setText(tr("Обследования"));
    if (m_navAnalytics) m_navAnalytics->setText(tr("Аналитика"));
    if (m_navTriggers) m_navTriggers->setText(tr("Триггеры"));
    if (m_navProfile) m_navProfile->setText(tr("Профиль"));
    if (m_themeBtn) m_themeBtn->setText(tr("Тема"));
    if (m_languageBtn) m_languageBtn->setText(tr("Язык"));
    if (m_navAnalytics) m_navAnalytics->setText(tr("Аналитика"));
    if (m_navTriggers) m_navTriggers->setText(tr("Самочувствие"));

    updateSidebarUi();
    refreshAnalyticsPage();
    refreshTriggersList();
}

void MainWindow::updateDashboard(const QJsonObject& obj)
{
    if (m_summary1) {
        m_summary1->setText(tr("%1 за текущий месяц").arg(obj.value("seizures_this_month").toInt()));
    }

    if (m_summary2) {
        m_summary2->setText(tr("%1 приемов сегодня").arg(obj.value("meds_today").toInt()));
    }

    if (m_summary3) {
        const QString nextExam = obj.value("next_exam").toString();
        m_summary3->setText(nextExam.isEmpty() ? tr("Нет ближайших записей") : nextExam);
    }

    if (m_summary4) {
        const QString doctor = obj.value("doctor").toString();
        m_summary4->setText(doctor.isEmpty() ? tr("Не указан") : doctor);
    }
}

void MainWindow::fillSeizures(const QJsonArray& arr)
{
    m_cachedSeizureItems = arr;
    m_datesWithSeizures.clear();

    QWidget* page = m_stack->widget(1);
    QWidget* host = page->findChild<QWidget*>("SeizureFeedHost");
    auto* layout = host ? host->findChild<QVBoxLayout*>("SeizureFeedLayout") : nullptr;
    if (!layout) return;

    clearLayout(layout);

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
        cl->setSpacing(8);

        auto* title = new QLabel(o.value("seizure_type").toString(), card);
        title->setObjectName("CardTitle");

        auto* meta = new QLabel(
            formatTs(o.value("started_at").toString()) + " · " + o.value("place").toString(),
            card);
        meta->setObjectName("MutedLabel");

        auto* severity = new QLabel(tr("Тяжесть: %1").arg(o.value("severity").toInt()), card);
        severity->setObjectName("BadgeLabel");

        auto* note = new QLabel(o.value("notes").toString(), card);
        note->setWordWrap(true);

        cl->addWidget(title);
        cl->addWidget(meta);
        cl->addWidget(severity, 0, Qt::AlignLeft);
        cl->addWidget(note);

        layout->addWidget(card);
    }

    layout->addStretch();
    updateDashboardCalendarDecorations();
    refreshAnalyticsPage();
}

void MainWindow::fillMedications(const QJsonArray& arr)
{
    m_cachedMedications = arr;
    m_datesWithMedication.clear();

    clearLayout(m_medCardsLayout);

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
void MainWindow::showIntakesForMedicationToday(const QString& medId)
{
    Q_UNUSED(medId);

    if (!m_medsTab || !m_intakesDateEdit)
        return;

    m_intakesDateEdit->setDate(QDate::currentDate());
    m_medsTab->setCurrentIndex(1); // вкладка "Приёмы"
    refreshTodayIntakesList();
}


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

void MainWindow::openPrnIntakeDialog(const QJsonObject& med)
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Приём по необходимости"));
    dlg.setModal(true);

    auto* layout = new QVBoxLayout(&dlg);
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

    if (dlg.exec() != QDialog::Accepted)
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
void MainWindow::refreshTodayIntakesList()
{
    if (!m_todayIntakesLayout || !m_intakesDateEdit)
        return;

    clearLayoutHelper(m_todayIntakesLayout);

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
    const QString timeStr   = schedule.value("intake_time").toString();
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

void MainWindow::startMedicationReminderLoop()
{
    if (!m_medReminderTimer) {
        m_medReminderTimer = new QTimer(this);
        connect(m_medReminderTimer, &QTimer::timeout,
                this, &MainWindow::checkMedicationReminders);
        m_medReminderTimer->start(60'000); // раз в минуту
    }
}

void MainWindow::checkMedicationReminders()
{
    if (!m_trayIcon)
        return;

    const QDate today = QDate::currentDate();
    const QTime now   = QTime::currentTime();

    const QJsonArray meds      = m_core->medicationsJson();
    const QJsonArray schedules = m_cachedMedicationItems;

    for (const auto& sv : schedules) {
        const QJsonObject s = sv.toObject();

        const QString timeStr = s.value("intake_time").toString();
        QTime t = QTime::fromString(timeStr, "HH:mm");
        if (!t.isValid())
            continue;

        // сравниваем только часы+минуты
        if (now.hour() == t.hour() && now.minute() == t.minute()) {
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

            if (!medName.isEmpty()) {
                m_trayIcon->showMessage(
                    tr("Пора принять лекарство"),
                    tr("%1 — %2").arg(medName, timeStr),
                    QSystemTrayIcon::Information,
                    10000);
            }
        }
    }
}



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

    QMenu menu(row);
    QAction* markTaken  = menu.addAction(tr("Отметить принятым"));
    QAction* markMissed = menu.addAction(tr("Отметить пропущенным"));
    QAction* deleteAct  = nullptr;

    if (isPrn) {
        menu.addSeparator();
        deleteAct = menu.addAction(tr("Удалить приём"));
    }

    QAction* chosen = menu.exec(row->mapToGlobal(pos));
    if (!chosen)
        return;

    // Удаление PRN
    if (chosen == deleteAct && isPrn && !logId.isEmpty()) {
        m_core->deleteMedicationLog(logId);
        return;
    }

    auto createLog = [this, medId, schedId](const QString& newStatus) {
        QJsonObject log;
        log["user_id"]       = m_core->currentUserId();
        log["medication_id"] = medId;
        if (!schedId.isEmpty())
            log["schedule_id"] = schedId;
        else
            log["schedule_id"] = QJsonValue(); // PRN

        log["taken_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        log["status"]   = newStatus;

        m_core->createMedicationLog(log);
    };

    // Тоггл логики:
    // если уже taken и ты снова жмёшь "Отметить принятым" → удалить лог (вернуть нейтральное)
    // аналогично для missed
    if (chosen == markTaken) {
        if (status == "taken" && !logId.isEmpty()) {
            m_core->deleteMedicationLog(logId);
        } else {
            if (!logId.isEmpty())
                m_core->deleteMedicationLog(logId); // отменяем прошлый статус (missed)
            createLog("taken");
        }
    } else if (chosen == markMissed) {
        if (status == "missed" && !logId.isEmpty()) {
            m_core->deleteMedicationLog(logId);
        } else {
            if (!logId.isEmpty())
                m_core->deleteMedicationLog(logId); // отменяем прошлый статус (taken)
            createLog("missed");
        }
    }
}

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

    auto* layout = new QVBoxLayout(&dlg);
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

    if (dlg.exec() != QDialog::Accepted)
        return;

    // 4. Собираем новое время: тот же день, другое время
    dt.setTime(timeEdit->time());

    // 5. Просим AppCore обновить taken_at
    m_core->updateMedicationLogTime(m_contextMenuLogId, dt);
}

void MainWindow::handleDeletePrnIntake()
{
    if (m_contextMenuLogId.isEmpty())
        return;

    m_core->deleteMedicationLog(m_contextMenuLogId);
}

void MainWindow::refreshDayMedicationSummary()
{
    if (!m_dayMedicationLayout || !m_medCalendar)
        return;

    clearLayoutHelper(m_dayMedicationLayout);

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

        const QString schedId = s.value("id").toString();
        const QJsonObject log = logBySchedule.value(schedId);

        const QString timeStr = s.value("intake_time").toString();
        QTime t = QTime::fromString(timeStr, "HH:mm");
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



void MainWindow::cancelMedicationIntake()
{
    if (m_contextMenuLogId.isEmpty())
        return;

    m_core->cancelMedicationLog(m_contextMenuLogId); // реализуешь в AppCore
    m_contextMenuLogId.clear();
}



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

void MainWindow::pickAvatar()
{
    const QString file = QFileDialog::getOpenFileName(
        this, tr("Выберите фото"), QString(),
        tr("Images (*.png *.jpg *.jpeg *.webp)"));

    if (file.isEmpty()) return;

    QPixmap px(file);
    if (!px.isNull()) {
        m_avatarPreview->setPixmap(px.scaled(132, 132, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    }
}

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

    clearLayout(m_pastExamLayout);
    clearLayout(m_plannedExamLayout);

    const QDate today = QDate::currentDate();

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

        const bool isPast = dt.isValid() ? (dt.date() <= today) : false;

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
                const QString url = QStringLiteral("https://your-project-ref.supabase.co")
                + "/storage/v1/object/public/medical_files/"
                    + storagePath;
                QDesktopServices::openUrl(QUrl(url));
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
}

void MainWindow::fillProfile(const QJsonObject& obj)
{
    if (!m_profileFullName) return;

    const QString email = obj.value("email").toString().trimmed().isEmpty()
                              ? m_core->currentEmail()
                              : obj.value("email").toString().trimmed();

    m_cachedProfile = obj;
    m_profileFullName->setText(obj.value("full_name").toString().trimmed());
    m_profileBirthDate->setText(obj.value("birth_date").toString());

    const QString gender = obj.value("gender").toString().trimmed();
    m_profileGender->setCurrentText(gender.isEmpty() ? tr("Не указан") : gender);

    m_profilePhone->setText(obj.value("phone").toString());
    m_profileEmail->setText(email);
    m_profileAddress->setText(obj.value("address").toString());
    m_profileEmergency->setText(obj.value("emergency_phone").toString());
    m_profileDoctor->setText(obj.value("doctor_name").toString());
    m_profileDiagnosis->setPlainText(obj.value("diagnosis").toString());

    refreshAnalyticsPage();
}

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


void MainWindow::refreshTriggersList()
{
    if (!m_triggerListLayout || !m_triggerListHost) {
        return;
    }

    clearLayout(m_triggerListLayout);

    if (m_cachedTriggers.isEmpty()) {
        auto* empty = new QLabel(tr("После добавления записей здесь появится история самочувствия."), m_triggerListHost);
        empty->setObjectName("MutedLabel");
        empty->setWordWrap(true);
        m_triggerListLayout->addWidget(empty);
        m_triggerListLayout->addStretch();
        return;
    }

    for (const auto& v : m_cachedTriggers) {
        const QJsonObject o = v.toObject();

        auto* card = new QFrame(m_triggerListHost);
        card->setObjectName("FeedCard");

        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(16, 16, 16, 16);
        cl->setSpacing(8);

        const int mood = o.value("mood").toInt();
        const double sleepHours = o.value("sleep_hours").toDouble();
        const int sleepQuality = o.value("sleep_quality").toInt();

        auto* title = new QLabel(o.value("trigger_title").toString().trimmed(), card);
        title->setObjectName("CardTitle");

        auto* meta = new QLabel(
            tr("%1 · Настрой %2/5 · Сон %3 ч · Качество сна %4/5")
                .arg(formatTs(o.value("logged_at").toString()))
                .arg(mood)
                .arg(QString::number(sleepHours, 'f', 1))
                .arg(sleepQuality),
            card
            );
        meta->setObjectName("MutedLabel");
        meta->setWordWrap(true);

        auto* kind = new QLabel(o.value("trigger_kind").toString().trimmed(), card);
        kind->setObjectName("BadgeLabel");

        QStringList tags;
        const auto arrTags = o.value("triggers").toArray();
        for (const auto& tv : arrTags) {
            const QString s = tv.toString().trimmed();
            if (!s.isEmpty()) {
                tags << s;
            }
        }

        if (!tags.isEmpty()) {
            auto* tagsLabel = new QLabel(tr("Триггеры: %1").arg(tags.join(", ")), card);
            tagsLabel->setObjectName("MutedLabel");
            tagsLabel->setWordWrap(true);
            cl->addWidget(title);
            cl->addWidget(meta);
            cl->addWidget(kind, 0, Qt::AlignLeft);
            cl->addWidget(tagsLabel);

            const QString noteText = o.value("note").toString().trimmed();
            if (!noteText.isEmpty()) {
                auto* note = new QLabel(noteText, card);
                note->setWordWrap(true);
                cl->addWidget(note);
            }
        } else {
            cl->addWidget(title);
            cl->addWidget(meta);
            cl->addWidget(kind, 0, Qt::AlignLeft);

            const QString noteText = o.value("note").toString().trimmed();
            if (!noteText.isEmpty()) {
                auto* note = new QLabel(noteText, card);
                note->setWordWrap(true);
                cl->addWidget(note);
            }
        }

        auto* deleteBtn = new QPushButton(tr("Удалить"), card);
        deleteBtn->setObjectName("SecondaryButton");
        connect(deleteBtn, &QPushButton::clicked, this, [this, o]() {
            const QString id = o.value("id").toString();
            if (!id.isEmpty()) {
                m_core->deleteTrigger(id);
            }
        });

        cl->addSpacing(4);
        cl->addWidget(deleteBtn, 0, Qt::AlignRight);

        m_triggerListLayout->addWidget(card);
    }

    m_triggerListLayout->addStretch();
}

void MainWindow::deleteTrigger(const QJsonObject& trigger)
{
    const QString id = trigger.value("id").toString();
    if (id.isEmpty())
        return;

    const QString title = trigger.value("title").toString().trimmed().isEmpty()
                              ? trigger.value("kind").toString()
                              : trigger.value("title").toString();

    const auto res = QMessageBox::question(
        this,
        tr("Удаление"),
        tr("Удалить запись \"%1\"?").arg(title),
        QMessageBox::Yes | QMessageBox::No
        );

    if (res == QMessageBox::Yes)
        m_core->deleteTrigger(id);
}




void MainWindow::submitTrigger()
{
    const QString title = m_triggerTitle ? m_triggerTitle->text().trimmed() : QString();
    const QString note = m_triggerNote ? m_triggerNote->toPlainText().trimmed() : QString();
    const QString tagsRaw = m_triggerTags ? m_triggerTags->text().trimmed() : QString();
    const QString kind = m_triggerKind ? m_triggerKind->currentText().trimmed() : QString();

    if (title.isEmpty()) {
        showToast(m_central, tr("Введите короткое название записи самочувствия"), ToastKind::Error, 2600);
        return;
    }

    if (m_sleepHours && m_sleepHours->value() <= 0.0) {
        showToast(m_central, tr("Укажите количество часов сна"), ToastKind::Error, 2600);
        return;
    }

    QJsonArray triggerArray;
    const QStringList parts = tagsRaw.split(',', Qt::SkipEmptyParts);
    for (QString part : parts) {
        part = part.trimmed();
        if (!part.isEmpty()) {
            triggerArray.append(part);
        }
    }

    QJsonObject payload{
        {"user_id", m_core->currentUserId()},
        {"logged_at", m_triggerDate->dateTime().toString(Qt::ISODate)},
        {"mood", m_moodScore->currentIndex() + 1},
        {"sleep_hours", m_sleepHours->value()},
        {"sleep_quality", m_sleepQuality->currentIndex() + 1},
        {"trigger_kind", kind.isEmpty() ? tr("Общее") : kind},
        {"trigger_title", title},
        {"note", note},
        {"triggers", triggerArray}
    };

    m_core->createTrigger(payload);

    m_triggerDate->setDateTime(QDateTime::currentDateTime());
    m_moodScore->setCurrentIndex(2);
    m_sleepHours->setValue(8.0);
    m_sleepQuality->setCurrentIndex(2);
    m_triggerKind->setCurrentIndex(0);
    m_triggerTitle->clear();
    m_triggerTags->clear();
    m_triggerNote->clear();
}

void MainWindow::fillTriggers(const QJsonArray& arr)
{
    m_cachedTriggers = arr;
    refreshTriggersList();
    refreshAnalyticsPage();
}



void MainWindow::openEmergencyCall()
{
    const QString phone = m_profileEmergency ? m_profileEmergency->text().trimmed() : QString();

    if (phone.isEmpty()) {
        QMessageBox::information(this, tr("Нет контакта"), tr("Укажите экстренный контакт в профиле."));
        return;
    }

    QMessageBox::information(this, tr("Экстренный звонок"), tr("Позвоните по номеру: %1").arg(phone));
}

void MainWindow::openEmergencySms()
{
    const QString phone = m_profileEmergency ? m_profileEmergency->text().trimmed() : QString();

    if (phone.isEmpty()) {
        QMessageBox::information(this, tr("Нет контакта"), tr("Укажите экстренный контакт в профиле."));
        return;
    }

    QMessageBox::information(
        this,
        tr("Экстренное SMS"),
        tr("Отправьте SMS на номер %1:\n\nУ пользователя эпилептический приступ. Нужна помощь.").arg(phone));
}

void MainWindow::openEmergencyEmail()
{
    const QString to = m_profileEmergency ? m_profileEmergency->text().trimmed() : QString();

    if (to.isEmpty()) {
        QMessageBox::information(
            this,
            tr("Нет email"),
            tr("Укажите email экстренного контакта в поле профиля."));
        return;
    }

    if (!to.contains('@')) {
        QMessageBox::warning(
            this,
            tr("Неверный email"),
            tr("Для отправки письма в поле экстренного контакта должен быть email."));
        return;
    }

    QString patientName = m_profileFullName ? m_profileFullName->text().trimmed() : QString();
    if (patientName.isEmpty()) patientName = tr("Пользователь Trackly");

    const QString text = tr("Возможен эпилептический приступ. "
                            "Проверьте состояние пользователя и свяжитесь с ним как можно скорее.");

    m_core->sendEmergencyEmail(to, patientName, text);
    showToast(m_central, tr("Запрос на отправку экстренного email отправлен."), ToastKind::Info, 2600);
}

void MainWindow::createPdfReport()
{
    QMessageBox::information(
        this,
        tr("Отчёт"),
        tr("Экспорт PDF пока реализован как заглушка. "
           "Сейчас можно собрать данные с дашборда, приступов, лекарств и обследований в следующем шаге."));
}

void MainWindow::updateDashboardCalendarDecorations()
{
    if (!m_dashboardCalendar && !m_medCalendar) return;

    auto decorate = [this](QCalendarWidget* cal) {
        if (!cal) return;

        QTextCharFormat base;
        cal->setDateTextFormat(QDate(), base);

        QTextCharFormat seizureFmt;
        seizureFmt.setBackground(QColor("#FDE2E7"));
        seizureFmt.setForeground(QColor("#7A2235"));

        QTextCharFormat medFmt;
        medFmt.setBackground(QColor("#E8E2FF"));
        medFmt.setForeground(QColor("#47308A"));

        QTextCharFormat examFmt;
        examFmt.setBackground(QColor("#E3F5EA"));
        examFmt.setForeground(QColor("#236B43"));

        QTextCharFormat mixedFmt;
        mixedFmt.setBackground(QColor("#FFE8C7"));
        mixedFmt.setForeground(QColor("#7A4B00"));

        QSet<QDate> allDates = m_datesWithSeizures;
        allDates.unite(m_datesWithMedication);
        allDates.unite(m_datesWithExams);

        for (const QDate& d : allDates) {
            const bool s = m_datesWithSeizures.contains(d);
            const bool m = m_datesWithMedication.contains(d);
            const bool e = m_datesWithExams.contains(d);

            if ((s && m) || (s && e) || (m && e)) cal->setDateTextFormat(d, mixedFmt);
            else if (s) cal->setDateTextFormat(d, seizureFmt);
            else if (m) cal->setDateTextFormat(d, medFmt);
            else if (e) cal->setDateTextFormat(d, examFmt);
        }
    };

    decorate(m_dashboardCalendar);
    decorate(m_medCalendar);
}

void MainWindow::refreshDashboardDayDetails()
{
    if (!m_dashboardCalendar || !m_dashboardExamLayout) return;
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }

    QMainWindow::changeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);

    if (m_helpOverlay) {
        m_helpOverlay->setGeometry(m_central->rect());
    }

    if (mauthOverlay) {
        mauthOverlay->setGeometry(m_central->rect());
    }

    if (s_toastHost && s_toastHost->parentWidget() == m_central) {
        const int width = 360;
        const int right = 18;
        const int top = 18;
        s_toastHost->move(m_central->width() - width - right, top);
        s_toastHost->resize(width, s_toastHost->height());
        s_toastHost->raise();
    }

    if (mauthOverlay && mauthLangBtn) {
        const int margin = 18;
        const int x = margin;
        const int y = mauthOverlay->height() - mauthLangBtn->height() - margin;
        mauthLangBtn->move(x, qMax(0, y));
    }
}
