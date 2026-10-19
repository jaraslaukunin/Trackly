// Profile page — Telegram-style design
// Round avatar · Gravatar · status · about · settings

#include "mainwindow.h"
#include "appcore.h"
#include "utils.h"
#include "widgets/toastwidget.h"
#include "page_includes.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QtMath>
#include <QIcon>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSettings>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWheelEvent>

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
        showAvatarPreviewDialog();
    });

    // Right side: name, email, status
    auto* infoCol = new QVBoxLayout;
    infoCol->setSpacing(8);

    m_profileFullName = new QLineEdit(headerCard);
    m_profileFullName->setPlaceholderText(tr("Имя"));
    m_profileFullName->setObjectName("ProfileNameEdit");

    m_profileEmail = new QLineEdit(headerCard);
    m_profileEmail->setReadOnly(true);
    m_profileEmail->setFocusPolicy(Qt::NoFocus);
    m_profileEmail->setObjectName("ProfileEmailEdit");

    // Status field
    m_profileStatus = new QLineEdit(headerCard);
    m_profileStatus->setPlaceholderText(tr("Статус (например: в порядке)"));
    m_profileStatus->setMaxLength(80);

    infoCol->addWidget(m_profileFullName);
    infoCol->addWidget(m_profileEmail);
    infoCol->addSpacing(4);
    infoCol->addWidget(m_profileStatus);

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

    m_profileAbout = new QTextEdit(aboutCard);
    m_profileAbout->setPlaceholderText(tr("Расскажи о себе, своём диагнозе, особенностях..."));
    m_profileAbout->setFixedHeight(100);

    aboutLay->addWidget(aboutTitle);
    aboutLay->addWidget(m_profileAbout);

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
    // клик по аватару — showAvatarPreviewDialog (привязан выше, стр.73)
    // Actually use a transparent button overlay
    auto* avatarBtn = new QPushButton(m_avatarPreview);
    avatarBtn->setFixedSize(120, 120);
    avatarBtn->setStyleSheet("QPushButton { border-radius: 60px; background: transparent; }");
    avatarBtn->setCursor(Qt::PointingHandCursor);
    QObject::connect(avatarBtn, &QPushButton::clicked, page, [this]() {
        showAvatarPreviewDialog();
    });

    QObject::connect(saveBtn, &QPushButton::clicked, page, [this]() {
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
        payload["status"]           = m_profileStatus->text().trimmed();
        payload["diagnosis"]        = m_profileAbout->toPlainText().trimmed();
        m_core->updateProfile(payload);
    });

    QObject::connect(logoutBtn, &QPushButton::clicked, page, [this]() {
        m_core->signOut();
    });

    return page;
}

// ─── Avatar picker ──────────────────────────────────────────────────────────

// ─── Avatar crop dialog ─────────────────────────────────────────────────────

class AvatarCropWidget : public QWidget
{
public:
    AvatarCropWidget(const QPixmap& src, QWidget* parent = nullptr)
        : QWidget(parent), m_source(src), m_offset(0, 0), m_rotation(0)
    {
        m_diameter = qMin(280, qMin(src.width(), src.height()));
        m_scale = 1.0;
        setMinimumSize(m_diameter + 60, m_diameter + 60);
        setMouseTracking(true);
        updateOffsetBounds();
    }

    void setRotation(qreal degrees)
    {
        m_rotation = degrees;
        updateOffsetBounds();
        clampOffset();
        update();
    }

    qreal rotation() const { return m_rotation; }

