cmake_minimum_required(VERSION 4.0)

set(APP_NAME "EasyLangSwitcher")

project(${APP_NAME} VERSION 1.4.0)

set(APP_VERSION "${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ELS_ENABLE_STRICT_WARNINGS "Treat compiler warnings as errors" OFF)
option(ELS_ENABLE_CLANG_TIDY "Enable clang-tidy during build when available" OFF)
option(ELS_CLANG_TIDY_WARNINGS_AS_ERRORS "Treat clang-tidy diagnostics as errors" OFF)
option(ELS_ENABLE_FORMAT_TARGETS "Enable clang-format helper targets" ON)
option(ELS_RUN_FORMAT_BEFORE_BUILD "Run 'format' target before app build (rewrites source files)" OFF)
option(ELS_RUN_CHECK_FORMAT_BEFORE_BUILD "Run 'check-format' target before app build (fails on formatting issues)" OFF)
set(ELS_CLANG_TIDY_PATH "" CACHE FILEPATH "Optional absolute path to clang-tidy executable")
set(ELS_CLANG_FORMAT_PATH "" CACHE FILEPATH "Optional absolute path to clang-format executable")

set(ELS_TOOL_HINT_DIRS
        "$ENV{LOCALAPPDATA}/Programs/CLion/bin/clang/win/x64/bin"
        "$ENV{LOCALAPPDATA}/Programs/CLion/plugins/clion-radler/DotFiles/windows-x64"
        "$ENV{LOCALAPPDATA}/Programs/CLion/plugins/clion-radler/DotFiles/windows-arm64"
        "$ENV{LOCALAPPDATA}/Qt/Tools/QtCreator/bin/clang/bin")

if (MSVC)
    add_compile_options(/utf-8 /W4 /permissive-)
    if (ELS_ENABLE_STRICT_WARNINGS)
        add_compile_options(/WX)
    endif ()
else ()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if (ELS_ENABLE_STRICT_WARNINGS)
        add_compile_options(-Werror)
    endif ()
endif ()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Paths to .ui forms
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/forms")

set(AUTOGEN_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}_autogen/include")
set(DIST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dist/release/${APP_VERSION}")

# Qt detection
set(ELS_QT_ROOT "" CACHE PATH "Optional Qt kit root (example: C:/Qt/6.10.1/msvc2022_64)")
set(_els_qt_candidates "")

if (ELS_QT_ROOT)
    list(APPEND _els_qt_candidates "${ELS_QT_ROOT}")
endif ()

if (DEFINED ENV{QT_DIR} AND NOT "$ENV{QT_DIR}" STREQUAL "")
    list(APPEND _els_qt_candidates "$ENV{QT_DIR}")
endif ()

file(GLOB _els_qt_local_kits "$ENV{LOCALAPPDATA}/Qt/*/msvc2022_64")
file(GLOB _els_qt_global_kits "C:/Qt/*/msvc2022_64")
list(APPEND _els_qt_candidates ${_els_qt_local_kits} ${_els_qt_global_kits})

foreach (_els_qt_candidate IN LISTS _els_qt_candidates)
    if (EXISTS "${_els_qt_candidate}/bin/windeployqt6.exe" OR EXISTS "${_els_qt_candidate}/lib/cmake/Qt6/Qt6Config.cmake")
        list(PREPEND CMAKE_PREFIX_PATH "${_els_qt_candidate}")
    endif ()
endforeach ()

find_package(Qt6 COMPONENTS Core Gui Widgets Svg Multimedia Network REQUIRED)

set(QT_KIT_DIR "")
if (DEFINED Qt6_DIR AND EXISTS "${Qt6_DIR}")
    get_filename_component(_qt6_cmake_dir "${Qt6_DIR}" REALPATH)
    get_filename_component(_qt6_cmake_parent "${_qt6_cmake_dir}" DIRECTORY)
    get_filename_component(_qt6_lib_dir "${_qt6_cmake_parent}" DIRECTORY)
    get_filename_component(QT_KIT_DIR "${_qt6_lib_dir}" DIRECTORY)
endif ()

if (NOT QT_KIT_DIR)
    foreach (_els_qt_candidate IN LISTS CMAKE_PREFIX_PATH)
        if (EXISTS "${_els_qt_candidate}/bin/windeployqt6.exe")
            set(QT_KIT_DIR "${_els_qt_candidate}")
            break()
        endif ()
    endforeach ()
endif ()

if (NOT QT_KIT_DIR)
    message(FATAL_ERROR "Qt kit root was not resolved. Set ELS_QT_ROOT to your Qt kit path.")
endif ()

set(QT_BIN_DIR "${QT_KIT_DIR}/bin")
set(WINDEPLOYQT "${QT_BIN_DIR}/windeployqt6.exe")

if (NOT EXISTS "${WINDEPLOYQT}")
    message(FATAL_ERROR "windeployqt6.exe was not found in Qt kit: ${QT_KIT_DIR}")
endif ()

if (ELS_ENABLE_CLANG_TIDY)
    if (ELS_CLANG_TIDY_PATH)
        set(CLANG_TIDY_EXE "${ELS_CLANG_TIDY_PATH}")
    else ()
        find_program(CLANG_TIDY_EXE NAMES clang-tidy HINTS ${ELS_TOOL_HINT_DIRS})
    endif ()
    if (CLANG_TIDY_EXE)
        set(ELS_CLANG_TIDY_ARGS
                ${CLANG_TIDY_EXE}
                --header-filter=^${CMAKE_SOURCE_DIR}/src/)
        if (ELS_CLANG_TIDY_WARNINGS_AS_ERRORS)
            list(APPEND ELS_CLANG_TIDY_ARGS --warnings-as-errors=*)
        endif ()
        set(CMAKE_CXX_CLANG_TIDY ${ELS_CLANG_TIDY_ARGS})
    else ()
        message(STATUS "clang-tidy was not found. Build will continue without clang-tidy.")
    endif ()
endif ()

file(GLOB_RECURSE UI_FILES "src/ui/forms/*.ui")

if (ELS_ENABLE_FORMAT_TARGETS)
    if (ELS_CLANG_FORMAT_PATH)
        set(CLANG_FORMAT_EXE "${ELS_CLANG_FORMAT_PATH}")
    else ()
        find_program(CLANG_FORMAT_EXE NAMES clang-format HINTS ${ELS_TOOL_HINT_DIRS})
    endif ()
    if (CLANG_FORMAT_EXE)
        file(GLOB_RECURSE ELS_FORMAT_FILES CONFIGURE_DEPENDS
                "${CMAKE_SOURCE_DIR}/src/*.cpp"
                "${CMAKE_SOURCE_DIR}/src/*.h")

        add_custom_target(format
                COMMAND ${CLANG_FORMAT_EXE} -i ${ELS_FORMAT_FILES}
                COMMENT "Formatting project sources with clang-format"
                VERBATIM)

        add_custom_target(check-format
                COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${ELS_FORMAT_FILES}
                COMMENT "Checking formatting with clang-format"
                VERBATIM)
    else ()
        message(STATUS "clang-format was not found. 'format' and 'check-format' targets will be unavailable.")
    endif ()
endif ()

if (TARGET check-format AND TARGET format AND ELS_RUN_FORMAT_BEFORE_BUILD AND ELS_RUN_CHECK_FORMAT_BEFORE_BUILD)
    add_dependencies(check-format format)
endif ()

add_executable(${APP_NAME}
        src/app/main.cpp
        src/app/mainBootstrapHelper.cpp
        src/app/mainBootstrapHelper.h

        # core
        src/core/config/appSettings.cpp
        src/core/config/appSettings.h
        src/core/config/logger.h
        src/core/config/loggerQtBridge.h

        src/core/handlers/kb.cpp
        src/core/handlers/kb.h

        src/core/i18n/lang.cpp
        src/core/i18n/lang.h

        # ui
        src/ui/helpers/acrylicHelper.h
        src/ui/helpers/closeButton.h
        src/ui/helpers/fontHelper.h
        src/ui/helpers/iconHelper.h
        src/ui/helpers/keySequenceHelper.h
        src/ui/helpers/screenResolver.h
        src/ui/helpers/syncIconHelper.h
        src/ui/helpers/trayHoverHelper.h
        src/ui/helpers/vkMapper.h
        src/ui/helpers/warningHelper.h
        src/ui/helpers/windowsNotificationState.cpp
        src/ui/helpers/windowsNotificationState.h
        src/ui/helpers/windowsToastIdentity.h
        src/ui/helpers/windowsToastNotification.cpp
        src/ui/helpers/windowsToastNotification.h

        src/ui/tray/tray.cpp
        src/ui/tray/tray.h

        src/ui/widgets/animatedSelector.cpp
        src/ui/widgets/animatedSelector.h
        src/ui/widgets/autoStartup.cpp
        src/ui/widgets/autoStartup.h
        src/ui/widgets/customToolTip.cpp
        src/ui/widgets/customToolTip.h
        src/ui/widgets/hoverWarning.cpp
        src/ui/widgets/hoverWarning.h

        src/ui/widgets/notifications/globalNotification.cpp
        src/ui/widgets/notifications/globalNotification.h
        src/ui/widgets/notifications/inAppNotification.cpp
        src/ui/widgets/notifications/inAppNotification.h

        src/ui/widgets/settingsWindow/settingsWindow.cpp
        src/ui/widgets/settingsWindow/settingsWindow.h
        src/ui/widgets/settingsWindow/settingsWindow_hotkey.cpp
        src/ui/widgets/settingsWindow/settingsWindow_state.cpp
        src/ui/widgets/settingsWindow/settingsWindow_update.cpp
        src/ui/widgets/settingsWindow/settingsWindow_visuals.cpp

        src/ui/widgets/soundManager.cpp
        src/ui/widgets/soundManager.h
        src/ui/widgets/updateManager.cpp
        src/ui/widgets/updateManager.h
        src/ui/widgets/updFrequencyPopup.cpp
        src/ui/widgets/updFrequencyPopup.h
        src/ui/widgets/windowDragger.cpp
        src/ui/widgets/windowDragger.h

        # resources
        resources/resources.qrc

        # app icon
        resources/icon.rc

        ${UI_FILES}
)

if (ELS_RUN_FORMAT_BEFORE_BUILD)
    if (TARGET format)
        add_dependencies(${APP_NAME} format)
    else ()
        message(STATUS "ELS_RUN_FORMAT_BEFORE_BUILD is ON but target 'format' is unavailable.")
    endif ()
endif ()

if (ELS_RUN_CHECK_FORMAT_BEFORE_BUILD)
    if (TARGET check-format)
        add_dependencies(${APP_NAME} check-format)
    else ()
        message(STATUS "ELS_RUN_CHECK_FORMAT_BEFORE_BUILD is ON but target 'check-format' is unavailable.")
    endif ()
endif ()

target_compile_definitions(${APP_NAME} PRIVATE APP_VERSION="${PROJECT_VERSION}")

configure_file(
        resources/version.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        @ONLY
)

target_sources(${APP_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/version.rc)

target_include_directories(${APP_NAME} PRIVATE "${AUTOGEN_INCLUDE}")

target_link_libraries(${APP_NAME} Qt::Core Qt::Gui Qt::Widgets Qt::Svg Qt::Multimedia Qt::Network)

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT}"
            --no-translations
            --no-compiler-runtime
            "$<TARGET_FILE:${APP_NAME}>"
            COMMENT "Running windeployqt to copy Qt dependencies"
    )
endif ()

if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    file(REMOVE_RECURSE "${DIST_DIR}")

    # Скрываем консоль
    set_target_properties(${APP_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

    add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}/platforms"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}/styles"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}/imageformats"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}/tls"

            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${APP_NAME}>" "${DIST_DIR}/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_BIN_DIR}/Qt6Core.dll" "${DIST_DIR}/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_BIN_DIR}/Qt6Gui.dll" "${DIST_DIR}/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_BIN_DIR}/Qt6Multimedia.dll" "${DIST_DIR}/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_BIN_DIR}/Qt6Network.dll" "${DIST_DIR}/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_BIN_DIR}/Qt6Svg.dll" "${DIST_DIR}/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_BIN_DIR}/Qt6Widgets.dll" "${DIST_DIR}/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_KIT_DIR}/plugins/platforms/qwindows.dll" "${DIST_DIR}/platforms/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_KIT_DIR}/plugins/styles/qmodernwindowsstyle.dll" "${DIST_DIR}/styles/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_KIT_DIR}/plugins/imageformats/qsvg.dll" "${DIST_DIR}/imageformats/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_KIT_DIR}/plugins/imageformats/qjpeg.dll" "${DIST_DIR}/imageformats/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_KIT_DIR}/plugins/tls/qcertonlybackend.dll" "${DIST_DIR}/tls/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_KIT_DIR}/plugins/tls/qschannelbackend.dll" "${DIST_DIR}/tls/"

            COMMENT "Deploying minimal set to ${DIST_DIR}"
    )

    #    # тянет слишком много мусора
    #    add_custom_command(TARGET ${APP_NAME} POST_BUILD
    #            # Создаем папку
    #            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}"
    #
    #            # Копируем сам EXE
    #            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE :${APP_NAME}>" "${DIST_DIR}/"
    #
    #            COMMAND "${WINDEPLOYQT}"
    #            --no-translations
    #            --no-opengl-sw
    #            --no-compiler-runtime
    #            --dir "${DIST_DIR}"
    #            "${DIST_DIR}/${APP_NAME}.exe"
    #
    #            # (Опционально) Если windeployqt всё же тянет что-то лишнее,
    #            # можно удалить их принудительно после работы утилиты:
    #            COMMAND ${CMAKE_COMMAND} -E remove "${DIST_DIR}/vc_redist.x64.exe"
    #
    #            COMMENT "Deploying minimal set to ${DIST_DIR}"
    #    )
endif ()
