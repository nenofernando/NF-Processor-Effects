cmake_minimum_required(VERSION 3.22)
# macOS target / universal binary: must be set BEFORE project() or CMake ignores it
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS Universal Binary")
project(NFProcessorEffects VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(NFPE_FETCH_JUCE "Fetch JUCE when not installed" ON)
# Owner's rule: new plug-ins get the NF License System at the END, with Paulo. OFF while developing.
option(NFPE_LICENSE "Enforce the NF License System (mute + activation overlay)" OFF)
option(NFPE_ENABLE_AAX "Build AAX (licensed SDK required)" OFF)
if(WIN32)
 set(_nfpe_home "$ENV{USERPROFILE}")
else()
 set(_nfpe_home "$ENV{HOME}")
endif()
set(NFPE_AAX_SDK_PATH "${_nfpe_home}/Documents/AAX_SDK" CACHE PATH "Licensed AAX SDK path")

find_package(JUCE CONFIG QUIET)
if(NOT JUCE_FOUND)
 if(NOT NFPE_FETCH_JUCE)
  message(FATAL_ERROR "JUCE not found. Set JUCE_DIR or enable NFPE_FETCH_JUCE.")
 endif()
 include(FetchContent)
 FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
 FetchContent_MakeAvailable(JUCE)
endif()

set(NFPE_FORMATS VST3)
if(APPLE)
 list(APPEND NFPE_FORMATS AU)
endif()
if(NFPE_ENABLE_AAX)
 if(NOT EXISTS "${NFPE_AAX_SDK_PATH}")
  message(FATAL_ERROR "NFPE_ENABLE_AAX requires the AAX SDK at NFPE_AAX_SDK_PATH (${NFPE_AAX_SDK_PATH}).")
 endif()
 juce_set_aax_sdk_path("${NFPE_AAX_SDK_PATH}")
 list(APPEND NFPE_FORMATS AAX)
endif()

juce_add_plugin(NFProcessorEffects
 COMPANY_NAME "NF Audio Tools"
 COMPANY_COPYRIGHT "NF Audio Tools by Nenno Fernando. (C) 2026 NF Audio Tools. All rights reserved."
 BUNDLE_ID "com.nfaudiotools.nfprocessoreffects"
 AAX_IDENTIFIER "com.nfaudiotools.nfprocessoreffects"
 PLUGIN_MANUFACTURER_CODE Nfat
 PLUGIN_CODE Nfpe
 FORMATS ${NFPE_FORMATS}
 PRODUCT_NAME "NF Processor Effects"
 PLUGIN_NAME "NF Processor Effects"
 DESCRIPTION "Dual reverb / effects processor: 16 reverbs, 6 delays, 8 modulation effects and 2 dynamics"
 IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE
 EDITOR_WANTS_KEYBOARD_FOCUS FALSE COPY_PLUGIN_AFTER_BUILD FALSE
 VST3_CATEGORIES Fx Reverb Delay
 AU_MAIN_TYPE kAudioUnitType_Effect
 AAX_CATEGORY AAX_ePlugInCategory_Reverb)
juce_generate_juce_header(NFProcessorEffects)

juce_add_binary_data(NFPEAssets HEADER_NAME NFPEBinaryData.h NAMESPACE NFPEBinaryData SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Assets/logo_nf_audio_tools_official.png"
 "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Manuals/NF_ProcessorEffects_Manual_Portugues.pdf" "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Manuals/NF_ProcessorEffects_Manual_English.pdf")
target_sources(NFProcessorEffects PRIVATE
 Source/PluginProcessor.cpp Source/PluginProcessor.h Source/PluginEditor.cpp Source/PluginEditor.h
 Source/DSP/Common.h Source/DSP/Catalog.h Source/DSP/Reverbs.h Source/DSP/Delays.h Source/DSP/Modulation.h Source/DSP/Dynamics.h
 Source/DSP/Engine.h Source/DSP/Audition.h Source/DSP/Programs.h)
if(NFPE_LICENSE)
 target_compile_definitions(NFProcessorEffects PUBLIC NF_LICENSE_ENFORCE=1)
endif()
target_compile_definitions(NFProcessorEffects PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0)
target_link_libraries(NFProcessorEffects PRIVATE NFPEAssets juce::juce_audio_utils juce::juce_dsp juce::juce_cryptography PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

add_executable(NFPEDspTests Tests/DspTests.cpp)
if(MSVC)
 target_compile_options(NFPEDspTests PRIVATE /W4)
else()
 target_compile_options(NFPEDspTests PRIVATE -Wall -Wextra)
endif()
enable_testing()
add_test(NAME NFPEDspTests COMMAND NFPEDspTests)

# Developer tool: renders the editor to a PNG (never shipped). cmake -DNFPE_BUILD_SNAPSHOT=ON
option(NFPE_BUILD_SNAPSHOT "Build the UI snapshot tool" OFF)
if(NFPE_BUILD_SNAPSHOT)
 juce_add_console_app(NFPESnapshot PRODUCT_NAME "NFPESnapshot")
 juce_generate_juce_header(NFPESnapshot)
 target_sources(NFPESnapshot PRIVATE Tests/UiSnapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
 target_compile_definitions(NFPESnapshot PRIVATE JucePlugin_Name="NF Processor Effects" JucePlugin_VersionString="1.0.0" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1 JucePlugin_Build_Standalone=0 JUCE_MODAL_LOOPS_PERMITTED=1 NF_LICENSE_TESTS=1)
 target_link_libraries(NFPESnapshot PRIVATE NFPEAssets juce::juce_audio_utils juce::juce_dsp juce::juce_cryptography juce::juce_gui_extra juce::juce_recommended_config_flags)
endif()
