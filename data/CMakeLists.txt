# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD

# Dictionaries: <lang>:<AOSP wordlist name>
set(PARROTFISH_LANGUAGES en:en_US it:it de:de es:es fr:fr pt_BR:pt_BR pt_PT:pt_PT nl:nl sv:sv da:da nb:nb ro:ro fi:fi)

set(dicts)
foreach(entry ${PARROTFISH_LANGUAGES})
    string(REPLACE ":" ";" parts ${entry})
    list(GET parts 0 lang)
    list(GET parts 1 src)
    set(out ${CMAKE_CURRENT_BINARY_DIR}/${lang}.pfd)
    add_custom_command(
        OUTPUT ${out}
        COMMAND ${Python3_EXECUTABLE} -I ${PROJECT_SOURCE_DIR}/tools/data/build-dict.py
                ${lang} ${CMAKE_CURRENT_SOURCE_DIR}/aosp/${src}_wordlist.combined.gz
                ${CMAKE_CURRENT_SOURCE_DIR}/rules ${out}
                --pairs ${CMAKE_CURRENT_SOURCE_DIR}/pairs/${lang}.tsv.gz
        DEPENDS ${PROJECT_SOURCE_DIR}/tools/data/build-dict.py
                ${CMAKE_CURRENT_SOURCE_DIR}/aosp/${src}_wordlist.combined.gz
                ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.rules
                ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.misspellings
                ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.supplement
                ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.boost
                ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.forms
                ${CMAKE_CURRENT_SOURCE_DIR}/pairs/${lang}.tsv.gz
        COMMENT "Building the ${lang} dictionary"
        VERBATIM)
    list(APPEND dicts ${out})
endforeach()
add_custom_target(dictionaries ALL DEPENDS ${dicts})

install(FILES ${dicts} DESTINATION ${PARROTFISH_DATA_DIR})
install(DIRECTORY layouts/ DESTINATION ${PARROTFISH_DATA_DIR}/layouts FILES_MATCHING PATTERN "*.layout")
