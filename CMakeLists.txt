cmake_minimum_required(VERSION 4.0)

set(APP_NAME "EasyLangSwitcher")

project(${APP_NAME})
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

set(CMAKE_PREFIX_PATH "${QT_DIR}/6.10.0/msvc2022_64")

find_package(Qt6 COMPONENTS Core Gui Widgets Svg REQUIRED)

# Sources
file(GLOB_RECURSE UI_FILES "src/ui/forms/*.ui")

add_executable(${APP_NAME}
        src/app/main.cpp

        # core
        src/core/config/app_config.h
        src/core/handlers/kb.cpp
        src/core/handlers/kb.h
        src/core/helpers/acrylicHelper.h
        src/core/helpers/hoverHelper.h
        src/core/helpers/iconHelper.h
        src/core/i18n/lang.cpp
        src/core/i18n/lang.h

        # ui
        src/ui/tray/tray.cpp
        src/ui/tray/tray.h

        # resources
        resources/resources.qrc

        ${UI_FILES}
)

set(AUTOGEN_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}_autogen/include")
target_include_directories(${APP_NAME} PRIVATE "${AUTOGEN_INCLUDE}")

target_link_libraries(${APP_NAME} Qt::Core Qt::Gui Qt::Widgets Qt::Svg)

# Qt deployment on Windows using windeployqt
if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)

    # Qt tool based on build configuration
    set(WINDEPLOYQT "${QT_DIR}/6.10.0/msvc2022_64/bin/windeployqt6.exe")

    add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT}" --no-translations --no-compiler-runtime
            "$<TARGET_FILE:${APP_NAME}>"
            COMMENT "Running windeployqt to copy Qt dependencies"
    )
endif ()