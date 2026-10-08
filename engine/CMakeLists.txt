# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD

# The engine has no Qt dependency, so it builds and tests on any host.
add_library(parrotfish-engine STATIC
    src/corrector.cpp
    src/dictionary.cpp
    src/engine.cpp
    src/languagetracker.cpp
    src/keymodel.cpp
    src/utf8.cpp
)
target_include_directories(parrotfish-engine PUBLIC include)
target_compile_options(parrotfish-engine PRIVATE -Wall -Wextra)
set_target_properties(parrotfish-engine PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(pf-query tools/pf-query.cpp)
target_link_libraries(pf-query PRIVATE parrotfish-engine)
