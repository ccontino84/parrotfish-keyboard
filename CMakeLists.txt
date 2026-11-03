# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD

cmake_minimum_required(VERSION 3.16)
project(parrotfish-keyboard VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

include(GNUInstallDirs)

option(PARROTFISH_TESTS "Build the host tests and benchmarks" ON)
option(PARROTFISH_PLUGIN "Build the QML plugin and install the handler (needs Qt 5)" ON)
option(PARROTFISH_DEVTOOLS "Install pf-query and the benchmarks (test builds)" OFF)
set(PARROTFISH_DATA_DIR "${CMAKE_INSTALL_FULL_DATADIR}/parrotfish-keyboard"
    CACHE PATH "Dictionaries and layouts")
# The package version, for the debug logs (the spec passes %{version}).
set(PARROTFISH_VERSION "dev" CACHE STRING "Package version")
set(PARROTFISH_QML_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/qt5/qml" CACHE PATH "Qt 5 QML import path")
set(PARROTFISH_HANDLER_DIR "${CMAKE_INSTALL_FULL_DATADIR}/maliit/plugins/com/jolla/handlers"
    CACHE PATH "jolla-keyboard handlers")
# Sailfish OS before 5.2 loads handlers from the plugin directory itself.
set(PARROTFISH_OLD_HANDLER_DIR "${CMAKE_INSTALL_FULL_DATADIR}/maliit/plugins/com/jolla"
    CACHE PATH "jolla-keyboard handlers before Sailfish OS 5.2")
set(PARROTFISH_LAYOUT_DIR "${CMAKE_INSTALL_FULL_DATADIR}/maliit/plugins/com/jolla/layouts"
    CACHE PATH "jolla-keyboard layouts")

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_subdirectory(engine)
add_subdirectory(data)
if(PARROTFISH_PLUGIN)
    add_subdirectory(plugin)
    install(FILES handler/ParrotfishHandler.qml handler/parrotfish-keyboard.conf
            DESTINATION ${PARROTFISH_HANDLER_DIR})
    # One package for all versions: each loads the copy in its own directory.
    install(FILES handler/ParrotfishHandler.qml DESTINATION ${PARROTFISH_OLD_HANDLER_DIR})
    install(FILES layouts/parrotfish.qml layouts/layouts_parrotfish.conf
            DESTINATION ${PARROTFISH_LAYOUT_DIR})
    install(DIRECTORY layouts/parrotfish DESTINATION ${PARROTFISH_LAYOUT_DIR})
    # the .conf files name this path for Settings > Text input
    install(FILES settings/ParrotfishSettings.qml settings/ParrotfishLanguages.qml settings/ParrotfishLearned.qml
                  settings/languages.js
            DESTINATION ${PARROTFISH_DATA_DIR}/settings)
endif()

if(PARROTFISH_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
