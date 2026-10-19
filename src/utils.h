#pragma once

#include <QString>
#include <QLayout>

QString formatIsoToLocal(const QString& iso);
void clearLayout(QLayout* layout);
QString prettifyError(const QString& raw);
QString successMessageFor(const QString& key);

// ─── Локальная конфигурация ─────────────────────────────────────────────────
// Секреты не хранятся в исходном коде. Значение ищется по порядку:
//   1) переменная окружения (envName)
//   2) поле jsonKey в trackly.local.json (рядом с проектом или с .app)
//   3) ключ settingsKey в QSettings приложения
// Шаблон — .env.example, пример файла — trackly.local.example.json
QString tracklyConfigValue(const char* envName,
                           const QString& jsonKey,
                           const QString& settingsKey);
