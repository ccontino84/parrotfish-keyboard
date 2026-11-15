# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later

add_executable(typo-bench bench/typo-bench.cpp)
target_link_libraries(typo-bench PRIVATE parrotfish-engine)

add_executable(lang-bench bench/lang-bench.cpp)
target_link_libraries(lang-bench PRIVATE parrotfish-engine)

add_executable(sentence-bench bench/sentence-bench.cpp)
target_link_libraries(sentence-bench PRIVATE parrotfish-engine)

add_executable(nextword-bench bench/nextword-bench.cpp)
target_link_libraries(nextword-bench PRIVATE parrotfish-engine)

add_executable(engine-test unit/engine-test.cpp)
target_link_libraries(engine-test PRIVATE parrotfish-engine)
add_dependencies(engine-test dictionaries)
add_test(NAME engine COMMAND engine-test ${PROJECT_BINARY_DIR}/data ${PROJECT_SOURCE_DIR}/data/layouts)
# sentence-bench -W: real misspellings typed after a few words of context.
add_test(NAME sentence-bench-misspellings
         COMMAND sentence-bench -l ${PROJECT_SOURCE_DIR}/data/layouts/phone-qwerty.layout
                 -t ${CMAKE_CURRENT_SOURCE_DIR}/data/tatoeba-en.txt -d ${PROJECT_BINARY_DIR}/data/en.pfd
                 -n 20 -r 0 -W ${CMAKE_CURRENT_SOURCE_DIR}/data/misspellings-smoke.tsv -A)
set_tests_properties(sentence-bench-misspellings PROPERTIES
                     PASS_REGULAR_EXPRESSION "misspelling +4 typed, 0 words in some language, of the rest fixed")
# bench::typedWords(): a final apostrophe stays only on dictionary words (po').
add_test(NAME bench-final-apostrophes
         COMMAND sentence-bench -l ${PROJECT_SOURCE_DIR}/data/layouts/phone-qwerty.layout
                 -t ${CMAKE_CURRENT_SOURCE_DIR}/data/apostrophes.txt -d ${PROJECT_BINARY_DIR}/data/it.pfd
                 -d ${PROJECT_BINARY_DIR}/data/en.pfd -E)
set_tests_properties(bench-final-apostrophes PROPERTIES
                     PASS_REGULAR_EXPRESSION "un \\| po' \\| di.*girls' \\| room.*boys \\| toys.*said \\| hello \\| to.*casa \\| de \\| nonni"
                     FAIL_REGULAR_EXPRESSION "hello'|boys'|de'")

# The voice helper's network block (voice/helper/sandbox.cpp).
add_executable(voice-sandbox-test unit/voice-sandbox-test.cpp ${PROJECT_SOURCE_DIR}/voice/helper/sandbox.cpp)
target_include_directories(voice-sandbox-test PRIVATE ${PROJECT_SOURCE_DIR}/voice/helper)
add_test(NAME voice-sandbox COMMAND voice-sandbox-test)

# The voice panel's text and edits (plugin/src/voicetext.cpp).
if(PARROTFISH_PLUGIN)
    find_package(Qt5 REQUIRED COMPONENTS Core)
    add_executable(voicetext-test unit/voicetext-test.cpp ${PROJECT_SOURCE_DIR}/plugin/src/voicetext.cpp)
    target_include_directories(voicetext-test PRIVATE ${PROJECT_SOURCE_DIR}/plugin/src)
    target_compile_definitions(voicetext-test PRIVATE QT_NO_CAST_FROM_ASCII)
    target_link_libraries(voicetext-test PRIVATE Qt5::Core)
    add_test(NAME voicetext COMMAND voicetext-test)
endif()

# parrotfish candidates / learned on a small learning.db.
if(PARROTFISH_PLUGIN)
    add_test(NAME cli COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cli/test-cli.py
                              $<TARGET_FILE:parrotfish-cli>)
endif()

# The spec's dictformat is the engine's dictionary format (the language
# packages depend on it).
add_test(NAME spec COMMAND ${Python3_EXECUTABLE} -c
         "import re,sys; s=open(sys.argv[1]).read(); h=open(sys.argv[2]).read(); a=re.search(r'%global dictformat (\\d+)', s).group(1); b=re.search(r'FormatVersion = (\\d+)', h).group(1); sys.exit(0 if a==b else f'spec dictformat {a}, engine FormatVersion {b}')"
         ${PROJECT_SOURCE_DIR}/rpm/parrotfish-keyboard.spec
         ${PROJECT_SOURCE_DIR}/engine/include/parrotfish/dictformat.h)

# The QML tests (tests/qml): the handler with the plugin, on the PC.
find_program(QMLTESTRUNNER NAMES qmltestrunner-qt5 qmltestrunner)
if(PARROTFISH_PLUGIN AND QMLTESTRUNNER)
    add_test(NAME qml COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/qml/run.sh ${PROJECT_BINARY_DIR})
    set_tests_properties(qml PROPERTIES ENVIRONMENT "QMLTESTRUNNER=${QMLTESTRUNNER}")
endif()

if(PARROTFISH_DEVTOOLS)
    install(TARGETS typo-bench lang-bench sentence-bench nextword-bench DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/parrotfish-keyboard)
    install(FILES data/sentences-en.txt data/sentences-it.txt data/sentences-de.txt
                  data/tatoeba-en.txt data/tatoeba-it.txt data/tatoeba-de.txt
            DESTINATION ${PARROTFISH_DATA_DIR}/test-data)
endif()
