# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later

# parrotfish: what the keyboard is learning (reads learning.db).
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)

add_executable(parrotfish-cli parrotfish.cpp)
set_target_properties(parrotfish-cli PROPERTIES OUTPUT_NAME parrotfish)
target_link_libraries(parrotfish-cli PRIVATE parrotfish-engine PkgConfig::SQLITE3)
target_compile_options(parrotfish-cli PRIVATE -Wall -Wextra)
install(TARGETS parrotfish-cli DESTINATION ${CMAKE_INSTALL_BINDIR})
