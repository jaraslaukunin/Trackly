#include "appcore.h"
#include "styles.h"

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QStyleHints>
#include <QTranslator>
#include <QUrl>
#include <QByteArray>
#include <QUrlQuery>

AppCore::AppCore(QObject* parent)
    : QObject(parent)
    , m_net(new QNetworkAccessManager(this))
{
    loadSession();
}

void AppCore::setSupabaseConfig(const QString& url, const QString& anonKey)
{
    m_supabaseUrl = url.trimmed();
    m_anonKey = anonKey.trimmed();
}

void AppCore::applyTheme(ThemeMode mode)
{
    m_themeMode = mode;

    QSettings settings("Trackly", "Trackly");
    settings.setValue("ui/theme", static_cast<int>(mode));

    ThemeMode actual = mode;
    if (mode == ThemeMode::Auto) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        const auto scheme = qApp->styleHints()->colorScheme();
        actual = (scheme == Qt::ColorScheme::Dark) ? ThemeMode::Dark : ThemeMode::Light;
#else
        actual = ThemeMode::Light;
#endif
    }

    qApp->setStyleSheet(actual == ThemeMode::Dark ? Styles::darkQss : Styles::lightQss);

    updateApplicationIcon(actual == ThemeMode::Dark);
}

AppCore::ThemeMode AppCore::themeMode() const
{
    return m_themeMode;
}

void AppCore::applyLanguage(const QString& languageCode)
{
    m_language = languageCode;
    QSettings s("Trackly", "Trackly");
    s.setValue("ui/language", languageCode);
}

QString AppCore::language() const
{
    return m_language;
}

bool AppCore::hasSavedSession() const
{
    return !m_accessToken.isEmpty() && !m_refreshToken.isEmpty() && !m_userId.isEmpty();
}

bool AppCore::isAuthenticated() const
{
    return m_authenticated && !m_accessToken.isEmpty() && !m_userId.isEmpty();
}

bool AppCore::isAccessTokenExpired(int skewSeconds) const
{
    if (m_accessToken.isEmpty() || !m_expiresAt.isValid()) {
        return true;
    }

    return QDateTime::currentDateTimeUtc().addSecs(skewSeconds) >= m_expiresAt.toUTC();
}

QString AppCore::accessToken() const
{
    return m_accessToken;
}

QString AppCore::refreshToken() const
{
    return m_refreshToken;
}

QString AppCore::currentUserId() const
{
    return m_userId;
}

QString AppCore::currentEmail() const
{
    return m_email;
}

void AppCore::loadSession()
{
    QSettings s("Trackly", "Trackly");
    m_accessToken = s.value("auth/access").toString();
    m_refreshToken = s.value("auth/refresh").toString();
    m_email = s.value("auth/email").toString();
    m_userId = s.value("auth/userId").toString();
    m_expiresAt = s.value("auth/expiresAt").toDateTime();
    m_authenticated = !m_accessToken.isEmpty() && !m_userId.isEmpty();
}

void AppCore::saveSession()
{
    QSettings s("Trackly", "Trackly");
    s.setValue("auth/access", m_accessToken);
    s.setValue("auth/refresh", m_refreshToken);
    s.setValue("auth/email", m_email);
    s.setValue("auth/userId", m_userId);
    s.setValue("auth/expiresAt", m_expiresAt);
    m_authenticated = !m_accessToken.isEmpty() && !m_userId.isEmpty();
}

void AppCore::clearSession()
{
    QSettings s("Trackly", "Trackly");
    s.remove("auth/access");
    s.remove("auth/refresh");
    s.remove("auth/email");
    s.remove("auth/userId");
    s.remove("auth/expiresAt");

    m_accessToken.clear();
    m_refreshToken.clear();
    m_email.clear();
    m_userId.clear();
    m_expiresAt = QDateTime();
    m_authenticated = false;

    m_cachedSeizures = QJsonArray();
    m_cachedMedications = QJsonArray();
    m_cachedExaminations = QJsonArray();
    m_cachedProfile = QJsonObject();
    m_cachedMedicationLogs = QJsonArray();
    m_cachedTriggers = QJsonArray();
}

bool AppCore::ensureConfigured() const
{
    return !m_supabaseUrl.isEmpty() && !m_anonKey.isEmpty();
}

QJsonArray AppCore::medicationsJson() const
{
    return m_cachedMedications;
}

QJsonArray AppCore::medicationLogsForDay(const QDate& date) const
{
    QJsonArray result;
    const QString day = date.toString("yyyy-MM-dd");

    for (const auto& v : m_cachedMedicationLogs) {
        const QJsonObject obj = v.toObject();
        const QString takenAt = obj.value("taken_at").toString();
        if (takenAt.startsWith(day)) {
            result.append(obj);
        }
    }
    return result;
}

void AppCore::cancelMedicationLog(const QString& logId)
{
    if (logId.isEmpty()) {
        emit networkError(tr("Не указан id приёма лекарства"));
        return;
    }

    withValidSession(tr("отменить приём лекарства"), [this, logId]() {
        QUrl url(m_supabaseUrl + "/rest/v1/medication_logs");
        QUrlQuery q;
        q.addQueryItem("id", "eq." + logId);
        url.setQuery(q);

        QNetworkRequest req(url);
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        if (!m_accessToken.isEmpty()) {
            req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
        }
        req.setRawHeader("Accept", "application/json");
        req.setRawHeader("Prefer", "return=representation");

        auto* reply = m_net->deleteResource(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, logId]() {
            const QByteArray raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(
                    raw.isEmpty() ? QByteArray("Не удалось отменить приём") : raw));
                return;
            }

            emit medicationLogCancelled(logId);
            emit saveSucceeded("medication_log_cancelled");
        });
    });
}

