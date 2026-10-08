# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD

# Dictionaries: <lang>:<AOSP wordlist name>
set(PARROTFISH_LANGUAGES en:en_US it:it de:de)

set(dicts)
foreach(entry ${PARROTFISH_LANGUAGES})
    string(REPLACE ":" ";" parts ${entry})
    list(GET parts 0 lang)
    list(GET parts 1 src)
    set(out ${CMAKE_CURRENT_BINARY_DIR}/${lang}.pfd)
    add_custom_command(
        OUTPUT ${out}
        COMMAND ${Python3_EXECUTABLE} -I ${PROJECT_SOURCE_DIR}/tools/build-dict.py
                ${lang} ${CMAKE_CURRENT_SOURCE_DIR}/aosp/${src}_wordlist.combined.gz
                ${CMAKE_CURRENT_SOURCE_DIR}/rules ${out}
        DEPENDS ${PROJECT_SOURCE_DIR}/tools/build-dict.py
                ${CMAKE_CURRENT_SOURCE_DIR}/aosp/${src}_wordlist.combined.gz
                ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.rules
                ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.misspellings
                ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.supplement
        COMMENT "Building the ${lang} dictionary"
        VERBATIM)
    list(APPEND dicts ${out})
endforeach()
add_custom_target(dictionaries ALL DEPENDS ${dicts})
