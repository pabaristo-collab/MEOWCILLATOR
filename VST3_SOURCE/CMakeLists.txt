cmake_minimum_required(VERSION 3.22)
project(Meowcillator VERSION 0.9.6 LANGUAGES C XXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(DEFINED JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
  add_subdirectory("${JUCE_DIR}" JUCE_build)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
  add_subdirectory("JUCE")
else()
  message(FATAL_ERROR "JUCE not found. Run BUILD_WINDOWS.bat to download JUCE 8 or specify -DJUCE_DIR=<path>.")
endif()
juce_add_plugin(Meowcillator
    COMPANY_NAME "pablo aristo"
    BUNDLE_ID "art.pabloaristo.meowcillator"
    PLUGIN_MANUFACTURER_CODE PbAr
    PLUGIN_CODE MwC5
    PRODUCT_NAME "MEOWCILLATOR"
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE)
juce_add_binary_data(MeowAssets SOURCES Assets/cat.png Assets/rat_wear.png)
target_sources(Meowcillator PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_include_directories(Meowcillator PRIVATE Source)
target_compile_definitions(Meowcillator PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Meowcillator PRIVATE MeowAssets juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