void AppCore::updateMedicationStock(const QString& id, int stock)
{
    if (id.isEmpty()) {
        emit networkError(tr("Не указан id лекарства для обновления остатка"));
        return;
    }

    withValidSession(tr("обновить остаток лекарства"), [this, id, stock]() {
        QUrl url(m_supabaseUrl + "/rest/v1/medications");
        QUrlQuery q;
        q.addQueryItem("id", "eq." + id);
        url.setQuery(q);

        QNetworkRequest req(url);
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        if (!m_accessToken.isEmpty()) {
            req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
        }
        req.setRawHeader("Accept", "application/json");
        req.setRawHeader("Prefer", "return=representation");
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject payload;
        payload["stock"] = stock;

        auto* reply = m_net->sendCustomRequest(
            req,
            "PATCH",
            QJsonDocument(payload).toJson(QJsonDocument::Compact)
            );

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QByteArray raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(
                    raw.isEmpty() ? QByteArray("Не удалось обновить остаток") : raw));
                return;
            }

            // перезагрузим список лекарств
            fetchMedications();
            emit saveSucceeded("medication_stock");
        });
    });
}

bool AppCore::ensureHasSession(const QString& actionName)
{
    if (!hasSavedSession()) {
        emit authFailed(tr("Сессия отсутствует. Войдите в аккаунт, чтобы %1").arg(actionName));
        emit signedOut();
        return false;
    }

    return true;
}

QList<QPair<QByteArray, QByteArray>> AppCore::authHeaders(bool json) const
{
    QList<QPair<QByteArray, QByteArray>> headers;
    headers.append(qMakePair(QByteArray("apikey"), m_anonKey.toUtf8()));

    if (!m_accessToken.isEmpty()) {
        headers.append(qMakePair(QByteArray("Authorization"),
                                 QByteArray("Bearer ") + m_accessToken.toUtf8()));
    }

    if (json) {
        headers.append(qMakePair(QByteArray("Accept"), QByteArray("application/json")));
        headers.append(qMakePair(QByteArray("Prefer"), QByteArray("return=representation")));
    }

    return headers;
}

QNetworkRequest AppCore::makeRequest(
    const QString& path,
    const QList<QPair<QByteArray, QByteArray>>& extraHeaders)
{
    QNetworkRequest req(QUrl(m_supabaseUrl + "/" + path));
    for (const auto& pair : extraHeaders) {
        req.setRawHeader(pair.first, pair.second);
    }
    return req;
}

void AppCore::markMedicationIntakePrn(const QString& medicationId,
                                      const QDateTime& takenAt,
                                      const QString& status)
{
    QJsonObject obj;
    obj["user_id"]        = m_userId;
    obj["medication_id"] = medicationId;
    obj["schedule_id"]   = QJsonValue::Null;  // PRN, без расписания
    obj["status"]        = status;
    obj["taken_at"]      = takenAt.toUTC().toString(Qt::ISODate);

    withValidSession(tr("отметки приёма по необходимости"), [this, obj]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("rest/v1/medication_logs", headers);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Prefer", "return=representation");

        auto* reply = m_net->post(
            req,
            QJsonDocument(obj).toJson(QJsonDocument::Compact));

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(
                    raw.isEmpty() ? QByteArray("Unknown error") : raw));
                return;
            }

            emit saveSucceeded("medication_log");
            fetchMedicationLogs();
        });
    });

}

void AppCore::setMedicationIntakeStatus(const QString& medicationId,
                                        const QString& scheduleId,
                                        const QDate& date,
                                        const QString& status)
{
    QJsonObject obj;
    obj["user_id"]        = m_userId;
    obj["medication_id"] = medicationId;
    obj["schedule_id"]   = scheduleId;
    obj["status"]        = status;
    obj["taken_at"]      = QDateTime(date, QTime::currentTime())
                          .toUTC()
                          .toString(Qt::ISODate);

    withValidSession(tr("обновления статуса приёма"), [this, obj]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("rest/v1/medication_logs", headers);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Prefer", "return=representation");

        auto* reply = m_net->post(
            req,
            QJsonDocument(obj).toJson(QJsonDocument::Compact));

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(
                    raw.isEmpty() ? QByteArray("Unknown error") : raw));
                return;
            }

            emit saveSucceeded("medication_log");
            fetchMedicationLogs();
        });
    });
}

void AppCore::deleteMedicationLog(const QString& id)
{
    withValidSession(tr("Удаление приёма лекарства"), [this, id]() {
        QUrl url(m_supabaseUrl + "/rest/v1/medication_logs");
        QUrlQuery q;
        q.addQueryItem("id", "eq." + id);
        url.setQuery(q);

        QNetworkRequest req(url);
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        if (!m_accessToken.isEmpty())
            req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
        req.setRawHeader("Accept", "application/json");
        req.setRawHeader("Prefer", "return=representation");

        auto* reply = m_net->sendCustomRequest(req, "DELETE", QByteArray());
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QByteArray raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Failed to delete medication log") : raw));
                return;
            }

            emit saveSucceeded("medicationlogdeleted");
            fetchMedicationLogs();
            fetchMedications();
        });
    });
}

void AppCore::updateMedicationLogTime(const QString& logId,
                                      const QDateTime& takenAt)
{
    if (logId.isEmpty())
        return;

    withValidSession(tr("обновления времени приёма"), [this, logId, takenAt]() {
        auto headers = authHeaders(true);

        // PATCH /rest/v1/medication_logs?id=eq.<logId>
        QNetworkRequest req = makeRequest(
            "/rest/v1/medication_logs?id=eq." + logId,
            headers);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject payload;
        payload["taken_at"] = takenAt.toUTC().toString(Qt::ISODate);

        auto* reply = m_net->sendCustomRequest(req, "PATCH",
                                               QJsonDocument(payload).toJson(QJsonDocument::Compact));

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QByteArray raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(
                    raw.isEmpty() ? QByteArray("Unknown error") : raw));
                return;
            }

            emit saveSucceeded("medication_log");
            fetchMedicationLogs();
        });
    });
}

