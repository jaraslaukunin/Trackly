#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QSettings>

#include "appcore.h"
#include "mainwindow.h"

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

    AppCore core;
    core.setSupabaseConfig(
        "https://your-project-ref.supabase.co",
        "REMOVED_ANON_KEY"
        );

    QSettings settings("Trackly", "Trackly");
    core.applyTheme(static_cast<AppCore::ThemeMode>(settings.value("ui/theme", 0).toInt()));
    core.applyLanguage(settings.value("ui/language", "ru").toString());

    MainWindow window(&core);
    window.show();

    return app.exec();
}
