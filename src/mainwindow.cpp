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
#include <QPageLayout>
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
    setWindowTitle("Trackly");

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
    m_trayIcon->setToolTip(tr("Trackly — напоминания о приёме"));
    m_trayIcon->show();

    // проверяем что иконка показалась
    qDebug() << "Tray icon visible:" << m_trayIcon->isVisible()
             << "supports messages:" << m_trayIcon->supportsMessages();

    m_core->fetchMedications();
    m_core->fetchMedicationSchedules();
    m_core->fetchMedicationLogs();
    m_core->fetchSeizures();
    m_core->fetchExaminations();
    m_core->fetchTriggers();

    m_core->fetchDashboard();
    m_core->fetchProfile();
    m_core->fetchAttachments();
    startMedicationReminderLoop();
    startWellbeingReminderLoop();

    connect(m_core, &AppCore::attachmentsReady,
            this, &MainWindow::handleAttachmentsReady);
}

QString MainWindow::formatTs(const QString& iso)
{
    return formatIsoToLocal(iso);
}

void MainWindow::clearLayout(QLayout* layout)
{
    ::clearLayout(layout);
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
    t->setProperty("origKey", title);  // для retranslateUi

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
    makeNav(m_navTriggers,  tr("Самочувствие"),   QStringLiteral(":/icons/warning.svg"));
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

    // Report buttons (visible on dashboard/analytics)
    m_topbarReportBtn = new QPushButton(tr("Отчёт"), bar);
    m_topbarReportBtn->setObjectName("SecondaryButton");
    m_topbarReportBtn->setCursor(Qt::PointingHandCursor);
    m_topbarReportBtn->setMinimumHeight(32);
    m_topbarReportBtn->hide();

    m_topbarExportPdfBtn = new QPushButton(tr("Экспорт PDF"), bar);
    m_topbarExportPdfBtn->setObjectName("SecondaryButton");
    m_topbarExportPdfBtn->setCursor(Qt::PointingHandCursor);
    m_topbarExportPdfBtn->setMinimumHeight(32);
    m_topbarExportPdfBtn->hide();

    m_topbarAiBtn = new QPushButton(tr("AI"), bar);
    m_topbarAiBtn->setObjectName("SecondaryButton");
    m_topbarAiBtn->setCursor(Qt::PointingHandCursor);
    m_topbarAiBtn->setToolTip(tr("AI-анализ (ChatGPT)"));
    m_topbarAiBtn->setMinimumHeight(32);
    m_topbarAiBtn->hide();

    m_helpButton = new QToolButton(bar);
    m_helpButton->setObjectName("HelpButton");
    m_helpButton->setIconSize(QSize(20, 20));
    m_helpButton->setMinimumHeight(32);
    m_helpButton->setToolTip(tr("Памятка при приступе"));

    layout->addWidget(m_title);
    layout->addStretch();
    layout->addWidget(m_topbarReportBtn);
    layout->addWidget(m_topbarExportPdfBtn);
    layout->addWidget(m_topbarAiBtn);
    layout->addWidget(m_helpButton);

    QObject::connect(m_topbarReportBtn, &QPushButton::clicked, this, &MainWindow::createPdfReport);
    QObject::connect(m_topbarExportPdfBtn, &QPushButton::clicked, this, &MainWindow::showReportDialog);
    QObject::connect(m_topbarAiBtn, &QPushButton::clicked, this, &MainWindow::showAiAnalyticsDialog);

    return bar;
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

    // Topbar report buttons visibility
    if (m_topbarReportBtn)
        m_topbarReportBtn->setVisible(index == 0);
    if (m_topbarExportPdfBtn)
        m_topbarExportPdfBtn->setVisible(index == 4);
    if (m_topbarAiBtn)
        m_topbarAiBtn->setVisible(index == 4);

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
        const QString text = btn->text();
        if (!text.isEmpty()) {
            btn->setProperty("fullText", text);
            btn->setToolTip(text);
        }
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
    m_helpCard->setFixedWidth(480);

    auto* cardLayout = new QVBoxLayout(m_helpCard);
    cardLayout->setContentsMargins(28, 24, 28, 20);
    cardLayout->setSpacing(0);

    // ── Title ──
    auto* title = new QLabel(tr("Памятка при приступе"), m_helpCard);
    title->setObjectName("SectionTitle");

    cardLayout->addWidget(title);

    auto* subtitle = new QLabel(tr("Что делать, если у человека рядом с вами начался приступ"), m_helpCard);
    subtitle->setObjectName("MutedLabel");
    subtitle->setWordWrap(true);

    cardLayout->addWidget(subtitle);

    // ── Steps ──
    auto makeStep = [&](int num, const QString& text) {
        auto* stepRow = new QHBoxLayout;
        stepRow->setSpacing(12);
        stepRow->setContentsMargins(0, 0, 0, 0);

        auto* circle = new QLabel(QString::number(num), m_helpCard);
        circle->setFixedSize(26, 26);
        circle->setAlignment(Qt::AlignCenter);
        circle->setStyleSheet(
            "QLabel { background:#7C5CFA;color:white;border-radius:13px;"
            "font-size:13px;font-weight:700; }");

        auto* stepText = new QLabel(text, m_helpCard);
        stepText->setObjectName("HelpStepText");
        stepText->setWordWrap(true);

        stepRow->addWidget(circle, 0, Qt::AlignTop);
        stepRow->addWidget(stepText, 1);
        return stepRow;
    };

    cardLayout->addLayout(makeStep(1, tr("Уберите опасные предметы и поддержите голову.")));
    cardLayout->addSpacing(10);
    cardLayout->addLayout(makeStep(2, tr("Не сдерживайте судороги и ничего не кладите в рот.")));
    cardLayout->addSpacing(10);
    cardLayout->addLayout(makeStep(3, tr("Поверните человека на бок после окончания приступа.")));
    cardLayout->addSpacing(10);
    cardLayout->addLayout(makeStep(4, tr("Засеките длительность приступа.")));
    cardLayout->addSpacing(14);

    // ── Warning note ──
    m_helpTextLabel = new QLabel(m_helpCard);
    m_helpTextLabel->setObjectName("HelpOverlayWarning");
    m_helpTextLabel->setWordWrap(true);
    m_helpTextLabel->setText(
        tr("Вызывайте 103 или 112, если приступ длится более 5 минут, "
           "повторяется подряд или человек получил травму."));

    cardLayout->addWidget(m_helpTextLabel);
    cardLayout->addSpacing(18);

    // ── Actions ──
    auto* actions = new QHBoxLayout;
    actions->setSpacing(10);

    m_helpMailBtn = new QPushButton(tr("Экстренное уведомление"), m_helpCard);
    m_helpMailBtn->setObjectName("DangerButton");
    m_helpMailBtn->setMinimumHeight(42);
    m_helpMailBtn->setCursor(Qt::PointingHandCursor);

    auto* closeBtn = new QPushButton(tr("Понятно"), m_helpCard);
    closeBtn->setObjectName("PrimaryButton");
    closeBtn->setMinimumHeight(42);
    closeBtn->setCursor(Qt::PointingHandCursor);

    actions->addWidget(m_helpMailBtn, 1);
    actions->addWidget(closeBtn, 1);
    cardLayout->addLayout(actions);

    row->addWidget(m_helpCard);
    row->addStretch();

    layout->addLayout(row);
    layout->addStretch();

    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::hideHelpOverlay);
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

    // ── Verification widgets (hidden until needed) ──
    mauthCodeHint = new QLabel(tr("На указанную почту отправлен 6-значный код. Введите его ниже:"), mauthCard);
    mauthCodeHint->setObjectName("AuthOverlaySubtitle");
    mauthCodeHint->setWordWrap(true);

    mauthCodeEdit = new QLineEdit(mauthCard);
    mauthCodeEdit->setPlaceholderText("000000");
    mauthCodeEdit->setMaxLength(6);
    mauthCodeEdit->setAlignment(Qt::AlignCenter);
    mauthCodeEdit->setStyleSheet(
        "QLineEdit { font-size:24px; font-weight:700; letter-spacing:8px;"
        " border:2px solid #E4DCF8; border-radius:12px; padding:12px 16px;"
        " background:#FBFAFF; color:#2B2144; }"
        "QLineEdit:focus { border-color:#7C5CFA; }");

    mauthVerifyBtn = new QPushButton(tr("Подтвердить"), mauthCard);
    mauthVerifyBtn->setObjectName("PrimaryButton");

    auto* verRow = new QHBoxLayout;
    verRow->setSpacing(10);
    mauthResendBtn = new QPushButton(tr("Отправить ещё раз"), mauthCard);
    mauthResendBtn->setObjectName("SecondaryButton");
    mauthResendBtn->setMinimumHeight(36);
    mauthBackBtn = new QPushButton(tr("← Назад ко входу"), mauthCard);
    mauthBackBtn->setObjectName("SecondaryButton");
    mauthBackBtn->setMinimumHeight(36);
    verRow->addWidget(mauthResendBtn, 1);
    verRow->addWidget(mauthBackBtn, 1);

    cardLayout->addWidget(mauthCodeHint);
    cardLayout->addWidget(mauthCodeEdit);
    cardLayout->addWidget(mauthVerifyBtn);
    cardLayout->addLayout(verRow);

    // hide verification by default
    mauthCodeHint->hide();
    mauthCodeEdit->hide();
    mauthVerifyBtn->hide();
    mauthResendBtn->hide();
    mauthBackBtn->hide();

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
            m_core->applyLanguage(QStringLiteral("be_Latn"));
        else if (now == QStringLiteral("be_Latn"))
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

            // Сохраняем данные и отправляем код (аккаунт НЕ создаётся)
            mauthPendingEmail = email;
            mauthPendingPassword = password;
            mauthPendingName = fullName;
            mauthAwaitingVerification = true; // блокируем любой authSucceeded
            m_core->sendVerificationCode(email, fullName);
            setVerificationMode(true);
            return;
        }

        m_core->signIn(email, password);
    });

    connect(m_core, &AppCore::authSucceeded, this, [this]() {
        // Ничего не делаем пока ждём код
        if (mauthAwaitingVerification) {
            qDebug() << "AUTH BLOCKED — waiting for verification code";
            return;
        }

        if (mauthRegisterMode) {
            // Аккаунт создан после верификации кода — заполняем профиль
            mauthRegisterMode = false;
            if (m_core->isAuthenticated()) {
                QJsonObject payload{
                    { "id", m_core->currentUserId() },
                    { "email", m_core->currentEmail() },
                    { "full_name", mauthPendingName },
                    { "accepted_privacy_policy", true },
                    { "accepted_terms_of_service", true },
                    { "accepted_personal_data_processing", true },
                    { "consent_accepted_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate) }
                };
                m_core->updateProfile(payload);
            }
            setVerificationMode(false);
            hideAuthOverlay();
            showToast(m_central, tr("Добро пожаловать в Trackly."), ToastKind::Success, 2200);
            if (m_navProfile)
                m_navProfile->setText(m_core->currentEmail().section('@', 0, 0));
            m_core->fetchDashboard();
            m_core->fetchTriggers();
            navigateTo(0, tr("Главная"));
        } else {
            hideAuthOverlay();
            showToast(m_central, tr("Добро пожаловать в Trackly."), ToastKind::Success, 2200);
            if (m_navProfile)
                m_navProfile->setText(m_core->currentEmail().section('@', 0, 0));
            m_core->fetchDashboard();
            m_core->fetchTriggers();
            navigateTo(0, tr("Главная"));
        }
    });

    // ── Verification signal handlers ──
    connect(m_core, &AppCore::verificationCodeSent, this, [this]() {
        showToast(m_central, tr("Код отправлен на %1").arg(mauthPendingEmail), ToastKind::Success, 2600);
    });

    connect(m_core, &AppCore::verificationCodeError, this, [this](const QString& e) {
        showToast(m_central, e, ToastKind::Error, 4000);
    });

    connect(m_core, &AppCore::codeVerified, this, [this]() {
        showToast(m_central, tr("Код подтверждён!"), ToastKind::Success, 2200);
        mauthAwaitingVerification = false; // разблокируем — сейчас будет signUp → authSucceeded
        // Создаём аккаунт только сейчас (после верификации)
        m_core->signUp(mauthPendingEmail, mauthPendingPassword, mauthPendingName);
    });

    connect(m_core, &AppCore::codeVerifyError, this, [this](const QString& e) {
        showToast(m_central, tr("❌ %1").arg(prettifyError(e)), ToastKind::Error, 4000);
    });

    // ── Verification UI handlers ──
    connect(mauthVerifyBtn, &QPushButton::clicked, this, [this]() {
        const QString code = mauthCodeEdit ? mauthCodeEdit->text().trimmed() : QString();
        if (code.length() != 6) {
            showToast(m_central, tr("Введите 6-значный код."), ToastKind::Error, 2600);
            return;
        }
        m_core->verifyCode(mauthPendingEmail, code);
    });

    connect(mauthResendBtn, &QPushButton::clicked, this, [this]() {
        m_core->sendVerificationCode(mauthPendingEmail, mauthPendingName);
    });

    connect(mauthBackBtn, &QPushButton::clicked, this, [this]() {
        mauthAwaitingVerification = false;
        setVerificationMode(false);
        setAuthRegisterMode(false);
        mauthEmailEdit->setText(mauthPendingEmail);
        mauthPasswordEdit->clear();
        mauthCodeEdit->clear();
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

void MainWindow::setVerificationMode(bool enabled)
{
    mauthVerificationMode = enabled;

    const bool ver = enabled;
    // Hide normal auth form
    if (mauthNameEdit) mauthNameEdit->setVisible(!ver && mauthRegisterMode);
    if (mauthEmailEdit) mauthEmailEdit->setVisible(!ver);
    if (mauthPasswordEdit) mauthPasswordEdit->setVisible(!ver);
    if (mauthSubmit) mauthSubmit->setVisible(!ver);
    if (mauthSwitch) mauthSwitch->setVisible(!ver);
    // Hide consent
    if (auto consentHost = mauthPolicyCheckbox ? mauthPolicyCheckbox->parentWidget() : nullptr)
        consentHost->setVisible(!ver && mauthRegisterMode);

    // Show/hide verification widgets
    if (mauthCodeHint) mauthCodeHint->setVisible(ver);
    if (mauthCodeEdit) mauthCodeEdit->setVisible(ver);
    if (mauthVerifyBtn) mauthVerifyBtn->setVisible(ver);
    if (mauthResendBtn) mauthResendBtn->setVisible(ver);
    if (mauthBackBtn) mauthBackBtn->setVisible(ver);

    if (mauthTitle) mauthTitle->setText(ver ? tr("Подтверждение") : (mauthRegisterMode ? tr("Создание аккаунта Trackly") : tr("Вход в Trackly")));
    if (mauthSubtitle) mauthSubtitle->setText(ver ? tr("На указанную почту отправлен 6-значный код. Введите его ниже:") : QString());

    if (ver) {
        mauthCodeEdit->clear();
        mauthCodeEdit->setFocus();
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
                if (key == "medication_log" || key == "medication_log_cancelled"
                    || key == "medicationlogdeleted") {
                    m_core->fetchMedicationLogs();
                }
                if (key == "medication_schedule") {
                    m_core->fetchMedicationSchedules();
                }
            });

    connect(m_core, &AppCore::medicationLogCancelled,
            this, [this](const QString&) {
                m_core->fetchMedicationLogs();
            });

    connect(m_languageBtn, &QPushButton::clicked, this, [this]() {
        const QString now = m_core->language();
        if (now == "ru") m_core->applyLanguage("be");
        else if (now == "be") m_core->applyLanguage("be_Latn");
        else if (now == "be_Latn") m_core->applyLanguage("en");
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

    connect(m_core, &AppCore::authSucceeded, this, [this] {
        setAuthBusy(false);
        if (mauthAwaitingVerification) return;
        // Основная логика в buildAuthOverlay. Здесь только сброс busy + фетчи.
        if (!mauthVerificationMode) {
            hideAuthOverlay();
            if (m_navProfile)
                m_navProfile->setText(m_core->currentEmail().section('@', 0, 0));
            m_core->fetchMedications();
            m_core->fetchMedicationSchedules();
            m_core->fetchMedicationLogs();
            m_core->fetchDashboard();
            m_core->fetchProfile();
            m_core->fetchAttachments();
            m_core->fetchTriggers();
        }
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
                if (key == "medication_log" || key == "medication_log_cancelled"
                    || key == "medicationlogdeleted") {
                    m_core->fetchMedicationLogs();
                }
                if (key == "medication_schedule") {
                    m_core->fetchMedicationSchedules();
                }
            });

    connect(m_core, &AppCore::medicationLogCancelled,
            this, [this](const QString&) {
                m_core->fetchMedicationLogs();
            });

    connect(m_core, &AppCore::medicationCreated,
            this, [this](const QJsonObject& med) {
                const QString medId = med.value("id").toString();
                if (!medId.isEmpty()) {
                    m_medicationScheduleDates[medId] = {m_pendingMedStartDate, m_pendingMedEndDate};
                    QSettings s("Trackly", "Trackly");
                    s.setValue("medication/" + medId + "/sched_start", m_pendingMedStartDate);
                    s.setValue("medication/" + medId + "/sched_end", m_pendingMedEndDate);
                    m_pendingMedStartDate = QDate();
                    m_pendingMedEndDate   = QDate();
                    m_pendingMedNoEndDate = false;
                }
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
    else if (m_navProfile && m_core->isAuthenticated())
        m_navProfile->setText(m_core->currentEmail().section('@', 0, 0));
}

void MainWindow::retranslateUi()
{
    if (!m_stack) return;

    const int currentIdx = m_stack->currentIndex();

    // ── Сохраняем состояние формы приступов перед пересозданием ──
    QString savedSeizureEditId = m_editSeizureId;

    // ── Сбрасываем указатели на виджеты (будут пересозданы) ──
    m_dashboardCalendar = nullptr;
    m_dashboardExamHost = nullptr;
    m_dashboardExamLayout = nullptr;
    m_summary1 = m_summary2 = m_summary3 = m_summary4 = nullptr;
    m_dashboardReportBtn = nullptr;

    m_seizureDate = nullptr;
    m_seizurePlace = nullptr;
    m_seizureType = nullptr;
    m_seizureTriggers = nullptr;
    m_seizurePost = nullptr;
    m_seizureSeverity = nullptr;
    m_seizureNotes = nullptr;
    m_saveSeizureBtn = nullptr;

    m_medCalendar = nullptr;
    m_dayMedicationHost = nullptr;
    m_dayMedicationLayout = nullptr;
    m_medTitle = nullptr;
    m_medTherapy = nullptr;
    m_medDose = nullptr;
    m_medTimesPerDay = nullptr;
    m_medTimes = nullptr;
    m_medStock = nullptr;
    m_medCardsHost = nullptr;
    m_medCardsLayout = nullptr;
    m_saveMedicationBtn = nullptr;
    m_openMedicationDialogBtn = nullptr;
    m_medsTab = nullptr;
    m_todayIntakesHost = nullptr;
    m_todayIntakesLayout = nullptr;
    m_intakesDateEdit = nullptr;
    m_medInn = nullptr;
    m_medDoseTablets = nullptr;
    m_medStockTablets = nullptr;

    m_examType = nullptr;
    m_examSubtype = nullptr;
    m_examDate = nullptr;
    m_examNotes = nullptr;
    m_saveExamBtn = nullptr;
    m_pastExamHost = nullptr;
    m_pastExamLayout = nullptr;
    m_plannedExamHost = nullptr;
    m_plannedExamLayout = nullptr;
    m_examDropArea = nullptr;

    m_analyticsHost = nullptr;
    m_analyticsLayout = nullptr;
    m_analyticsPeriodCombo = nullptr;
    m_analyticsFromDate = nullptr;
    m_analyticsToDate = nullptr;
    m_analyticsCustomRange = nullptr;
    m_aiAnalyticsBtn = nullptr;
    m_aiPlaceholder = nullptr;
    m_aiResultLabel = nullptr;
    m_exportAnalyticsPdfBtn = nullptr;

    m_triggerKind = nullptr;
    m_triggerDate = nullptr;
    m_moodScore = nullptr;
    m_sleepHours = nullptr;
    m_sleepQuality = nullptr;
    m_triggerTitle = nullptr;
    m_triggerTags = nullptr;
    m_triggerNote = nullptr;
    m_saveTriggerBtn = nullptr;
    m_triggerListHost = nullptr;
    m_triggerListLayout = nullptr;

    m_avatarPreview = nullptr;
    m_profileFullName = nullptr;
    m_profileBirthDate = nullptr;
    m_profileStatus = nullptr;
    m_profileAbout = nullptr;
    m_profileGender = nullptr;
    m_profilePhone = nullptr;
    m_profileEmail = nullptr;
    m_profileAddress = nullptr;
    m_profileEmergency = nullptr;
    m_profileDoctor = nullptr;
    m_profileDiagnosis = nullptr;

    // ── Удаляем все страницы из стека ──
    while (m_stack->count() > 0) {
        QWidget* w = m_stack->widget(0);
        m_stack->removeWidget(w);
        delete w;
    }

    // ── Пересоздаём все страницы (tr() вызовется с новым языком) ──
    m_stack->addWidget(createDashboardPage());   // 0
    m_stack->addWidget(createSeizuresPage());    // 1
    m_stack->addWidget(createMedicinesPage());   // 2
    m_stack->addWidget(createExamsPage());       // 3
    m_stack->addWidget(createAnalyticsPage());   // 4
    m_stack->addWidget(createTriggersPage());    // 5
    m_stack->addWidget(createProfilePage());     // 6

    m_editSeizureId = savedSeizureEditId;

    // ── Восстанавливаем текущую страницу ──
    m_stack->setCurrentIndex(currentIdx);

    // ── навигация ──
    if (m_navHome) m_navHome->setText(tr("Главная"));
    if (m_navSeizures) m_navSeizures->setText(tr("Приступы"));
    if (m_navMeds) m_navMeds->setText(tr("Лекарства"));
    if (m_navExams) m_navExams->setText(tr("Обследования"));
    if (m_navAnalytics) m_navAnalytics->setText(tr("Аналитика"));
    if (m_navTriggers) m_navTriggers->setText(tr("Самочувствие"));
    if (m_navProfile) {
        if (m_core->isAuthenticated())
            m_navProfile->setText(m_core->currentEmail().section('@', 0, 0));
        else
            m_navProfile->setText(tr("Профиль"));
    }
    if (m_themeBtn) m_themeBtn->setText(tr("Тема"));
    if (m_languageBtn) m_languageBtn->setText(tr("Язык"));

    // ── заголовок текущей страницы ──
    if (m_title) {
        if (currentIdx == 0) m_title->setText(tr("Дашборд"));
        else if (currentIdx == 1) m_title->setText(tr("Приступы"));
        else if (currentIdx == 2) m_title->setText(tr("Терапия"));
        else if (currentIdx == 3) m_title->setText(tr("Обследования"));
        else if (currentIdx == 4) m_title->setText(tr("Аналитика"));
        else if (currentIdx == 5) m_title->setText(tr("Самочувствие"));
        else if (currentIdx == 6) m_title->setText(tr("Профиль"));
    }

    // ── Восстанавливаем содержимое ВСЕХ страниц из кеша ──
    fillSeizures(m_cachedSeizureItems);
    fillMedications(m_cachedMedications);
    fillExaminations(m_cachedExaminations);
    refreshTriggersList();
    fillProfile(m_cachedProfile);
    refreshAnalyticsPage();
    updateDashboardCalendarDecorations();
    refreshDashboardDayDetails();

    // ── Перезапрашиваем свежие данные ──
    m_core->fetchDashboard();
    m_core->fetchMedicationSchedules();
    refreshDayMedicationSummary();

    // ── auth overlay ──
    if (mauthOverlay && mauthOverlay->isVisible()) {
        if (mauthVerificationMode) {
            if (mauthTitle) mauthTitle->setText(tr("Подтверждение"));
            if (mauthCodeHint) mauthCodeHint->setText(tr("На указанную почту отправлен 6-значный код. Введите его ниже:"));
            if (mauthVerifyBtn) mauthVerifyBtn->setText(tr("Подтвердить"));
            if (mauthResendBtn) mauthResendBtn->setText(tr("Отправить ещё раз"));
            if (mauthBackBtn) mauthBackBtn->setText(tr("← Назад ко входу"));
        } else {
            if (mauthTitle) mauthTitle->setText(mauthRegisterMode
                ? tr("Создание аккаунта Trackly")
                : tr("Вход в Trackly"));
            mauthSubtitle->setText(mauthRegisterMode
                ? tr("Присоединяйся и отслеживай своё состояние и прогресс лечения.")
                : tr("Введите email и пароль, чтобы продолжить."));
            mauthNameEdit->setPlaceholderText(tr("Имя"));
            mauthEmailEdit->setPlaceholderText(tr("E-mail..."));
            mauthPasswordEdit->setPlaceholderText(tr("Пароль"));
            mauthSubmit->setText(mauthRegisterMode
                ? tr("Зарегистрироваться")
                : tr("Войти"));
            mauthSwitch->setText(mauthRegisterMode
                ? tr("Уже есть аккаунт")
                : tr("Создать аккаунт"));
        }
    }

    // ── help overlay (rebuild for retranslation) ──
    if (m_helpOverlay) {
        bool wasVisible = m_helpOverlay->isVisible();
        delete m_helpOverlay;
        m_helpOverlay = nullptr;
        m_helpCard = nullptr;
        m_helpTextLabel = nullptr;
        m_helpMailBtn = nullptr;
        buildHelpOverlay();
        if (wasVisible) m_helpOverlay->show();
    }

    // ── topbar ──
    if (m_topbarReportBtn) m_topbarReportBtn->setText(tr("Отчёт о здоровье"));
    if (m_topbarAiBtn) m_topbarAiBtn->setText(tr("AI"));
    if (m_topbarExportPdfBtn) m_topbarExportPdfBtn->setText(tr("Экспорт PDF"));

    updateSidebarUi();
    updateIcons();
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
        showToast(m_central, tr("Укажите email экстренного контакта в профиле"), ToastKind::Error, 3000);
        return;
    }

    if (!to.contains('@')) {
        showToast(m_central, tr("В поле экстренного контакта должен быть email"), ToastKind::Error, 3000);
        return;
    }

    // Берём имя из кеша профиля (надёжнее чем виджет, который мог быть пересоздан)
    QString patientName = m_cachedProfile.value("full_name").toString().trimmed();
    if (patientName.isEmpty() && m_profileFullName)
        patientName = m_profileFullName->text().trimmed();
    if (patientName.isEmpty())
        patientName = m_core->currentEmail().section('@', 0, 0);
    if (patientName.isEmpty())
        patientName = tr("Пользователь Trackly");

    const QString nowStr = QDateTime::currentDateTime().toString("dd MMMM yyyy, HH:mm");

    const QString text = tr("Экстренное уведомление от %1.\n"
                            "Отправлено: %2.\n\n"
                            "Возможен эпилептический приступ. Проверьте состояние пользователя и свяжитесь с ним как можно скорее.")
                            .arg(patientName, nowStr);

    m_core->sendEmergencyEmail(to, patientName, text);
    showToast(m_central, tr("Экстренное уведомление отправлено на %1").arg(to), ToastKind::Info, 2600);
}

void MainWindow::createPdfReport()
{
    showReportDialog();
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

    repositionToast(m_central);
    positionAiWidgets();

    if (mauthOverlay && mauthLangBtn) {
        const int margin = 18;
        const int x = margin;
        const int y = mauthOverlay->height() - mauthLangBtn->height() - margin;
        mauthLangBtn->move(x, qMax(0, y));
    }
}

// --- showConfirmDialog --- (общий хелпер с блюром)
bool MainWindow::showConfirmDialog(const QString& title, const QString& message)
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
    dlg.setWindowTitle(title);

    auto* outer = new QVBoxLayout(&dlg);
    outer->setContentsMargins(16, 16, 16, 16);
    outer->setSpacing(12);

    auto* card = new QFrame(&dlg);
    card->setObjectName("SectionCard");
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(14);

    auto* titleLbl = new QLabel(title, card);
    titleLbl->setObjectName("SectionTitle");
    titleLbl->setWordWrap(true);

    auto* msgLbl = new QLabel(message, card);
    msgLbl->setObjectName("MutedLabel");
    msgLbl->setWordWrap(true);

    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();

    auto* noBtn = new QPushButton(tr("Отмена"), card);
    noBtn->setObjectName("SecondaryButton");

    auto* yesBtn = new QPushButton(tr("Да"), card);
    yesBtn->setObjectName("PrimaryButton");

    btnRow->addWidget(noBtn);
    btnRow->addWidget(yesBtn);

    layout->addWidget(titleLbl);
    layout->addWidget(msgLbl);
    layout->addStretch();
    layout->addLayout(btnRow);

    outer->addWidget(card);

    bool result = false;
    connect(yesBtn, &QPushButton::clicked, &dlg, [&]() { result = true; dlg.accept(); });
    connect(noBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    dlg.exec();

    if (central) {
        central->setGraphicsEffect(nullptr); // setGraphicsEffect(nullptr) сам удаляет старый
    }

    return result;
}

// --- showBlurredDialog --- (накладывает блюр, exec диалога, убирает блюр)
void MainWindow::showBlurredDialog(QDialog* dlg)
{
    auto* central = this->centralWidget();
    if (central && dlg) {
        auto* blur = new QGraphicsBlurEffect(central);
        blur->setBlurRadius(6.0);
        blur->setBlurHints(QGraphicsBlurEffect::QualityHint);
        central->setGraphicsEffect(blur);

        dlg->exec();

        central->setGraphicsEffect(nullptr); // сам удалит blur
    } else if (dlg) {
        dlg->exec();
    }
}