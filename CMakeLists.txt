cmake_minimum_required(VERSION 4.0)
add_compile_options(/utf-8)

set(APP_NAME "EasyLangSwitcher")

project(${APP_NAME} VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 26)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Paths to .ui forms
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/forms")

# Qt detection
set(QT_DIR "$ENV{QT_DIR}")
if (NOT QT_DIR)
    message(FATAL_ERROR "Please set QT_DIR environment variable to Qt installation path")
endif ()

set(CMAKE_PREFIX "${QT_DIR}/6.10.0/msvc2022_64")
set(QT_BIN_DIR "${CMAKE_PREFIX}/bin")
set(WINDEPLOYQT "${QT_BIN_DIR}/windeployqt6.exe")
set(AUTOGEN_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}_autogen/include")
set(DIST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dist/release")

find_package(Qt6 COMPONENTS Core Gui Widgets Svg REQUIRED)

file(GLOB_RECURSE UI_FILES "src/ui/forms/*.ui")

add_executable(${APP_NAME}
        src/app/main.cpp

        # core
        src/core/config/app_config.h
        src/core/handlers/kb.cpp
        src/core/handlers/kb.h
        src/core/i18n/lang.cpp
        src/core/i18n/lang.h

        # ui
        src/ui/helpers/acrylicHelper.h
        src/ui/helpers/hoverHelper.h
        src/ui/helpers/iconHelper.h
        src/ui/tray/tray.cpp
        src/ui/tray/tray.h
        src/ui/widgets/settings.cpp
        src/ui/widgets/settings.h

        # resources
        resources/resources.qrc

        ${UI_FILES}

        # app icon
        resources/icon.rc
)

target_compile_definitions(${APP_NAME} PRIVATE APP_VERSION="${PROJECT_VERSION}")

configure_file(
        resources/version.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        @ONLY
)

target_sources(${APP_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/version.rc)

target_include_directories(${APP_NAME} PRIVATE "${AUTOGEN_INCLUDE}")

target_link_libraries(${APP_NAME} Qt::Core Qt::Gui Qt::Widgets Qt::Svg)

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

    set_target_properties(${APP_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

    add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}/platforms"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${APP_NAME}>" "${DIST_DIR}/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_BIN_DIR}/Qt6Core.dll" "${DIST_DIR}/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_BIN_DIR}/Qt6Gui.dll" "${DIST_DIR}/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_BIN_DIR}/Qt6Widgets.dll" "${DIST_DIR}/"
            COMMAND ${CMAKE_COMMAND} -E copy "${QT_BIN_DIR}/Qt6Svg.dll" "${DIST_DIR}/"
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_PREFIX}/plugins/platforms/qwindows.dll" "${DIST_DIR}/platforms/"
            COMMENT "Deploying minimal set to ${DIST_DIR}"
    )
endif ()