void AppCore::refreshSession(std::function<void(bool)> callback)
{
    if (m_refreshToken.isEmpty()) {
        clearSession();
        emit signedOut();
        callback(false);
        return;
    }

    m_pendingSessionCallbacks.append(callback);

    if (m_refreshInProgress) {
        return;
    }

    m_refreshInProgress = true;

    QNetworkRequest req(QUrl(m_supabaseUrl + "/auth/v1/token?grant_type=refresh_token"));
    req.setRawHeader("apikey", m_anonKey.toUtf8());
    req.setRawHeader("Content-Type", "application/json");
    req.setRawHeader("Accept", "application/json");

    QJsonObject payload{
        {"refresh_token", m_refreshToken}
    };

    auto* reply = m_net->post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray raw = reply->readAll();
        const auto err = reply->error();
        reply->deleteLater();

        bool ok = false;

        if (err == QNetworkReply::NoError) {
            const auto obj = QJsonDocument::fromJson(raw).object();

            const QString newAccess = obj.value("access_token").toString();
            const QString newRefresh = obj.value("refresh_token").toString();
            const int expiresIn = obj.value("expires_in").toInt(0);
            const QString newUserId = obj.value("user").toObject().value("id").toString();

            if (!newAccess.isEmpty() && !newRefresh.isEmpty()) {
                m_accessToken = newAccess;
                m_refreshToken = newRefresh;

                if (!newUserId.isEmpty()) {
                    m_userId = newUserId;
                }

                m_expiresAt = expiresIn > 0
                                  ? QDateTime::currentDateTimeUtc().addSecs(expiresIn)
                                  : QDateTime();

                saveSession();
                ok = true;
            }
        }

        m_refreshInProgress = false;

        const auto callbacks = m_pendingSessionCallbacks;
        m_pendingSessionCallbacks.clear();

        if (!ok) {
            clearSession();
            emit signedOut();
        }

        for (const auto& cb : callbacks) {
            cb(ok);
        }
    });
}

void AppCore::withValidSession(const QString& actionName, std::function<void()> onReady)
{
    if (!ensureConfigured()) {
        emit authFailed(tr("Supabase не настроен: вставьте Project URL и anon key"));
        return;
    }

    if (!ensureHasSession(actionName)) {
        return;
    }

    if (!isAccessTokenExpired()) {
        onReady();
        return;
    }

    refreshSession([this, actionName, onReady](bool ok) {
        if (!ok) {
            emit authFailed(tr("Сессия истекла. Войдите снова, чтобы %1").arg(actionName));
            return;
        }
        onReady();
    });
}

void AppCore::signIn(const QString& email, const QString& password)
{
    if (!ensureConfigured()) {
        emit authFailed(tr("Supabase не настроен: вставьте Project URL и anon key"));
        return;
    }

    if (email.trimmed().isEmpty() || password.isEmpty()) {
        emit authFailed(tr("Введите email и пароль"));
        return;
    }

    QNetworkRequest req(QUrl(m_supabaseUrl + "/auth/v1/token?grant_type=password"));
    req.setRawHeader("apikey", m_anonKey.toUtf8());
    req.setRawHeader("Content-Type", "application/json");
    req.setRawHeader("Accept", "application/json");

    QJsonObject payload{
        {"email", email.trimmed()},
        {"password", password}
    };

    auto* reply = m_net->post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::finished, this, [this, reply, email]() {
        const QByteArray raw = reply->readAll();
        const auto err = reply->error();
        reply->deleteLater();

        if (err != QNetworkReply::NoError) {
            emit authFailed(QString::fromUtf8(raw.isEmpty() ? QByteArray("Ошибка входа") : raw));
            return;
        }

        const auto obj = QJsonDocument::fromJson(raw).object();

        m_accessToken = obj.value("access_token").toString();
        m_refreshToken = obj.value("refresh_token").toString();
        m_email = email.trimmed();
        m_userId = obj.value("user").toObject().value("id").toString();

        const int expiresIn = obj.value("expires_in").toInt(0);
        m_expiresAt = expiresIn > 0
                          ? QDateTime::currentDateTimeUtc().addSecs(expiresIn)
                          : QDateTime();

        if (m_accessToken.isEmpty() || m_refreshToken.isEmpty() || m_userId.isEmpty()) {
            clearSession();
            emit authFailed(tr("Не удалось получить рабочую сессию"));
            return;
        }

        saveSession();
        emit authSucceeded();
    });
}

void AppCore::upsertProfileAfterSignup(const QString& name)
{
    if (!isAuthenticated()) {
        return;
    }

    QJsonObject profile{
        {"id", m_userId},
        {"full_name", name}
    };

    updateProfile(profile);
}

