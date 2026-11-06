# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later

# The settings' translations (qsTr: the context is the QML file's name):
# each .ts here becomes the .qm of the same name in
# /usr/share/translations, loaded by the plugin (plugin/src/plugin.cpp).
# Only the lrelease of the Qt 5 we build with (the SDK's
# qt5-qttools-linguist); without it, no translations are built.
find_package(Qt5 REQUIRED COMPONENTS Core)
get_target_property(qmake Qt5::qmake IMPORTED_LOCATION)
get_filename_component(qt_bin ${qmake} DIRECTORY)
find_program(LRELEASE NAMES lrelease lrelease-qt5 HINTS ${qt_bin} NO_DEFAULT_PATH)
if(NOT LRELEASE)
    message(STATUS "No Qt 5 lrelease: translations not built")
    return()
endif()

file(GLOB ts_files ${CMAKE_CURRENT_SOURCE_DIR}/*.ts)
set(qm_files)
foreach(ts ${ts_files})
    get_filename_component(name ${ts} NAME_WE)
    set(qm ${CMAKE_CURRENT_BINARY_DIR}/${name}.qm)
    add_custom_command(OUTPUT ${qm}
                       COMMAND ${LRELEASE} -silent ${ts} -qm ${qm}
                       DEPENDS ${ts} VERBATIM)
    list(APPEND qm_files ${qm})
endforeach()
add_custom_target(translations ALL DEPENDS ${qm_files})
install(FILES ${qm_files} DESTINATION ${CMAKE_INSTALL_DATADIR}/translations)
