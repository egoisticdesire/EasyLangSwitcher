cmake_minimum_required(VERSION 4.0)

project(EasyLangSwitcher)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(QT_DIR "$ENV{QT_DIR}")
if (NOT QT_DIR)
    message(FATAL_ERROR "Please set QT_DIR environment variable to Qt installation path")
endif ()
set(CMAKE_PREFIX_PATH "${QT_DIR}/6.10.0/msvc2022_64/lib/cmake/Qt6")

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Svg
        REQUIRED
)
add_executable(EasyLangSwitcher
        main.cpp
        handlers/keyboard.cpp
        handlers/keyboard.h
        widgets/settings.cpp
        widgets/settings.h
        widgets/tray.cpp
        widgets/tray.h
        helpers/acrylicEffect.h
        helpers/svg.h
        helpers/hoverEffect.h
)
target_link_libraries(EasyLangSwitcher
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Svg
)
qt_add_resources(RESOURCES resources/resources.qrc)
target_sources(EasyLangSwitcher PRIVATE ${RESOURCES})
if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
            if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
                set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
            endif ()
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND
                ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/"
        )
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND
                ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/"
        )
    endif ()
    foreach (QT_LIB
            Core
            Gui
            Widgets
            Svg
    )
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND
                ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )
    endforeach (QT_LIB)
endif ()