    QPixmap croppedResult() const
    {
        const int d = m_diameter;
        const int cx = (width() - d) / 2;
        const int cy = (height() - d) / 2;

        // source rect in original coords
        const int sx = static_cast<int>((-m_offset.x() + cx) / m_scale);
        const int sy = static_cast<int>((-m_offset.y() + cy) / m_scale);
        const int ss = static_cast<int>(d / m_scale);
        QPixmap cropped = m_source.copy(sx, sy, ss, ss);

        // Apply rotation if needed
        if (qAbs(m_rotation) > 0.01) {
            QPixmap rotated(cropped.size());
            rotated.fill(Qt::transparent);
            QPainter rp(&rotated);
            rp.setRenderHint(QPainter::Antialiasing);
            rp.setRenderHint(QPainter::SmoothPixmapTransform);
            rp.translate(rotated.width() / 2.0, rotated.height() / 2.0);
            rp.rotate(m_rotation);
            rp.translate(-cropped.width() / 2.0, -cropped.height() / 2.0);
            rp.drawPixmap(0, 0, cropped);
            rp.end();
            cropped = rotated;
        }

        // scale to target and round
        QPixmap result(256, 256);
        result.fill(Qt::transparent);
        QPainter p(&result);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath path;
        path.addEllipse(0, 0, 256, 256);
        p.setClipPath(path);
        p.drawPixmap(0, 0, 256, 256, cropped.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        p.end();
        return result;
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        const int d = m_diameter;
        const int cx = (width() - d) / 2;
        const int cy = (height() - d) / 2;

        // image with rotation
        const int drawW = static_cast<int>(m_source.width() * m_scale);
        const int drawH = static_cast<int>(m_source.height() * m_scale);

        p.save();
        if (qAbs(m_rotation) > 0.01) {
            const int centerX = m_offset.x() + drawW / 2;
            const int centerY = m_offset.y() + drawH / 2;
            p.translate(centerX, centerY);
            p.rotate(m_rotation);
            p.translate(-centerX, -centerY);
        }
        p.drawPixmap(m_offset.x(), m_offset.y(), drawW, drawH, m_source);
        p.restore();

        // dark overlay outside circle
        QPainterPath overlay;
        overlay.addRect(rect());
        overlay.addEllipse(cx, cy, d, d);
        p.fillPath(overlay, QColor(0, 0, 0, 160));

        // circle border
        p.setPen(QPen(Qt::white, 2));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(cx, cy, d, d);
    }

    void mousePressEvent(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
            m_lastPos = e->pos();
    }

    void mouseMoveEvent(QMouseEvent* e) override
    {
        if (e->buttons() & Qt::LeftButton) {
            QPoint delta = e->pos() - m_lastPos;
            m_offset += delta;
            clampOffset();
            m_lastPos = e->pos();
            update();
        }
    }

    void wheelEvent(QWheelEvent* e) override
    {
        double factor = (e->angleDelta().y() > 0) ? 1.1 : 0.9;
        m_scale = qBound(0.3, m_scale * factor, 3.0);
        updateOffsetBounds();
        clampOffset();
        update();
    }

private:
    void updateOffsetBounds()
    {
        const int d = m_diameter;
        const int srcW = m_source.width();
        const int srcH = m_source.height();

        // bounding box after rotation
        const qreal rad = qDegreesToRadians(qAbs(m_rotation));
        const int rw = static_cast<int>(srcW * qAbs(qCos(rad)) + srcH * qAbs(qSin(rad)));
        const int rh = static_cast<int>(srcW * qAbs(qSin(rad)) + srcH * qAbs(qCos(rad)));
        const int drawW = static_cast<int>(rw * m_scale);
        const int drawH = static_cast<int>(rh * m_scale);

        const int cx = (width() - d) / 2;
        const int cy = (height() - d) / 2;
        const int minX = cx + d - drawW;
        const int maxX = cx;
        const int minY = cy + d - drawH;
        const int maxY = cy;
        m_minOff = QPoint(qMin(minX, maxX), qMin(minY, maxY));
        m_maxOff = QPoint(qMax(minX, maxX), qMax(minY, maxY));
    }

    void clampOffset()
    {
        if (m_minOff.x() <= m_maxOff.x()) {
            m_offset.setX(qBound(m_minOff.x(), m_offset.x(), m_maxOff.x()));
            m_offset.setY(qBound(m_minOff.y(), m_offset.y(), m_maxOff.y()));
        } else {
            m_offset = QPoint((width() - m_diameter) / 2, (height() - m_diameter) / 2);
        }
    }

    QPixmap m_source;
    QPoint m_offset, m_lastPos, m_minOff, m_maxOff;
    int m_diameter;
    double m_scale;
    qreal m_rotation;
};

void MainWindow::pickAvatar()
{
    const QString file = QFileDialog::getOpenFileName(
        this, tr("Выбери фото"), QString(),
        tr("Изображения (*.png *.jpg *.jpeg *.webp)"));
    if (file.isEmpty()) return;

    QPixmap original(file);
    if (original.isNull()) return;

    // ─── Диалог обрезки ───
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Обрезать аватар"));
    dlg.setModal(true);
    dlg.setWindowFlag(Qt::FramelessWindowHint, true);
    dlg.setAttribute(Qt::WA_TranslucentBackground, true);
    dlg.setMinimumSize(420, 540);

    auto* outer = new QVBoxLayout(&dlg);
    outer->setContentsMargins(16, 16, 16, 16);
    outer->setSpacing(12);

    auto* frame = new QFrame(&dlg);
    frame->setObjectName("SectionCard");
    auto* layout = new QVBoxLayout(frame);
    layout->setSpacing(12);

    auto* title = new QLabel(tr("Двигайте и масштабируйте"), frame);
    title->setObjectName("SectionTitle");

    auto* cropWidget = new AvatarCropWidget(original, frame);

    auto* hint = new QLabel(tr("Колёсико — масштаб, перетаскивание — позиция"), frame);
    hint->setObjectName("MutedLabel");
    hint->setWordWrap(true);

    // ── Rotation controls ──
    auto* rotRow = new QHBoxLayout;
    rotRow->setSpacing(8);

    auto* rotLabel = new QLabel(tr("Поворот:"), frame);
    rotLabel->setObjectName("MutedLabel");

    auto* rotSlider = new QSlider(Qt::Horizontal, frame);
    rotSlider->setRange(-180, 180);
    rotSlider->setValue(0);
    rotSlider->setTickPosition(QSlider::TicksBelow);
    rotSlider->setTickInterval(45);
    rotSlider->setFixedWidth(160);

    auto* rotValueLabel = new QLabel(QStringLiteral("0°"), frame);
    rotValueLabel->setObjectName("MutedLabel");
    rotValueLabel->setFixedWidth(36);
    rotValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto* rotLeftBtn = new QPushButton(tr("↺"), frame);
    rotLeftBtn->setFixedSize(32, 32);
    rotLeftBtn->setToolTip(tr("Повернуть на -90°"));

    auto* rotRightBtn = new QPushButton(tr("↻"), frame);
    rotRightBtn->setFixedSize(32, 32);
    rotRightBtn->setToolTip(tr("Повернуть на +90°"));

    auto* rotResetBtn = new QPushButton(tr("⟲"), frame);
    rotResetBtn->setFixedSize(32, 32);
    rotResetBtn->setToolTip(tr("Сбросить поворот"));

    rotRow->addWidget(rotLabel);
    rotRow->addWidget(rotLeftBtn);
    rotRow->addWidget(rotSlider);
    rotRow->addWidget(rotValueLabel);
    rotRow->addWidget(rotRightBtn);
    rotRow->addWidget(rotResetBtn);
    rotRow->addStretch();

    QObject::connect(rotSlider, &QSlider::valueChanged, frame, [cropWidget, rotValueLabel](int v) {
        cropWidget->setRotation(static_cast<qreal>(v));
        rotValueLabel->setText(QString::number(v) + QStringLiteral("°"));
    });
    QObject::connect(rotLeftBtn, &QPushButton::clicked, frame, [cropWidget, rotSlider]() {
        int v = rotSlider->value() - 90;
        if (v < -180) v += 360;
        rotSlider->setValue(v);
    });
    QObject::connect(rotRightBtn, &QPushButton::clicked, frame, [cropWidget, rotSlider]() {
        int v = rotSlider->value() + 90;
        if (v > 180) v -= 360;
        rotSlider->setValue(v);
    });
    QObject::connect(rotResetBtn, &QPushButton::clicked, frame, [rotSlider]() {
        rotSlider->setValue(0);
    });

    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto* cancelBtn = new QPushButton(tr("Отмена"), frame);
    auto* okBtn = new QPushButton(tr("Готово"), frame);
    okBtn->setObjectName("PrimaryButton");
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(okBtn);

    layout->addWidget(title);
    layout->addWidget(cropWidget, 1);
    layout->addLayout(rotRow);
    layout->addWidget(hint);
    layout->addLayout(btnRow);

    outer->addWidget(frame);

    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    showBlurredDialog(&dlg);

    if (dlg.result() != QDialog::Accepted) return;

    QPixmap avatar = cropWidget->croppedResult();
    m_cachedAvatarPixmap = avatar.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    applyAvatarToPreview();

    // сохраняем локально (пока нет колонки avatar в БД)
    QByteArray pngData;
    QBuffer buf(&pngData);
    buf.open(QIODevice::WriteOnly);
    avatar.save(&buf, "PNG");
    QSettings s("Trackly", "Trackly");
    s.setValue("profile/avatar", QString::fromLatin1(pngData.toBase64()));
    // также пробуем сохранить в Supabase (если колонка есть — ок, иначе молча упадёт)
    QJsonObject payload;
    payload["avatar"] = QString::fromLatin1(pngData.toBase64());
    m_core->updateProfile(payload);
}

// ─── Data fill ──────────────────────────────────────────────────────────────

void MainWindow::fillProfile(const QJsonObject& obj)
{
    if (!m_profileFullName) return;

    m_cachedProfile = obj;

    // загружаем аватар (только если кеш пуст — первый раз)
    if (m_cachedAvatarPixmap.isNull()) {
        QString avatarB64 = obj.value("avatar").toString().trimmed();
        if (avatarB64.isEmpty()) {
            QSettings s("Trackly", "Trackly");
            avatarB64 = s.value("profile/avatar").toString().trimmed();
        }
        if (!avatarB64.isEmpty()) {
            QByteArray imgData = QByteArray::fromBase64(avatarB64.toLatin1());
            QPixmap avatar;
            if (avatar.loadFromData(imgData)) {
                m_cachedAvatarPixmap = avatar.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                applyAvatarToPreview();
            }
        }
    }

    // применяем аватар к preview (актуально после пересоздания страницы)
    if (m_cachedAvatarPixmap.isNull()) {
        m_avatarPreview->setPixmap(QPixmap());
        m_avatarPreview->setText(QStringLiteral("👤"));
    } else {
        applyAvatarToPreview();
    }

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
    m_profileStatus->setText(obj.value("status").toString().trimmed());
    m_profileAbout->setPlainText(obj.value("diagnosis").toString().trimmed());

    // Load Gravatar только если нет своего аватара
    if (m_cachedAvatarPixmap.isNull()) {
        const QString gravatar = gravatarUrl(email, 256);
        auto* net = new QNetworkAccessManager(this);
        QNetworkRequest req{QUrl(gravatar)};
        auto* reply = net->get(req);
        QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, net]() {
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap px;
                px.loadFromData(reply->readAll());
                if (!px.isNull()) {
                    m_cachedAvatarPixmap = px.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    applyAvatarToPreview();
                }
            }
            reply->deleteLater();
            net->deleteLater();
        });
    }

    refreshAnalyticsPage();
}