void AppCore::signUp(const QString& email, const QString& password, const QString& name)
{
    if (!ensureConfigured()) {
        emit authFailed(tr("Supabase не настроен: вставьте Project URL и anon key"));
        return;
    }

    if (email.trimmed().isEmpty() || password.isEmpty() || name.trimmed().isEmpty()) {
        emit authFailed(tr("Заполните имя, email и пароль"));
        return;
    }

    QNetworkRequest req(QUrl(m_supabaseUrl + "/auth/v1/signup"));
    req.setRawHeader("apikey", m_anonKey.toUtf8());
    req.setRawHeader("Content-Type", "application/json");
    req.setRawHeader("Accept", "application/json");

    QJsonObject payload{
        {"email", email.trimmed()},
        {"password", password},
        {"data", QJsonObject{{"full_name", name.trimmed()}}}
    };

    auto* reply = m_net->post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::finished, this, [this, reply, email, name]() {
        const QByteArray raw = reply->readAll();
        const auto err = reply->error();
        reply->deleteLater();

        if (err != QNetworkReply::NoError) {
            emit authFailed(QString::fromUtf8(raw.isEmpty() ? QByteArray("Ошибка регистрации") : raw));
            return;
        }

        const auto obj = QJsonDocument::fromJson(raw).object();

        m_accessToken = obj.value("access_token").toString();
        m_refreshToken = obj.value("refresh_token").toString();
        m_email = email.trimmed();
        m_userId = obj.value("user").toObject().value("id").toString();

        const int expiresIn = obj.value("expires_in").toInt(0);
        m_expiresAt = expiresIn > 0
                          ? QDateTime::currentDateTimeUtc().addSecs(expiresIn)
                          : QDateTime();

        if (m_accessToken.isEmpty() || m_refreshToken.isEmpty() || m_userId.isEmpty()) {
            clearSession();
            emit authFailed(tr("Регистрация завершена. Подтвердите email и затем войдите вручную."));
            return;
        }

        saveSession();
        upsertProfileAfterSignup(name.trimmed());
        emit authSucceeded();
    });
}

void AppCore::signOut()
{
    clearSession();
    emit signedOut();
}

QString AppCore::generateNormalizedFileName(int fileIndex)
{
    // Просто file_1, file_2, file_3 и т.д.
    return QString("file_%1").arg(fileIndex);
}

void AppCore::rebuildDashboard()
{
    QJsonObject obj;

    obj["seizures_this_month"] = m_cachedSeizures.size();
    obj["meds_today"] = m_cachedMedications.size();
    obj["wellbeing_count"] = m_cachedTriggers.size();

    if (!m_cachedExaminations.isEmpty()) {
        const auto exam = m_cachedExaminations.first().toObject();
        obj["next_exam"] = exam.value("planned_at").toString();
    } else {
        obj["next_exam"] = "";
    }

    if (!m_cachedProfile.isEmpty()) {
        const QString doctor = m_cachedProfile.value("doctor_name").toString().trimmed();
        obj["doctor"] = doctor.isEmpty() ? tr("Не указан") : doctor;
    } else {
        obj["doctor"] = tr("Не указан");
    }

    if (!m_cachedTriggers.isEmpty()) {
        double sumMood = 0.0;
        int countMood = 0;

        for (const auto& v : m_cachedTriggers) {
            const auto o = v.toObject();
            const int mood = o.value("mood").toInt(0);
            if (mood > 0) {
                sumMood += mood;
                ++countMood;
            }
        }

        obj["avg_mood"] = countMood > 0 ? (sumMood / countMood) : 0.0;
    } else {
        obj["avg_mood"] = 0.0;
    }

    emit dashboardReady(obj);
}

void AppCore::fetchDashboard()
{
    rebuildDashboard();

    if (!hasSavedSession()) {
        return;
    }

    fetchSeizures();
    fetchMedications();
    fetchExaminations();
    fetchProfile();
    fetchMedicationLogs();
    fetchTriggers();
}
void AppCore::fetchSeizures()
{
    withValidSession(tr("загрузить приступы"), [this]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("/rest/v1/seizures?select=*&order=started_at.desc", headers);

        auto* reply = m_net->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Ошибка загрузки приступов") : raw));
                return;
            }

            m_cachedSeizures = QJsonDocument::fromJson(raw).array();
            emit seizuresReady(m_cachedSeizures);
            rebuildDashboard();
        });
    });
}

void AppCore::fetchMedications()
{
    withValidSession(tr("загрузить лекарства"), [this]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("/rest/v1/medications?select=*&order=created_at.desc", headers);

        auto* reply = m_net->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Ошибка загрузки лекарств") : raw));
                return;
            }

            m_cachedMedications = QJsonDocument::fromJson(raw).array();
            emit medicationsReady(m_cachedMedications);
            rebuildDashboard();
        });
    });
}

void AppCore::fetchMedicationSchedules()
{
    withValidSession(tr("загрузить расписания приёмов"), [this]() {
        QUrl url(m_supabaseUrl + "/rest/v1/medication_schedules");
        QUrlQuery q;
        q.addQueryItem("select", "*");
        q.addQueryItem("user_id", "eq." + m_userId);
        url.setQuery(q);

        QNetworkRequest req(url);
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        if (!m_accessToken.isEmpty()) {
            req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
        }
        req.setRawHeader("Accept", "application/json");
        req.setRawHeader("Prefer", "return=representation");

        auto* reply = m_net->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QByteArray raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Не удалось загрузить расписания") : raw));
                return;
            }

            const auto doc = QJsonDocument::fromJson(raw);
            if (!doc.isArray()) {
                emit networkError(tr("Некорректный ответ при загрузке расписаний"));
                return;
            }

            const QJsonArray arr = doc.array();
            emit medicationSchedulesLoaded(arr);
        });
    });
}

void AppCore::createMedicationSchedule(const QJsonObject& obj)
{
    QJsonObject payload = obj;
    payload["user_id"] = m_userId;

    withValidSession(tr("создать расписание приёма"), [this, payload]() {
        QUrl url(m_supabaseUrl + "/rest/v1/medication_schedules");
        QUrlQuery q;
        q.addQueryItem("select", "id");
        url.setQuery(q);

        QNetworkRequest req(url);
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        if (!m_accessToken.isEmpty()) {
            req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
        }
        req.setRawHeader("Accept", "application/json");
        req.setRawHeader("Prefer", "return=representation");
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        auto* reply = m_net->post(
            req,
            QJsonDocument(payload).toJson(QJsonDocument::Compact)
            );

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QByteArray raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Не удалось создать расписание") : raw));
                return;
            }

            const auto doc = QJsonDocument::fromJson(raw);
            if (!doc.isArray() || doc.array().isEmpty()) {
                emit networkError(tr("Не удалось получить id расписания"));
                return;
            }

            const QString id = doc.array().first().toObject().value("id").toString();
            emit saveSucceeded("medication_schedule");
            emit medicationLogCreated(id); // можно другой сигнал, если нужно
        });
    });
}

