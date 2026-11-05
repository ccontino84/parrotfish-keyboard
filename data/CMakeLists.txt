# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later

# Dictionaries: <lang>:<AOSP wordlist name>
set(PARROTFISH_LANGUAGES en:en_US it:it de:de es:es fr:fr pt_BR:pt_BR pt_PT:pt_PT nl:nl sv:sv da:da nb:nb ro:ro fi:fi)

# The inputs of each dictionary, and a fingerprint of all of them
# (inputs.sha256): `tools/pf rpm` puts dictionaries built on the PC in
# prebuilt/ with the fingerprint they were built from, and a build whose
# inputs have the same fingerprint copies them instead of building them
# again (in the SDK's build engine that takes most of the RPM's time).
# Without prebuilt/, or with another fingerprint, they are built here.
set(inputs_all ${PROJECT_SOURCE_DIR}/tools/data/build-dict.py)
foreach(entry ${PARROTFISH_LANGUAGES})
    string(REPLACE ":" ";" parts ${entry})
    list(GET parts 0 lang)
    list(GET parts 1 src)
    # Optional, hand-written (not every language has one).
    set(manual)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.manual-misspellings)
        set(manual ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.manual-misspellings)
    endif()
    set(inputs_${lang}
        ${CMAKE_CURRENT_SOURCE_DIR}/aosp/${src}_wordlist.combined.gz
        ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.rules
        ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.respellings
        ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.misspellings
        ${manual}
        ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.supplement
        ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.boost
        ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.forms
        ${CMAKE_CURRENT_SOURCE_DIR}/pairs/${lang}.tsv.gz)
    list(APPEND inputs_all ${inputs_${lang}})
endforeach()
set(fingerprint "${PARROTFISH_LANGUAGES}")
foreach(f ${inputs_all})
    file(SHA256 ${f} h)
    file(RELATIVE_PATH rel ${PROJECT_SOURCE_DIR} ${f})
    string(APPEND fingerprint "\n${rel} ${h}")
endforeach()
string(SHA256 fingerprint "${fingerprint}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/inputs.sha256 "${fingerprint}\n")
# The inputs are watched: an edit reruns this check at the next build.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${inputs_all})

set(prebuilt ${PROJECT_SOURCE_DIR}/prebuilt)
set(use_prebuilt OFF)
if(EXISTS ${prebuilt}/inputs.sha256)
    file(READ ${prebuilt}/inputs.sha256 stamp)
    string(STRIP "${stamp}" stamp)
    if(stamp STREQUAL fingerprint)
        set(use_prebuilt ON)
        message(STATUS "Dictionaries: prebuilt/ (inputs ${fingerprint})")
    else()
        message(STATUS "Dictionaries: prebuilt/ is for other inputs, building them")
    endif()
endif()

set(dicts)
foreach(entry ${PARROTFISH_LANGUAGES})
    string(REPLACE ":" ";" parts ${entry})
    list(GET parts 0 lang)
    list(GET parts 1 src)
    set(out ${CMAKE_CURRENT_BINARY_DIR}/${lang}.pfd)
    if(use_prebuilt AND EXISTS ${prebuilt}/${lang}.pfd)
        add_custom_command(
            OUTPUT ${out}
            COMMAND ${CMAKE_COMMAND} -E copy ${prebuilt}/${lang}.pfd ${out}
            DEPENDS ${prebuilt}/${lang}.pfd
            COMMENT "Copying the prebuilt ${lang} dictionary"
            VERBATIM)
        list(APPEND dicts ${out})
        continue()
    endif()
    add_custom_command(
        OUTPUT ${out}
        COMMAND ${Python3_EXECUTABLE} -I ${PROJECT_SOURCE_DIR}/tools/data/build-dict.py
                ${lang} ${CMAKE_CURRENT_SOURCE_DIR}/aosp/${src}_wordlist.combined.gz
                ${CMAKE_CURRENT_SOURCE_DIR}/rules ${out}
                --pairs ${CMAKE_CURRENT_SOURCE_DIR}/pairs/${lang}.tsv.gz
        DEPENDS ${PROJECT_SOURCE_DIR}/tools/data/build-dict.py ${inputs_${lang}}
        COMMENT "Building the ${lang} dictionary"
        VERBATIM)
    list(APPEND dicts ${out})
endforeach()
# The language list (tools/data/language-list.py, from rules/LANG.language).
set(langs)
set(language_files)
foreach(entry ${PARROTFISH_LANGUAGES})
    string(REPLACE ":" ";" parts ${entry})
    list(GET parts 0 lang)
    list(APPEND langs ${lang})
    list(APPEND language_files ${CMAKE_CURRENT_SOURCE_DIR}/rules/${lang}.language)
endforeach()
set(language_list ${CMAKE_CURRENT_BINARY_DIR}/languages.json)
add_custom_command(
    OUTPUT ${language_list}
    COMMAND ${Python3_EXECUTABLE} -I ${PROJECT_SOURCE_DIR}/tools/data/language-list.py
            ${CMAKE_CURRENT_SOURCE_DIR}/rules ${language_list} ${langs}
    DEPENDS ${PROJECT_SOURCE_DIR}/tools/data/language-list.py ${language_files}
    COMMENT "Writing the language list"
    VERBATIM)
add_custom_target(dictionaries ALL DEPENDS ${dicts} ${language_list})

install(FILES ${dicts} ${language_list} DESTINATION ${PARROTFISH_DATA_DIR})
install(DIRECTORY layouts/ DESTINATION ${PARROTFISH_DATA_DIR}/layouts FILES_MATCHING PATTERN "*.layout")
