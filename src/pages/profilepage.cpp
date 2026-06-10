// Profile page implementation for MainWindow
// Extracted from mainwindow.cpp

#include "mainwindow.h"
#include "appcore.h"
#include "page_includes.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
// Page creation
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Avatar picker
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Data fill
// ---------------------------------------------------------------------------

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