void AppCore::markMedicationIntake(const QString& medicationId,
                                   const QString& scheduleId,
                                   const QString& status)
{
    if (medicationId.isEmpty() || scheduleId.isEmpty()) {
        emit networkError(tr("Отсутствует medicationId или scheduleId"));
        return;
    }

    const QString normalizedStatus = (status == "missed") ? "missed" : "taken";

    QJsonObject payload{
        {"user_id", m_userId},
        {"medication_id", medicationId},
        {"schedule_id", scheduleId},
        {"taken_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"status", normalizedStatus}
    };

    withValidSession(tr("отметить приём лекарства"), [this, payload]() {
        QUrl url(m_supabaseUrl + "/rest/v1/medication_logs");
        QUrlQuery q;
        q.addQueryItem("select", "id");
        url.setQuery(q);

        QNetworkRequest req(url);
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        if (!m_accessToken.isEmpty()) {
            req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
        }
        req.setRawHeader("Accept", "application/json");
        req.setRawHeader("Prefer", "return=representation");
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        auto* reply = m_net->post(
            req,
            QJsonDocument(payload).toJson(QJsonDocument::Compact)
            );

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QByteArray raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Не удалось отметить приём") : raw));
                return;
            }

            const auto doc = QJsonDocument::fromJson(raw);
            if (!doc.isArray() || doc.array().isEmpty()) {
                emit networkError(tr("Не удалось получить id приёма"));
                return;
            }

            const QString id = doc.array().first().toObject().value("id").toString();
            emit medicationLogCreated(id);
            emit saveSucceeded("medication_log");
        });
    });
}

void AppCore::fetchEmergencyContacts()
{
    withValidSession(tr("загрузить экстренные контакты"), [this]() {
        QUrl url(m_supabaseUrl + "/rest/v1/emergency_contacts");
        QUrlQuery q;
        q.addQueryItem("select", "*");
        q.addQueryItem("user_id", "eq." + m_userId);
        url.setQuery(q);

        QNetworkRequest req(url);
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        if (!m_accessToken.isEmpty()) {
            req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
        }
        req.setRawHeader("Accept", "application/json");
        req.setRawHeader("Prefer", "return=representation");

        auto* reply = m_net->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QByteArray raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Не удалось загрузить экстренные контакты") : raw));
                return;
            }

            const auto doc = QJsonDocument::fromJson(raw);
            if (!doc.isArray()) {
                emit networkError(tr("Некорректный ответ при загрузке экстренных контактов"));
                return;
            }

            emit emergencyContactsLoaded(doc.array());
        });
    });
}

void AppCore::createEmergencyContact(const QJsonObject& obj)
{
    QJsonObject payload = obj;
    payload["user_id"] = m_userId;

    withValidSession(tr("добавить экстренный контакт"), [this, payload]() {
        QUrl url(m_supabaseUrl + "/rest/v1/emergency_contacts");
        QUrlQuery q;
        q.addQueryItem("select", "id,email,full_name,relation,created_at");
        url.setQuery(q);

        QNetworkRequest req(url);
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        if (!m_accessToken.isEmpty()) {
            req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
        }
        req.setRawHeader("Accept", "application/json");
        req.setRawHeader("Prefer", "return=representation");
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        auto* reply = m_net->post(
            req,
            QJsonDocument(payload).toJson(QJsonDocument::Compact)
            );

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QByteArray raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Не удалось добавить контакт") : raw));
                return;
            }

            const auto doc = QJsonDocument::fromJson(raw);
            if (!doc.isArray() || doc.array().isEmpty()) {
                emit networkError(tr("Не удалось получить id экстренного контакта"));
                return;
            }

            const auto obj = doc.array().first().toObject();
            const QString id = obj.value("id").toString();

            emit emergencyContactCreated(id);
            emit saveSucceeded("emergency_contact");
        });
    });
}

void AppCore::deleteEmergencyContact(const QString& id)
{
    if (id.isEmpty()) {
        emit networkError(tr("Не указан id экстренного контакта"));
        return;
    }

    withValidSession(tr("удалить экстренный контакт"), [this, id]() {
        QUrl url(m_supabaseUrl + "/rest/v1/emergency_contacts");
        QUrlQuery q;
        q.addQueryItem("id", "eq." + id);
        url.setQuery(q);

        QNetworkRequest req(url);
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        if (!m_accessToken.isEmpty()) {
            req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
        }
        req.setRawHeader("Accept", "application/json");
        req.setRawHeader("Prefer", "return=representation");

        auto* reply = m_net->deleteResource(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, id]() {
            const QByteArray raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Не удалось удалить контакт") : raw));
                return;
            }

            emit emergencyContactDeleted(id);
            emit saveSucceeded("emergency_contact_deleted");
        });
    });
}



void AppCore::fetchExaminations()
{
    withValidSession(tr("загрузить обследования"), [this]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("/rest/v1/examinations?select=*&order=planned_at.asc", headers);

        auto* reply = m_net->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Ошибка загрузки обследований") : raw));
                return;
            }

            m_cachedExaminations = QJsonDocument::fromJson(raw).array();
            emit examinationsReady(m_cachedExaminations);
            rebuildDashboard();
        });
    });
}

