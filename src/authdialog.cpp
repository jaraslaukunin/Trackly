#include "authdialog.h"
#include "appcore.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsBlurEffect>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

AuthDialog::AuthDialog(AppCore* core, QWidget* parent) : QDialog(parent), m_core(core) {
    setModal(true);
    setObjectName("GlassPanel");
    setWindowTitle(tr("Вход в Trackly"));
    resize(430, 420);
    buildUi();
    wire();
}

void AuthDialog::buildUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 28, 28, 28);
    root->setSpacing(14);

    auto* logo = new QLabel("T", this);
    logo->setFixedSize(56, 56);
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet("font-size:28px;font-weight:700;border-radius:18px;background:rgba(120,120,128,0.15);");

    m_title = new QLabel(tr("Добро пожаловать"), this);
    m_title->setObjectName("HeroTitle");
    m_subtitle = new QLabel(tr("Персональный контроль эпилепсии и терапии"), this);
    m_subtitle->setObjectName("MutedLabel");

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("Имя"));
    m_nameEdit->hide();

    m_emailEdit = new QLineEdit(this);
    m_emailEdit->setPlaceholderText(tr("Email"));
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText(tr("Пароль"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_submit = new QPushButton(tr("Войти"), this);
    m_submit->setProperty("accent", true);
    m_switch = new QPushButton(tr("Создать аккаунт"), this);

    root->addWidget(logo, 0, Qt::AlignHCenter);
    root->addWidget(m_title);
    root->addWidget(m_subtitle);
    root->addSpacing(8);
    root->addWidget(m_nameEdit);
    root->addWidget(m_emailEdit);
    root->addWidget(m_passwordEdit);
    root->addSpacing(8);
    root->addWidget(m_submit);
    root->addWidget(m_switch);
    root->addStretch();
}

void AuthDialog::wire() {
    connect(m_switch, &QPushButton::clicked, this, [this]() { setRegisterMode(!m_registerMode); });
    connect(m_submit, &QPushButton::clicked, this, [this]() {
        if (m_registerMode) m_core->signUp(m_emailEdit->text(), m_passwordEdit->text(), m_nameEdit->text());
        else m_core->signIn(m_emailEdit->text(), m_passwordEdit->text());
    });
    connect(m_core, &AppCore::authSucceeded, this, &QDialog::accept);
    connect(m_core, &AppCore::authFailed, this, [this](const QString& e) {
        QMessageBox::warning(this, tr("Ошибка"), e.left(400));
    });
}

void AuthDialog::setRegisterMode(bool enabled) {
    m_registerMode = enabled;
    m_nameEdit->setVisible(enabled);
    m_title->setText(enabled ? tr("Создание аккаунта") : tr("Добро пожаловать"));
    m_submit->setText(enabled ? tr("Зарегистрироваться") : tr("Войти"));
    m_switch->setText(enabled ? tr("Уже есть аккаунт") : tr("Создать аккаунт"));
}
