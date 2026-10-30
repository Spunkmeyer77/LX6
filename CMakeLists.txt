cmake_minimum_required(VERSION 3.22)

project(LX6 VERSION 0.1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# JUCE: use a local checkout if JUCE_DIR is given, otherwise download it
if(DEFINED JUCE_DIR)
    add_subdirectory("${JUCE_DIR}" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.10
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

add_subdirectory(engine)

set(LX6_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND LX6_FORMATS AU)
endif()

juce_add_plugin(LX6
    PRODUCT_NAME "LX-6"
    COMPANY_NAME "Ticco Ross"
    COMPANY_WEBSITE "https://ticcoross.gumroad.com/"
    COMPANY_EMAIL "ticcoross@gmail.com"
    BUNDLE_ID com.ticcoross.lx6
    PLUGIN_MANUFACTURER_CODE TcRs
    PLUGIN_CODE Lx6d
    FORMATS ${LX6_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Drum
    DESCRIPTION "Six-voice digital drum synthesizer")

juce_add_binary_data(LX6Data SOURCES
    Resources/fonts/SpaceGrotesk-Medium.ttf
    Resources/fonts/SpaceGrotesk-Bold.ttf
    Resources/fonts/JetBrainsMono-Medium.ttf)

target_sources(LX6 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/LxrParams.cpp
    Source/Sequencer.cpp
    Source/HardwareLink.cpp
    Source/Randomizer.cpp
    Source/LX6LookAndFeel.cpp
    Source/AboutWindow.cpp)

target_compile_definitions(LX6 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    LX6_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(LX6
    PRIVATE
        lxr_engine
        LX6Data
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

option(LX6_BUILD_TESTS "Build the offline smoke test" OFF)
if(LX6_BUILD_TESTS)
    juce_add_console_app(LX6Test PRODUCT_NAME "LX6Test")
    target_sources(LX6Test PRIVATE tests/lx6_test.cpp
        Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/LxrParams.cpp Source/Sequencer.cpp
        Source/HardwareLink.cpp Source/Randomizer.cpp Source/LX6LookAndFeel.cpp Source/AboutWindow.cpp)
    target_compile_definitions(LX6Test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_ALSA=0 JUCE_JACK=0
        LX6_VERSION_STRING="${PROJECT_VERSION}" JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(LX6Test PRIVATE lxr_engine LX6Data juce::juce_audio_utils juce::juce_dsp)
endif()
