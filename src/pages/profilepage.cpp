// Profile page — Telegram-style design
// Round avatar · Gravatar · status · about · settings

#include "mainwindow.h"
#include "appcore.h"
#include "utils.h"
#include "widgets/toastwidget.h"
#include "page_includes.h"

#include <QCryptographicHash>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QTextEdit>
#include <QVBoxLayout>

// ─── Gravatar helper ────────────────────────────────────────────────────────

static QString gravatarUrl(const QString& email, int size = 200)
{
    const QByteArray hash = QCryptographicHash::hash(
        email.trimmed().toLower().toUtf8(), QCryptographicHash::Md5).toHex();
    return QString("https://www.gravatar.com/avatar/%1?s=%2&d=mp").arg(QString::fromLatin1(hash)).arg(size);
}

// ─── Page creation ──────────────────────────────────────────────────────────

QWidget* MainWindow::createProfilePage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(18);

    // ═══ Header card: avatar + name + email + status ═══════════════════
    auto* headerCard = new QFrame(page);
    headerCard->setObjectName("SectionCard");
    auto* hl = new QHBoxLayout(headerCard);
    hl->setContentsMargins(24, 24, 24, 24);
    hl->setSpacing(20);

    // Avatar — round, clickable
    m_avatarPreview = new QLabel(headerCard);
    m_avatarPreview->setFixedSize(120, 120);
    m_avatarPreview->setAlignment(Qt::AlignCenter);
    m_avatarPreview->setCursor(Qt::PointingHandCursor);
    m_avatarPreview->setStyleSheet(
        "QLabel {"
        "  border-radius: 60px;"
        "  background: #E4DCF8;"
        "  font-size: 40px;"
        "  color: #7C5CFA;"
        "}");
    m_avatarPreview->setText("👤");

    QObject::connect(m_avatarPreview, &QLabel::linkActivated, page, [this]() {
        pickAvatar();
    });

    // Right side: name, email, status
    auto* infoCol = new QVBoxLayout;
    infoCol->setSpacing(8);

    m_profileFullName = new QLineEdit(headerCard);
    m_profileFullName->setPlaceholderText(tr("Имя"));
    m_profileFullName->setStyleSheet(
        "QLineEdit { font-size: 22px; font-weight: 700; border: none;"
        " background: transparent; padding: 0; color: #20172E; }");

    m_profileEmail = new QLineEdit(headerCard);
    m_profileEmail->setReadOnly(true);
    m_profileEmail->setFocusPolicy(Qt::NoFocus);
    m_profileEmail->setStyleSheet(
        "QLineEdit { font-size: 13px; border: none; background: transparent;"
        " padding: 0; color: #786C98; }");

    // Status field
    auto* statusEdit = new QLineEdit(headerCard);
    statusEdit->setPlaceholderText(tr("Статус (например: в порядке)"));
    statusEdit->setMaxLength(80);
    statusEdit->setStyleSheet(
        "QLineEdit { font-size: 13px; border: 1px solid #E4DCF8;"
        " border-radius: 8px; padding: 6px 10px; background: #FBFAFF;"
        " color: #5D5379; }");

    infoCol->addWidget(m_profileFullName);
    infoCol->addWidget(m_profileEmail);
    infoCol->addSpacing(4);
    infoCol->addWidget(statusEdit);

    hl->addWidget(m_avatarPreview);
    hl->addLayout(infoCol, 1);

    root->addWidget(headerCard);

    // ═══ About card ═════════════════════════════════════════════════════
    auto* aboutCard = new QFrame(page);
    aboutCard->setObjectName("SectionCard");
    auto* aboutLay = new QVBoxLayout(aboutCard);
    aboutLay->setContentsMargins(20, 20, 20, 20);
    aboutLay->setSpacing(10);

    auto* aboutTitle = new QLabel(tr("О себе"), aboutCard);
    aboutTitle->setObjectName("SectionTitle");

    auto* aboutEdit = new QTextEdit(aboutCard);
    aboutEdit->setPlaceholderText(tr("Расскажи о себе, своём диагнозе, особенностях..."));
    aboutEdit->setFixedHeight(100);

    aboutLay->addWidget(aboutTitle);
    aboutLay->addWidget(aboutEdit);

    root->addWidget(aboutCard);

    // ═══ Details card ═══════════════════════════════════════════════════
    auto* detCard = new QFrame(page);
    detCard->setObjectName("SectionCard");
    auto* detLay = new QVBoxLayout(detCard);
    detLay->setContentsMargins(20, 20, 20, 20);
    detLay->setSpacing(12);

    auto* detTitle = new QLabel(tr("Данные"), detCard);
    detTitle->setObjectName("SectionTitle");

    auto* grid = new QGridLayout;
    grid->setSpacing(10);

    m_profileBirthDate = new QLineEdit(detCard);
    m_profileBirthDate->setPlaceholderText("01.01.1990");

    m_profileGender = new QComboBox(detCard);
    m_profileGender->addItems({tr("Не указан"), tr("Мужской"), tr("Женский")});

    m_profilePhone = new QLineEdit(detCard);
    m_profilePhone->setPlaceholderText("+375...");

    m_profileAddress = new QLineEdit(detCard);
    m_profileAddress->setPlaceholderText(tr("Город, улица"));

    m_profileEmergency = new QLineEdit(detCard);
    m_profileEmergency->setPlaceholderText(tr("Контакт для экстренной связи"));

    m_profileDoctor = new QLineEdit(detCard);
    m_profileDoctor->setPlaceholderText(tr("Лечащий врач"));

    grid->addWidget(new QLabel(tr("Дата рождения"), detCard), 0, 0);
    grid->addWidget(m_profileBirthDate, 0, 1);
    grid->addWidget(new QLabel(tr("Пол"), detCard), 1, 0);
    grid->addWidget(m_profileGender, 1, 1);
    grid->addWidget(new QLabel(tr("Телефон"), detCard), 2, 0);
    grid->addWidget(m_profilePhone, 2, 1);
    grid->addWidget(new QLabel(tr("Адрес"), detCard), 3, 0);
    grid->addWidget(m_profileAddress, 3, 1);
    grid->addWidget(new QLabel(tr("Экстренный контакт"), detCard), 4, 0);
    grid->addWidget(m_profileEmergency, 4, 1);
    grid->addWidget(new QLabel(tr("Врач"), detCard), 5, 0);
    grid->addWidget(m_profileDoctor, 5, 1);

    detLay->addWidget(detTitle);
    detLay->addLayout(grid);

    root->addWidget(detCard);

    // ═══ Actions ════════════════════════════════════════════════════════
    auto* actRow = new QHBoxLayout;
    actRow->setSpacing(12);

    auto* saveBtn = new QPushButton(tr("Сохранить"), page);
    saveBtn->setObjectName("PrimaryButton");
    saveBtn->setMinimumHeight(44);
    saveBtn->setCursor(Qt::PointingHandCursor);

    auto* logoutBtn = new QPushButton(tr("Выйти"), page);
    logoutBtn->setObjectName("DangerButton");
    logoutBtn->setMinimumHeight(44);
    logoutBtn->setCursor(Qt::PointingHandCursor);

    actRow->addStretch();
    actRow->addWidget(saveBtn);
    actRow->addWidget(logoutBtn);

    root->addLayout(actRow);
    root->addStretch();

    // ═══ Scroll ═════════════════════════════════════════════════════════
    auto* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    // wrap everything in a container for scroll
    auto* container = new QWidget;
    container->setLayout(root);
    scroll->setWidget(container);

    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);

    // ═══ Wire ──────────────────────────────────────────────────────────
    QObject::connect(m_avatarPreview, &QLabel::linkActivated, page, [this]() {
        pickAvatar();
    });
    // Make avatar clickable via mouse press
    m_avatarPreview->installEventFilter(page);
    // Actually use a transparent button overlay
    auto* avatarBtn = new QPushButton(m_avatarPreview);
    avatarBtn->setFixedSize(120, 120);
    avatarBtn->setStyleSheet("QPushButton { border-radius: 60px; background: transparent; }");
    avatarBtn->setCursor(Qt::PointingHandCursor);
    QObject::connect(avatarBtn, &QPushButton::clicked, page, [this]() {
        pickAvatar();
    });

    QObject::connect(saveBtn, &QPushButton::clicked, page, [this, statusEdit, aboutEdit]() {
        QJsonObject payload;
        payload["id"]               = m_core->currentUserId();
        payload["email"]            = m_core->currentEmail();
        payload["full_name"]        = m_profileFullName->text().trimmed();
        payload["birth_date"]       = m_profileBirthDate->text().trimmed();
        payload["gender"]           = m_profileGender->currentText();
        payload["phone"]            = m_profilePhone->text().trimmed();
        payload["address"]          = m_profileAddress->text().trimmed();
        payload["emergency_phone"]  = m_profileEmergency->text().trimmed();
        payload["doctor_name"]      = m_profileDoctor->text().trimmed();
        payload["diagnosis"]        = aboutEdit->toPlainText().trimmed();
        m_core->updateProfile(payload);
    });

    QObject::connect(logoutBtn, &QPushButton::clicked, page, [this]() {
        m_core->signOut();
    });

    return page;
}

