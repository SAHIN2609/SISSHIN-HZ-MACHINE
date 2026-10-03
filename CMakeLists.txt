cmake_minimum_required(VERSION 3.22)
project(SisshinHZMachine VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.8
  GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

# Web UI + JUCE's own JS bridge library, embedded into the plugin binary
juce_add_binary_data(HZAssets SOURCES
  web/index.html
  "${juce_SOURCE_DIR}/modules/juce_gui_extra/native/javascript/index.js")

juce_add_plugin(HZMachine
  COMPANY_NAME "SISSHIN"
  PLUGIN_MANUFACTURER_CODE Ssns
  PLUGIN_CODE Hzmc
  FORMATS VST3
  PRODUCT_NAME "SISSHIN HZ MACHINE"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  NEEDS_WEBVIEW2 TRUE
  VST3_CATEGORIES Fx Distortion
  COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(HZMachine PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)

target_compile_definitions(HZMachine PUBLIC
  JUCE_WEB_BROWSER=1
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)

target_link_libraries(HZMachine
  PRIVATE HZAssets juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