void AppCore::fetchAttachments()
{
    withValidSession(tr("загрузки вложений"), [this]() {
        QUrl url(m_supabaseUrl + "/rest/v1/attachments");
        QUrlQuery q;
        q.addQueryItem("user_id", "eq." + m_userId);
        q.addQueryItem("select", "*");
        url.setQuery(q);

        QNetworkRequest req(url);
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        if (!m_accessToken.isEmpty())
            req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
        req.setRawHeader("Accept", "application/json");

        auto* reply = m_net->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QByteArray raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(
                    raw.isEmpty() ? QByteArray("Не удалось загрузить вложения") : raw));
                return;
            }

            m_cachedAttachments = QJsonDocument::fromJson(raw).array();
            emit attachmentsReady(m_cachedAttachments);
        });
    });
}

QString AppCore::makePublicAttachmentUrl(const QString& storagePath) const
{
    // medical_files должен быть Public
    return m_supabaseUrl + "/storage/v1/object/public/medical_files/" + storagePath;
}


void AppCore::fetchProfile()
{
    withValidSession(tr("загрузить профиль"), [this]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("/rest/v1/profiles?id=eq." + m_userId + "&select=*", headers);

        auto* reply = m_net->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Ошибка загрузки профиля") : raw));
                return;
            }

            const auto arr = QJsonDocument::fromJson(raw).array();

            if (!arr.isEmpty()) {
                m_cachedProfile = arr.first().toObject();
            } else {
                m_cachedProfile = QJsonObject{
                    {"full_name", ""},
                    {"birth_date", ""},
                    {"emergency_phone", ""},
                    {"doctor_name", ""}
                };
            }

            emit profileReady(m_cachedProfile);
            rebuildDashboard();
        });
    });
}

void AppCore::fetchMedicationLogs()
{
    withValidSession(tr("загрузить логи приема лекарств"), [this]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest(
            "/rest/v1/medication_logs?select=*&order=taken_at.desc",
            headers
            );

        auto* reply = m_net->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(
                    raw.isEmpty() ? QByteArray("Ошибка загрузки логов приема") : raw
                    ));
                return;
            }

            m_cachedMedicationLogs = QJsonDocument::fromJson(raw).array();
            emit medicationLogsReady(m_cachedMedicationLogs);
        });
    });
}

void AppCore::createSeizure(const QJsonObject& obj)
{
    withValidSession(tr("сохранить приступ"), [this, obj]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("/rest/v1/seizures", headers);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        auto* reply = m_net->post(req, QJsonDocument(obj).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Ошибка сохранения приступа") : raw));
                return;
            }

            emit saveSucceeded("seizure");
            fetchSeizures();
        });
    });
}

void AppCore::fetchTriggers()
{
    withValidSession(tr("загрузить записи самочувствия"), [this]() {
        QUrl url(m_supabaseUrl + "/rest/v1/wellbeing_entries");
        QUrlQuery query;
        query.addQueryItem("select", "*");
        query.addQueryItem("user_id", "eq." + m_userId);
        query.addQueryItem("order", "logged_at.desc");
        url.setQuery(query);

        QNetworkRequest req(url);
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
        req.setRawHeader("Accept", "application/json");
        req.setRawHeader("Prefer", "return=representation");
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        auto* reply = m_net->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QByteArray raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(
                    raw.isEmpty() ? QByteArray("Ошибка загрузки самочувствия") : raw));
                return;
            }

            m_cachedTriggers = QJsonDocument::fromJson(raw).array();
            emit triggersReady(m_cachedTriggers);
            rebuildDashboard();
        });
    });
}

void AppCore::createTrigger(const QJsonObject& obj)
{
    withValidSession(tr("сохранить запись самочувствия"), [this, obj]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("/rest/v1/wellbeing_entries", headers);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        auto* reply = m_net->post(req, QJsonDocument(obj).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(
                    raw.isEmpty() ? QByteArray("Ошибка сохранения самочувствия") : raw));
                return;
            }

            emit saveSucceeded("trigger");
            fetchTriggers();
        });
    });
}

void AppCore::deleteTrigger(const QString& id)
{
    withValidSession(tr("удалить запись самочувствия"), [this, id]() {
        QUrl url(m_supabaseUrl + "/rest/v1/wellbeing_entries");
        QUrlQuery query;
        query.addQueryItem("id", "eq." + id);
        url.setQuery(query);

        QNetworkRequest req(url);
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
        req.setRawHeader("Accept", "application/json");
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        auto* reply = m_net->sendCustomRequest(req, "DELETE", QByteArray());
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(
                    raw.isEmpty() ? QByteArray("Ошибка удаления записи самочувствия") : raw));
                return;
            }

            emit saveSucceeded("trigger_deleted");
            fetchTriggers();
        });
    });
}



void AppCore::createMedication(const QJsonObject& obj)
{
    withValidSession(tr("сохранения лекарства"), [this, obj]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("rest/v1/medications", headers);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Prefer", "return=representation");

        auto* reply = m_net->post(
            req,
            QJsonDocument(obj).toJson(QJsonDocument::Compact));

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                qDebug() << "createMedication error:" << err << raw;
                emit networkError(QString::fromUtf8(
                    raw.isEmpty() ? QByteArray("Unknown error") : raw));
                return;
            }

            QJsonObject createdMed;
            const auto doc = QJsonDocument::fromJson(raw);
            if (doc.isArray() && !doc.array().isEmpty())
                createdMed = doc.array().first().toObject();
            else if (doc.isObject())
                createdMed = doc.object();

            emit saveSucceeded("medication");
            if (!createdMed.isEmpty())
                emit medicationCreated(createdMed);

            fetchMedications();
            fetchMedicationSchedules();
        });
    });
}

