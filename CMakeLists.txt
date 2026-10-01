cmake_minimum_required(VERSION 3.22)

project(snowsBlockade VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Link the MSVC runtime statically so the plugin is a single self-contained
# file with no VC++ redistributable dependency. Must be set before JUCE is added.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(snowsBlockade
    COMPANY_NAME "Snows"
    PLUGIN_MANUFACTURER_CODE Snow
    PLUGIN_CODE Sblk
    FORMATS VST3
    PRODUCT_NAME "snowsBlockade"
    VST3_CATEGORIES "Fx"
)

juce_add_binary_data(snowsGuiAssets
    HEADER_NAME SnowsGuiAssets.h
    NAMESPACE snowsguiassets
    SOURCES Source/assets/snowlogo.svg
)

target_sources(snowsBlockade
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(snowsBlockade
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(snowsBlockade
    PRIVATE
        snowsGuiAssets
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
