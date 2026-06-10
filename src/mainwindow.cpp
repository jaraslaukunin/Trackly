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
    m_trayIcon->show();

    m_core->fetchMedications();
    m_core->fetchMedicationSchedules();
    m_core->fetchMedicationLogs();

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
    // Profile widget — added below instead
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
    layout->addStretch();

    // Profile widget — QPushButton containing avatar + email
    auto* profileBtn = new QPushButton(m_sidebar);
    profileBtn->setFlat(true);
    profileBtn->setCursor(Qt::PointingHandCursor);
    profileBtn->setStyleSheet(
        "QPushButton { text-align: left; padding: 4px 10px;"
        " border-radius: 12px; background: transparent; }"
        "QPushButton:hover { background: #E5DEF8; }");
    auto* pl = new QHBoxLayout(profileBtn);
    pl->setContentsMargins(0, 4, 0, 4);
    pl->setSpacing(10);

    m_sidebarAvatar = new QLabel(profileBtn);
    m_sidebarAvatar->setFixedSize(36, 36);
    m_sidebarAvatar->setAlignment(Qt::AlignCenter);
    m_sidebarAvatar->setStyleSheet(
        "QLabel { border-radius: 18px; background: #E4DCF8;"
        " font-size: 16px; color: #7C5CFA; }");
    m_sidebarAvatar->setText("👤");

    m_sidebarEmail = new QLabel(profileBtn);
    m_sidebarEmail->setStyleSheet("font-size: 11px; color: #786C98; background: transparent;");

    pl->addWidget(m_sidebarAvatar);
    pl->addWidget(m_sidebarEmail, 1);

    QObject::connect(profileBtn, &QPushButton::clicked, this, [this]() {
        navigateTo(6, tr("Профиль"));
        m_core->fetchProfile();
    });

    layout->addWidget(profileBtn);

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
        m_navProfile->setVisible(false);  // replaced by sidebar profile widget
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
        if (m_sidebarEmail)
            m_sidebarEmail->setText(m_core->currentEmail());
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
        if (m_sidebarEmail)
            m_sidebarEmail->setText(m_core->currentEmail());
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

    if (mauthOverlay && mauthLangBtn) {
        const int margin = 18;
        const int x = margin;
        const int y = mauthOverlay->height() - mauthLangBtn->height() - margin;
        mauthLangBtn->move(x, qMax(0, y));
    }
}
