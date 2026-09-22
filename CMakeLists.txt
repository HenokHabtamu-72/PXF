cmake_minimum_required(VERSION 3.22)

project(PXF
    VERSION   1.0.0
    LANGUAGES C CXX
    DESCRIPTION "PXF — a late-night subtractive synth and granulizer in one VST3 instrument")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The sources are UTF-8 (em dashes and the like live in comments). Tell MSVC, which
# otherwise reads them in the system code page and emits C4819.
if(MSVC)
    add_compile_options(/utf-8)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Universal binaries on macOS so one build runs on Intel and Apple silicon.
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)
endif()

# ---------------------------------------------------------------------------
# JUCE
#
# By default this downloads JUCE for you on the first configure (~100 MB, once).
# Already have a checkout? Point at it and skip the download:
#     cmake -B build -DPXF_FETCH_JUCE=OFF -DPXF_JUCE_PATH=C:/SDKs/JUCE
# ---------------------------------------------------------------------------
option(PXF_FETCH_JUCE "Download JUCE automatically" ON)
set(PXF_JUCE_PATH "" CACHE PATH "Path to an existing JUCE checkout")
set(PXF_JUCE_TAG  "8.0.4" CACHE STRING "JUCE tag to fetch when PXF_FETCH_JUCE is ON")

if(PXF_JUCE_PATH)
    message(STATUS "PXF: using JUCE from ${PXF_JUCE_PATH}")
    add_subdirectory(${PXF_JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE)
elseif(PXF_FETCH_JUCE)
    message(STATUS "PXF: fetching JUCE ${PXF_JUCE_TAG}")
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${PXF_JUCE_TAG}
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(JUCE)
else()
    message(FATAL_ERROR
        "PXF: JUCE not found. Either leave PXF_FETCH_JUCE ON, or pass "
        "-DPXF_JUCE_PATH=<path to a JUCE checkout>.")
endif()

# ---------------------------------------------------------------------------
# The plugin
# ---------------------------------------------------------------------------
juce_add_plugin(PXF
    COMPANY_NAME                "PXF Audio"
    COMPANY_WEBSITE             "https://github.com/HenokHabtamu-72/PXF"
    BUNDLE_ID                   "com.pxfaudio.pxf"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE   # copy manually — Program Files needs admin
    PLUGIN_MANUFACTURER_CODE    Pxfa
    PLUGIN_CODE                 Pxf1
    VST3_CATEGORIES             "Instrument" "Synth"
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    FORMATS                     VST3 AU Standalone      # AU builds on macOS only
    PRODUCT_NAME                "PXF")

target_sources(PXF PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/Presets.cpp
    Source/SynthEngine.cpp
    Source/GrainEngine.cpp
    Source/FxChain.cpp
    Source/PxfLookAndFeel.cpp
    Source/UIComponents.cpp)

target_compile_definitions(PXF PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_MP3AUDIOFORMAT=1          # lets GRAIN load .mp3 one-shots
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_DISPLAY_SPLASH_SCREEN=1)      # see LICENSE: JUCE is AGPLv3 / commercial

target_link_libraries(PXF
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Headless engine tests (optional):  cmake -B build -DPXF_BUILD_TESTS=ON
# ---------------------------------------------------------------------------
option(PXF_BUILD_TESTS "Build the headless engine tests" OFF)

if(PXF_BUILD_TESTS)
    enable_testing()

    juce_add_console_app(pxf_tests PRODUCT_NAME "pxf_tests")

    target_sources(pxf_tests PRIVATE
        Tests/EngineTests.cpp
        Source/SynthEngine.cpp
        Source/GrainEngine.cpp)

    target_compile_definitions(pxf_tests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1)

    target_link_libraries(pxf_tests
        PRIVATE
            juce::juce_audio_formats
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    add_test(NAME pxf_engine_tests COMMAND pxf_tests)
endif()
