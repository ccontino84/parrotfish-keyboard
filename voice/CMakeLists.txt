# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later

# The voice package: the helper (parrotfish-voice), the model downloader
# (parrotfish-voice-download) and Silero VAD; the speech model itself is
# downloaded from the settings. deps/ is not in the repository:
# fetch-deps.sh puts sherpa-onnx's prebuilt aarch64 C API (lib/, include/)
# and models/silero_vad.onnx there.
set(VOICE_DEPS ${CMAKE_CURRENT_SOURCE_DIR}/deps)
set(PARROTFISH_VOICE_LIB_DIR ${CMAKE_INSTALL_FULL_LIBDIR}/parrotfish-keyboard-voice)
set(PARROTFISH_VOICE_DATA_DIR ${CMAKE_INSTALL_FULL_DATADIR}/parrotfish-keyboard-voice)

find_package(Qt5 REQUIRED COMPONENTS Core Network)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(PULSE REQUIRED IMPORTED_TARGET libpulse-simple)

add_library(sherpa-onnx-c-api SHARED IMPORTED)
set_target_properties(sherpa-onnx-c-api PROPERTIES
    IMPORTED_LOCATION ${VOICE_DEPS}/sherpa-onnx/lib/libsherpa-onnx-c-api.so
    IMPORTED_SONAME libsherpa-onnx-c-api.so
    INTERFACE_INCLUDE_DIRECTORIES ${VOICE_DEPS}/sherpa-onnx/include)

add_executable(parrotfish-voice
    helper/audio.cpp
    helper/livetext.cpp
    helper/main.cpp
    helper/phrases.cpp
    helper/recognizer.cpp
    helper/sandbox.cpp
    helper/vad.cpp
)
target_link_libraries(parrotfish-voice PRIVATE sherpa-onnx-c-api PkgConfig::PULSE Threads::Threads)
target_compile_definitions(parrotfish-voice PRIVATE PARROTFISH_VOICE_DATA="${PARROTFISH_VOICE_DATA_DIR}")
target_compile_options(parrotfish-voice PRIVATE -Wall -Wextra)
# libsherpa-onnx-c-api.so needs libonnxruntime.so, found beside it ($ORIGIN).
target_link_options(parrotfish-voice PRIVATE -Wl,-rpath-link,${VOICE_DEPS}/sherpa-onnx/lib)
set_target_properties(parrotfish-voice PROPERTIES INSTALL_RPATH ${PARROTFISH_VOICE_LIB_DIR})

# The network part of voice input, kept out of the keyboard itself.
add_executable(parrotfish-voice-download downloader/main.cpp)
target_link_libraries(parrotfish-voice-download PRIVATE Qt5::Core Qt5::Network)
target_compile_definitions(parrotfish-voice-download PRIVATE QT_NO_CAST_FROM_ASCII)
target_compile_options(parrotfish-voice-download PRIVATE -Wall -Wextra)

install(TARGETS parrotfish-voice parrotfish-voice-download DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/parrotfish-keyboard-voice)
install(FILES ${VOICE_DEPS}/sherpa-onnx/lib/libsherpa-onnx-c-api.so ${VOICE_DEPS}/sherpa-onnx/lib/libonnxruntime.so
        DESTINATION ${PARROTFISH_VOICE_LIB_DIR})
install(FILES ${VOICE_DEPS}/models/silero_vad.onnx DESTINATION ${PARROTFISH_VOICE_DATA_DIR})