void AppCore::createMedicationLog(const QJsonObject& obj)
{
    withValidSession(tr("Создание приёма лекарства"), [this, obj]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("rest/v1/medication_logs", headers);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        qDebug() << "POST medication_logs" << req.url();

        auto* reply = m_net->post(req, QJsonDocument(obj).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Ошибка сохранения лога приема") : raw));
                return;
            }

            emit saveSucceeded("medication_log");
            fetchMedicationLogs();
        });
    });
}

void AppCore::createExamination(const QJsonObject& obj)
{
    withValidSession(tr("создания обследования"), [this, obj]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("rest/v1/examinations", headers);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Prefer", "return=representation");

        QJsonObject payload = obj;
        payload["user_id"] = m_userId;

        auto* reply = m_net->post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QByteArray raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(
                    raw.isEmpty() ? QByteArray("Не удалось создать обследование") : raw));
                return;
            }

            const QJsonDocument doc = QJsonDocument::fromJson(raw);
            QJsonObject created;
            if (doc.isArray() && !doc.array().isEmpty())
                created = doc.array().first().toObject();
            else if (doc.isObject())
                created = doc.object();

            emit saveSucceeded("examination");
            if (!created.isEmpty())
                emit examinationCreated(created);

            fetchExaminations();
        });
    });
}

void AppCore::updateSeizure(const QString& id, const QJsonObject& obj)
{
    withValidSession(tr("обновить приступ"), [this, id, obj]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("/rest/v1/seizures?id=eq." + id, headers);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        auto* reply = m_net->sendCustomRequest(
            req,
            "PATCH",
            QJsonDocument(obj).toJson(QJsonDocument::Compact)
            );

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Ошибка обновления приступа") : raw));
                return;
            }

            emit saveSucceeded("seizure_updated");
            fetchSeizures();
        });
    });
}

void AppCore::deleteSeizure(const QString& id)
{
    withValidSession(tr("удалить приступ"), [this, id]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("/rest/v1/seizures?id=eq." + id, headers);

        auto* reply = m_net->sendCustomRequest(req, "DELETE", QByteArray());

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Ошибка удаления приступа") : raw));
                return;
            }

            emit saveSucceeded("seizure_deleted");
            fetchSeizures();
        });
    });
}

void AppCore::updateMedication(const QString& id, const QJsonObject& obj)
{
    if (id.isEmpty()) {
        emit networkError(tr("Не указан id препарата для обновления"));
        return;
    }

    QJsonObject payload = obj;

    withValidSession(tr("обновить препарат"), [this, id, payload]() {
        QUrl url(m_supabaseUrl + "/rest/v1/medications");
        QUrlQuery q;
        q.addQueryItem("id", "eq." + id);
        url.setQuery(q);

        QNetworkRequest req(url);
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        if (!m_accessToken.isEmpty()) {
            req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
        }
        req.setRawHeader("Accept", "application/json");
        req.setRawHeader("Prefer", "return=representation");
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");


        // если у тебя нет метода patch, используй sendCustomRequest:
        auto* reply = m_net->sendCustomRequest(
            req, "PATCH",
            QJsonDocument(payload).toJson(QJsonDocument::Compact)
        );

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QByteArray raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(
                    raw.isEmpty() ? QByteArray("Не удалось обновить препарат") : raw));
                return;
            }

            emit saveSucceeded("medication");
            fetchMedications();
        });
    });
}

void AppCore::deleteMedication(const QString& id)
{
    withValidSession(tr("удалить лекарство"), [this, id]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("/rest/v1/medications?id=eq." + id, headers);

        auto* reply = m_net->sendCustomRequest(req, "DELETE", QByteArray());

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Ошибка удаления лекарства") : raw));
                return;
            }

            emit saveSucceeded("medication_deleted");
            fetchMedications();
        });
    });
}

void AppCore::updateExamination(const QString& id, const QJsonObject& obj)
{
    withValidSession(tr("обновить обследование"), [this, id, obj]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("/rest/v1/examinations?id=eq." + id, headers);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        auto* reply = m_net->sendCustomRequest(
            req,
            "PATCH",
            QJsonDocument(obj).toJson(QJsonDocument::Compact)
            );

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Ошибка обновления обследования") : raw));
                return;
            }

            emit saveSucceeded("examination_updated");
            fetchExaminations();
        });
    });
}

void AppCore::deleteExamination(const QString& id)
{
    withValidSession(tr("удалить обследование"), [this, id]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("/rest/v1/examinations?id=eq." + id, headers);

        auto* reply = m_net->sendCustomRequest(req, "DELETE", QByteArray());

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Ошибка удаления обследования") : raw));
                return;
            }

            emit saveSucceeded("examination_deleted");
            fetchExaminations();
        });
    });
}

void AppCore::updateProfile(const QJsonObject& obj)
{
    withValidSession(tr("обновить профиль"), [this, obj]() {
        auto headers = authHeaders(true);
        QNetworkRequest req = makeRequest("/rest/v1/profiles?on_conflict=id", headers);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Prefer", "resolution=merge-duplicates,return=representation");

        QJsonObject payload = obj;
        payload["id"] = m_userId;
        payload["email"] = m_email;

        auto* reply = m_net->post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(raw.isEmpty() ? QByteArray("Ошибка обновления профиля") : raw));
                return;
            }

            const auto arr = QJsonDocument::fromJson(raw).array();
            if (!arr.isEmpty()) {
                m_cachedProfile = arr.first().toObject();
                emit profileReady(m_cachedProfile);
            }

            emit saveSucceeded("profile");
            fetchProfile();
        });
    });
}

