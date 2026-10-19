#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QSettings>

#include "appcore.h"
#include "mainwindow.h"
#include "utils.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Секреты (адрес и anon key Supabase, ключ AI-прокси) НЕ хранятся в коде.
//  Значения читаются через tracklyConfigValue() — см. .env.example и
//  trackly.local.example.json.
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qDebug() << "MAIN STARTED FROM" << QString::fromLocal8Bit(argv[0]);
    QApplication::setOrganizationName("Trackly");
    QApplication::setApplicationName("Trackly");

    const int fontId = QFontDatabase::addApplicationFont(":/resources/fonts/JetBrainsMono.ttf");
    if (fontId >= 0) {
        const QString family = QFontDatabase::applicationFontFamilies(fontId).value(0);
        if (!family.isEmpty()) {
            QFont appFont(family);
            appFont.setPointSize(11);
            app.setFont(appFont);
        }
    }

    QSettings settings("Trackly", "Trackly");

    const QString supabaseUrl = tracklyConfigValue(
        "TRACKLY_SUPABASE_URL", QStringLiteral("supabase_url"), QStringLiteral("supabase/url"));
    const QString supabaseAnonKey = tracklyConfigValue(
        "TRACKLY_SUPABASE_ANON_KEY", QStringLiteral("supabase_anon_key"), QStringLiteral("supabase/anon_key"));

    AppCore core;
    core.setSupabaseConfig(supabaseUrl, supabaseAnonKey);

    core.applyTheme(static_cast<AppCore::ThemeMode>(settings.value("ui/theme", 0).toInt()));
    core.applyLanguage(settings.value("ui/language", "ru").toString());

    MainWindow window(&core);
    window.show();

    return app.exec();
}