// ─── applyAvatarToPreview ──────────────────────────────────────────────────

// ─── showAvatarPreviewDialog ──────────────────────────────────────────────

void MainWindow::showAvatarPreviewDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Аватар"));
    dlg.setModal(true);
    dlg.setWindowFlag(Qt::FramelessWindowHint, true);
    dlg.setAttribute(Qt::WA_TranslucentBackground, true);
    dlg.setMinimumWidth(320);

    auto* outer = new QVBoxLayout(&dlg);
    outer->setContentsMargins(16, 16, 16, 16);
    outer->setSpacing(12);

    auto* frame = new QFrame(&dlg);
    frame->setObjectName("SectionCard");
    auto* layout = new QVBoxLayout(frame);
    layout->setSpacing(16);

    // Большой превью
    auto* preview = new QLabel(frame);
    preview->setFixedSize(200, 200);
    preview->setAlignment(Qt::AlignCenter);
    preview->setStyleSheet(
        "QLabel { border-radius: 100px; background: #E4DCF8; font-size: 60px; color: #7C5CFA; }");

    if (!m_cachedAvatarPixmap.isNull()) {
        QPixmap rounded(200, 200);
        rounded.fill(Qt::transparent);
        QPainter pr(&rounded);
        pr.setRenderHint(QPainter::Antialiasing);
        pr.setBrush(m_cachedAvatarPixmap.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        pr.setPen(Qt::NoPen);
        pr.drawEllipse(0, 0, 200, 200);
        pr.end();
        preview->setPixmap(rounded);
    } else {
        preview->setText("👤");
    }

    layout->addWidget(preview, 0, Qt::AlignCenter);

    // Кнопки
    auto* btnRow = new QHBoxLayout;
    auto* changeBtn = new QPushButton(tr("Выбрать другое"), frame);
    auto* deleteBtn = new QPushButton(tr("Удалить"), frame);
    deleteBtn->setStyleSheet(
        "QPushButton { background:#EF4444;color:white;border-radius:16px;padding:10px 18px;font-weight:bold; }"
        "QPushButton:hover { background:#DC2626; }");
    auto* closeBtn = new QPushButton(tr("Закрыть"), frame);

    btnRow->addWidget(changeBtn);
    btnRow->addWidget(deleteBtn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);

    layout->addLayout(btnRow);
    outer->addWidget(frame);

    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(changeBtn, &QPushButton::clicked, &dlg, [&dlg, this]() {
        dlg.accept();
        pickAvatar();
    });
    connect(deleteBtn, &QPushButton::clicked, &dlg, [&dlg, this]() {
        m_cachedAvatarPixmap = QPixmap();
        applyAvatarToPreview();
        QSettings s("Trackly", "Trackly");
        s.remove("profile/avatar");
        QJsonObject payload;
        payload["avatar"] = QJsonValue::Null;
        m_core->updateProfile(payload);
        dlg.accept();
    });

    showBlurredDialog(&dlg);
}

void MainWindow::applyAvatarToPreview()
{
    if (!m_avatarPreview) return;

    if (m_cachedAvatarPixmap.isNull()) {
        m_avatarPreview->setPixmap(QPixmap());
        m_avatarPreview->setText(QStringLiteral("👤"));
        return;
    }

    QPixmap rounded(120, 120);
    rounded.fill(Qt::transparent);
    QPainter pr(&rounded);
    pr.setRenderHint(QPainter::Antialiasing);
    pr.setBrush(m_cachedAvatarPixmap);
    pr.setPen(Qt::NoPen);
    pr.drawEllipse(0, 0, 120, 120);
    pr.end();

    m_avatarPreview->setPixmap(rounded);
}
