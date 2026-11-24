# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later

# parrotfish-download: the settings' downloads (the speech model, the
# dictionaries), the network part kept out of the keyboard itself.
find_package(Qt5 REQUIRED COMPONENTS Core Network)
find_package(ZLIB REQUIRED)

add_executable(parrotfish-download main.cpp)
target_link_libraries(parrotfish-download PRIVATE Qt5::Core Qt5::Network ZLIB::ZLIB)
target_compile_definitions(parrotfish-download PRIVATE QT_NO_CAST_FROM_ASCII PARROTFISH_DATA_DIR="${PARROTFISH_DATA_DIR}")
target_compile_options(parrotfish-download PRIVATE -Wall -Wextra)
install(TARGETS parrotfish-download DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/parrotfish-keyboard)