// ─── Avatar picker ──────────────────────────────────────────────────────────

void MainWindow::pickAvatar()
{
    const QString file = QFileDialog::getOpenFileName(
        this, tr("Выбери фото"), QString(),
        tr("Изображения (*.png *.jpg *.jpeg *.webp)"));

    if (file.isEmpty()) return;

    QPixmap original(file);
    if (original.isNull()) return;

    // Crop to square (center crop), then scale to 256x256
    int size = qMin(original.width(), original.height());
    int x = (original.width() - size) / 2;
    int y = (original.height() - size) / 2;
    QPixmap cropped = original.copy(x, y, size, size);
    QPixmap scaled = cropped.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Round mask for display
    QPixmap rounded(256, 256);
    rounded.fill(Qt::transparent);
    QPainter p(&rounded);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QBrush(scaled));
    p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, 256, 256);
    p.end();

    m_avatarPreview->setPixmap(rounded.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

// ─── Data fill ──────────────────────────────────────────────────────────────

void MainWindow::fillProfile(const QJsonObject& obj)
{
    if (!m_profileFullName) return;

    m_cachedProfile = obj;

    const QString email = obj.value("email").toString().trimmed().isEmpty()
                              ? m_core->currentEmail()
                              : obj.value("email").toString().trimmed();

    m_profileFullName->setText(obj.value("full_name").toString().trimmed());
    m_profileBirthDate->setText(obj.value("birth_date").toString());
    m_profileEmail->setText(email);

    const QString gender = obj.value("gender").toString().trimmed();
    m_profileGender->setCurrentText(gender.isEmpty() ? tr("Не указан") : gender);

    m_profilePhone->setText(obj.value("phone").toString());
    m_profileAddress->setText(obj.value("address").toString());
    m_profileEmergency->setText(obj.value("emergency_phone").toString());
    m_profileDoctor->setText(obj.value("doctor_name").toString());

    // Load Gravatar for profile avatar
    const QString gravatar = gravatarUrl(email, 256);
    auto* net = new QNetworkAccessManager(this);
    QNetworkRequest req{QUrl(gravatar)};
    auto* reply = net->get(req);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, net]() {
        if (reply->error() == QNetworkReply::NoError) {
            QPixmap px;
            px.loadFromData(reply->readAll());
            if (!px.isNull()) {
                QPixmap rounded(256, 256);
                rounded.fill(Qt::transparent);
                QPainter p(&rounded);
                p.setRenderHint(QPainter::Antialiasing);
                QPixmap scaled = px.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                p.setBrush(QBrush(scaled));
                p.setPen(Qt::NoPen);
                p.drawEllipse(0, 0, 256, 256);
                p.end();
                m_avatarPreview->setPixmap(
                    rounded.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }
        reply->deleteLater();
        net->deleteLater();
    });

    refreshAnalyticsPage();
}