void AppCore::deleteAttachment(const QString& attachmentId)
{
    if (attachmentId.isEmpty()) {
        emit networkError(tr("Не указан id вложения"));
        return;
    }

    withValidSession(tr("удалить вложение"), [this, attachmentId]() {
        QUrl url(m_supabaseUrl + "/rest/v1/attachments");
        QUrlQuery q;
        q.addQueryItem("id", "eq." + attachmentId);
        url.setQuery(q);

        QNetworkRequest req(url);
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        if (!m_accessToken.isEmpty()) {
            req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
        }
        req.setRawHeader("Accept", "application/json");

        auto* reply = m_net->deleteResource(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                emit networkError(tr("Не удалось удалить вложение"));
                return;
            }
            emit saveSucceeded("attachment_deleted");
            fetchAttachments();  // обновляем список
        });
    });
}

static QString normalizeFileName(const QString& name)
{
    QString base = name;
    // убираем путь
    int slash = base.lastIndexOf('/');
#ifdef Q_OS_WIN
    int backslash = base.lastIndexOf('\\');
    slash = qMax(slash, backslash);
#endif
    if (slash >= 0)
        base = base.mid(slash + 1);

    // 🔥 НЕ ТРОГАЕМ то, что уже нормальное
    QString out;
    out.reserve(base.size());
    for (QChar ch : base) {
        // Разрешаем русские буквы, латиницу, цифры, точки, подчёркивания, дефисы
        if (ch.isLetterOrNumber() || ch == '.' || ch == '_' || ch == '-') {
            out.append(ch);
        } else {
            out.append('_');
        }
    }

    if (out.isEmpty())
        out = "file.bin";

    return out;
}

void AppCore::uploadAttachment(const QString& localPath,
                               const QString& storagePath,
                               const QString& examinationId)
{
    if (m_userId.isEmpty()) {
        emit networkError(tr("Невозможно сохранить вложение: не задан user_id"));
        return;
    }

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit networkError(tr("Не удалось открыть файл: %1").arg(localPath));
        return;
    }
    const QByteArray data = file.readAll();
    file.close();

    const QString normalizedName = normalizeFileName(QFileInfo(storagePath).fileName());
    const QString baseDir = QFileInfo(storagePath).path();
    const QString normalizedKey = baseDir.isEmpty() ? normalizedName : baseDir + "/" + normalizedName;

    qDebug() << "UPLOADING to storage:" << normalizedKey;
    qDebug() << "Storage URL:" << m_supabaseUrl + "/storage/v1/object/medical_files/" + normalizedKey;

    withValidSession(tr("загрузки файла обследования"), [this, data, normalizedKey, examinationId]() {
        // ✅ Исправленный URL - без /public/ для bucket "medical_files"
        QUrl url(m_supabaseUrl + "/storage/v1/object/medical_files/" + normalizedKey);
        QNetworkRequest req(url);
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        if (!m_accessToken.isEmpty())
            req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");

        auto* reply = m_net->post(req, data);
        connect(reply, &QNetworkReply::finished, this, [this, reply, normalizedKey, examinationId]() {
            const QByteArray raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            qDebug() << "STORAGE RESPONSE error:" << err;
            qDebug() << "STORAGE RESPONSE data:" << raw;

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(
                    raw.isEmpty() ? QByteArray("Не удалось загрузить файл в хранилище") : raw));
                return;
            }

            // Сохраняем метаданные в таблицу attachments
            QJsonObject attachment;
            attachment["user_id"]        = m_userId;
            attachment["examination_id"] = examinationId;
            attachment["storage_path"]   = normalizedKey;
            attachment["bucket"]         = QStringLiteral("medical_files");
            attachment["file_name"]      = QFileInfo(normalizedKey).fileName();

            auto headers = authHeaders(true);
            QNetworkRequest req2 = makeRequest("rest/v1/attachments", headers);
            req2.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

            qDebug() << "SENDING ATTACHMENT METADATA:" << attachment;

            auto* reply2 = m_net->post(req2, QJsonDocument(attachment).toJson(QJsonDocument::Compact));
            connect(reply2, &QNetworkReply::finished, this, [this, reply2]() {
                const QByteArray raw2 = reply2->readAll();
                const auto err2 = reply2->error();
                reply2->deleteLater();

                qDebug() << "ATTACHMENT METADATA RESPONSE error:" << err2;
                qDebug() << "ATTACHMENT METADATA RESPONSE data:" << raw2;

                if (err2 != QNetworkReply::NoError) {
                    emit networkError(QString::fromUtf8(
                        raw2.isEmpty() ? QByteArray("Не удалось сохранить метаданные файла") : raw2));
                    return;
                }

                emit saveSucceeded("attachment");
                fetchAttachments();
            });
        });
    });
}



void AppCore::sendEmergencyEmail(const QString& to,
                                 const QString& patient,
                                 const QString& text,
                                 const QString& subject)
{
    withValidSession(tr("отправить экстренное письмо"), [this, to, patient, text, subject]() {
        if (to.trimmed().isEmpty()) {
            emit networkError(tr("Не указан email экстренного контакта"));
            return;
        }

        QNetworkRequest req(QUrl(m_supabaseUrl + "/functions/v1/emergency-email"));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("apikey", m_anonKey.toUtf8());
        req.setRawHeader("Authorization", "Bearer " + m_accessToken.toUtf8());

        QJsonObject payload{
            {"to", to.trimmed()},
            {"patient", patient.trimmed()},
            {"text", text.trimmed()},
            {"subject", subject.trimmed().isEmpty()
                            ? tr("Экстренное уведомление из Trackly")
                            : subject.trimmed()}
        };

        auto* reply = m_net->post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto raw = reply->readAll();
            const auto err = reply->error();
            reply->deleteLater();

            if (err != QNetworkReply::NoError) {
                emit networkError(QString::fromUtf8(
                    raw.isEmpty() ? QByteArray("Ошибка отправки экстренного email") : raw
                    ));
                return;
            }

            emit saveSucceeded("emergency_email");
        });
    });
}
