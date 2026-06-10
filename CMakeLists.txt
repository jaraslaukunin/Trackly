cmake_minimum_required(VERSION 3.21)

project(TracklyDesktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)


find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    Network
    Svg
    LinguistTools
    Charts
)

qt_standard_project_setup(
    I18N_TRANSLATED_LANGUAGES
        ru
        be
        be_Latn
        en
)

set(APP_SOURCES
    src/main.cpp

    src/appcore.cpp
    src/appcore.h

    src/mainwindow.cpp
    src/mainwindow.h

    src/authdialog.cpp
    src/authdialog.h

    src/styles.h

    src/utils.cpp
    src/utils.h

    src/widgets/toastwidget.cpp
    src/widgets/toastwidget.h

    src/tracklycalendar.cpp
    src/tracklycalendar.h

    src/examattachmentdroparea.cpp
    src/examattachmentdroparea.h

    src/pages/analyticspage.cpp
    src/pages/triggerspage.cpp
    src/pages/profilepage.cpp
    src/pages/dashboardpage.cpp
    src/pages/seizurespage.cpp
    src/pages/medicationspage.cpp
    src/pages/examinationspage.cpp

    resources/icons/Trackly.icns
    resources/resources.qrc
)

set(APP_TRANSLATIONS
    translations/trackly_ru.ts
    translations/trackly_be.ts
    translations/trackly_be_Latn.ts
    translations/trackly_en.ts
)

qt_add_executable(TracklyDesktop
    ${APP_SOURCES}
)

qt_add_translations(TracklyDesktop
    TS_FILES ${APP_TRANSLATIONS}
)

target_link_libraries(TracklyDesktop PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Svg
    Qt6::Charts
)

target_include_directories(TracklyDesktop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(TracklyDesktop PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Trackly Desktop"
    OUTPUT_NAME "Trackly Desktop"
)

set(MACOSX_BUNDLE_ICON_FILE Trackly.icns)
set_source_files_properties(resources/icons/Trackly.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
