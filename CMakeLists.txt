cmake_minimum_required(VERSION 3.21)

project(TracklyDesktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

# ─── Опция: деплой (включите при сборке для распространения) ───
option(TRACKLY_DEPLOY "Bundle Qt libraries for distribution" OFF)

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

# ─── Исходники ───

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
    src/exportreport.cpp
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
    resources/resources.qrc
)

# macOS-специфичный код
if(APPLE)
    list(APPEND APP_SOURCES
        src/appcore_mac.mm
    )
    # Иконка должна быть в resources.qrc или отдельно
    set(MACOSX_BUNDLE_ICON_FILE Trackly.icns)
endif()

# Windows-иконка
if(WIN32)
    list(APPEND APP_SOURCES resources/icons/Trackly.ico)
endif()

# ─── Переводы ───

set(APP_TRANSLATIONS
    translations/trackly_ru.ts
    translations/trackly_be.ts
    translations/trackly_be_Latn.ts
    translations/trackly_en.ts
)

# ─── Исполняемая цель ───

qt_add_executable(TracklyDesktop ${APP_SOURCES})

qt_add_translations(TracklyDesktop
    TS_FILES ${APP_TRANSLATIONS}
)

target_link_libraries(TracklyDesktop PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Svg
    Qt6::Charts
)

if(APPLE)
    target_link_libraries(TracklyDesktop PRIVATE "-framework AppKit")
endif()

target_include_directories(TracklyDesktop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(TracklyDesktop PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "Trackly Desktop"
)

if(APPLE)
    set_target_properties(TracklyDesktop PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Trackly Desktop"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2025 Trackly"
        MACOSX_BUNDLE_ICON_FILE "Trackly.icns"
    )
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# ДЕПЛОЙ ТОЛЬКО ПРИ ВКЛЮЧЕННОЙ ОПЦИИ
# ═══════════════════════════════════════════════════════════════════════════════

if(TRACKLY_DEPLOY AND APPLE)
    # Находим macdeployqt
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

    # Создаем кастомную цель для деплоя
    add_custom_target(deploy_qt ALL
        COMMENT "Deploying Qt libraries into bundle..."
        DEPENDS TracklyDesktop
    )

    # Добавляем команду после сборки
    add_custom_command(TARGET deploy_qt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Running macdeployqt..."
        COMMAND ${_qt_bin_dir}/macdeployqt "$<TARGET_FILE_DIR:TracklyDesktop>/../.."
                -verbose=2
                -always-overwrite
        COMMAND ${CMAKE_COMMAND} -E echo "✅ Deployment complete"
        COMMENT "Deploying Qt frameworks and plugins"
    )

    # Если используете QML (раскомментируйте если нужно)
    # COMMAND ${_qt_bin_dir}/macdeployqt "$<TARGET_FILE_DIR:TracklyDesktop>/../.."
    #         -qmldir=${CMAKE_SOURCE_DIR}/qml
    #         -verbose=2
    #         -always-overwrite

elseif(TRACKLY_DEPLOY AND WIN32)
    # Аналогично для Windows
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

    add_custom_target(deploy_qt ALL
        COMMENT "Deploying Qt libraries..."
        DEPENDS TracklyDesktop
    )

    add_custom_command(TARGET deploy_qt POST_BUILD
        COMMAND ${_qt_bin_dir}/windeployqt "$<TARGET_FILE:TracklyDesktop>"
                --verbose 2
                --no-compiler-runtime
        COMMENT "Running windeployqt..."
    )
endif()
