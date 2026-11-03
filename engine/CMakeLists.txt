# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD

# The engine has no Qt dependency, so it builds and tests on any host.
add_library(parrotfish-engine STATIC
    src/corrector.cpp
    src/dictionary.cpp
    src/engine.cpp
    src/search.cpp
    src/decide.cpp
    src/params.cpp
    src/languagetracker.cpp
    src/keymodel.cpp
    src/learned.cpp
    src/utf8.cpp
)
target_include_directories(parrotfish-engine PUBLIC include)
target_compile_options(parrotfish-engine PRIVATE -Wall -Wextra)
set_target_properties(parrotfish-engine PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(pf-query tools/pf-query.cpp)
target_link_libraries(pf-query PRIVATE parrotfish-engine)
add_executable(pf-time tools/pf-time.cpp)
target_link_libraries(pf-time PRIVATE parrotfish-engine)

if(PARROTFISH_DEVTOOLS)
    install(TARGETS pf-query pf-time DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/parrotfish-keyboard)
endif()
