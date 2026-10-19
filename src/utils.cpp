#include "utils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QLayout>
#include <QWidget>
#include <QLayoutItem>
#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>

QString formatIsoToLocal(const QString& iso)
{
    if (iso.trimmed().isEmpty()) {
        return QString();
    }

    QDateTime dt = QDateTime::fromString(iso, Qt::ISODate);
    if (!dt.isValid()) {
        dt = QDateTime::fromString(iso, Qt::ISODateWithMs);
    }
    if (!dt.isValid()) {
        return iso;
    }

    return dt.toLocalTime().toString("dd.MM.yyyy HH:mm");
}

static void clearLayoutHelper(QLayout* layout)
{
    if (!layout) {
        return;
    }

    while (QLayoutItem* item = layout->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        if (item->layout()) {
            clearLayoutHelper(item->layout());
            delete item->layout();
        }
        delete item;
    }
}

void clearLayout(QLayout* layout)
{
    clearLayoutHelper(layout);
}

QString prettifyError(const QString& raw)
{
    QString s = raw.trimmed();
    if (s.isEmpty()) {
        return QObject::tr("Произошла ошибка.");
    }

    // Пробуем распарсить JSON — вытащить error / message / msg
    if (s.startsWith('{')) {
        const auto doc = QJsonDocument::fromJson(s.toUtf8());
        if (doc.isObject()) {
            const auto obj = doc.object();
            QString msg = obj.value("error_description").toString();
            if (msg.isEmpty()) msg = obj.value("error").toString();
            if (msg.isEmpty()) msg = obj.value("message").toString();
            if (msg.isEmpty()) msg = obj.value("msg").toString();
            if (!msg.isEmpty()) {
                s = msg;
            }
        }
    }

    s.replace("\\n", " ");
    s.replace("\\\"", "\"");
    s.replace(QRegularExpression("\\s+"), " ");
    s = s.trimmed();

    if (s.contains("Invalid login credentials", Qt::CaseInsensitive))
        return QObject::tr("Неверный email или пароль.");
    if (s.contains("Email not confirmed", Qt::CaseInsensitive))
        return QObject::tr("Подтверди email перед входом.");
    if (s.contains("User already registered", Qt::CaseInsensitive))
        return QObject::tr("Пользователь с таким email уже зарегистрирован.");
    if (s.contains("duplicate key", Qt::CaseInsensitive))
        return QObject::tr("Такая запись уже существует.");
    if (s.contains("JWT", Qt::CaseInsensitive) || s.contains("token", Qt::CaseInsensitive))
        return QObject::tr("Сессия истекла. Войди снова.");
    if (s.contains("row-level security", Qt::CaseInsensitive))
        return QObject::tr("Нет доступа к этой операции. Проверь RLS policy в Supabase.");
    if (s.contains("violates foreign key constraint", Qt::CaseInsensitive))
        return QObject::tr("Связанная запись не найдена. Проверь данные в базе.");
    if (s.contains("Could not find the", Qt::CaseInsensitive))
        return QObject::tr("В Supabase отсутствует нужная таблица или колонка.");
    if (s.contains("Failed to fetch", Qt::CaseInsensitive) || s.contains("Network", Qt::CaseInsensitive))
        return QObject::tr("Не удалось выполнить запрос. Проверь интернет и настройки Supabase.");

    return s.left(240);
}

// ─── Локальная конфигурация ─────────────────────────────────────────────────

// Читает trackly.local.json из рабочей папки или рядом с исполняемым файлом.
static QJsonObject loadLocalConfig()
{
    const QStringList candidates = {
        QDir::current().filePath(QStringLiteral("trackly.local.json")),
        QCoreApplication::applicationDirPath() + QStringLiteral("/trackly.local.json")
    };

    for (const QString& path : candidates) {
        QFile file(path);
        if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
            continue;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject()) {
            return doc.object();
        }
    }
    return {};
}

QString tracklyConfigValue(const char* envName,
                           const QString& jsonKey,
                           const QString& settingsKey)
{
    QString value = qEnvironmentVariable(envName).trimmed();

    if (value.isEmpty()) {
        value = loadLocalConfig().value(jsonKey).toString().trimmed();
    }

    if (value.isEmpty()) {
        QSettings settings(QStringLiteral("Trackly"), QStringLiteral("Trackly"));
        value = settings.value(settingsKey).toString().trimmed();
    }

    return value;
}

QString successMessageFor(const QString& key)
{
    if (key == "seizure") return QObject::tr("Приступ добавлен.");
    if (key == "medication") return QObject::tr("Лекарство добавлено.");
    if (key == "medication_log") return QObject::tr("Прием лекарства сохранен.");
    if (key == "examination") return QObject::tr("Обследование сохранено.");
    if (key == "attachment") return QObject::tr("Файл загружен.");
    if (key == "profile") return QObject::tr("Профиль обновлен.");
    if (key == "emergency_email") return QObject::tr("Экстренное письмо отправлено.");
    if (key == "trigger") return QObject::tr("Запись самочувствия сохранена.");
    return QObject::tr("Данные сохранены.");
}
