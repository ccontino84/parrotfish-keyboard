# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD

add_executable(typo-bench bench/typo-bench.cpp)
target_link_libraries(typo-bench PRIVATE parrotfish-engine)

add_executable(lang-bench bench/lang-bench.cpp)
target_link_libraries(lang-bench PRIVATE parrotfish-engine)

add_executable(sentence-bench bench/sentence-bench.cpp)
target_link_libraries(sentence-bench PRIVATE parrotfish-engine)

add_executable(engine-test unit/engine-test.cpp)
target_link_libraries(engine-test PRIVATE parrotfish-engine)
add_dependencies(engine-test dictionaries)
add_test(NAME engine COMMAND engine-test ${PROJECT_BINARY_DIR}/data ${PROJECT_SOURCE_DIR}/data/layouts)

if(PARROTFISH_DEVTOOLS)
    install(TARGETS typo-bench lang-bench sentence-bench DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/parrotfish-keyboard)
    install(FILES data/sentences-en.txt data/sentences-it.txt data/sentences-de.txt
                  data/tatoeba-en.txt data/tatoeba-it.txt data/tatoeba-de.txt
            DESTINATION ${PARROTFISH_DATA_DIR}/test-data)
endif()
