cmake_minimum_required(VERSION 3.22)
project(Meowcillator VERSION 0.9.23 LANGUAGES C CXX)
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
    BUNDLE_ID "art.pabloaristo.meowcillator.restored"
    PLUGIN_MANUFACTURER_CODE PbAr
    PLUGIN_CODE Mw23
    PRODUCT_NAME "MEOWCILLATOR RESTORED"
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    NEEDS_WEBVIEW2 TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE)
juce_generate_juce_header(Meowcillator)
juce_add_binary_data(MeowAssets SOURCES Assets/cat.png Assets/rat_wear.png Assets/meow_ui.html)
target_sources(Meowcillator PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_include_directories(Meowcillator PRIVATE Source)
target_compile_definitions(Meowcillator PUBLIC JUCE_WEB_BROWSER=1 JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
# Windows WebView2 safe optimization (/O1): validated to display the HTML UI
# in the existing REAPER 0.9.8 build. Do not restore /Ox here.
target_link_libraries(Meowcillator PRIVATE
    MeowAssets juce::juce_audio_utils juce::juce_dsp
    juce::juce_recommended_warning_flags)
if(MSVC)
    target_compile_options(Meowcillator PRIVATE "$<$<CONFIG:Release>:/O1>" /MP /EHsc)
else()
    target_link_libraries(Meowcillator PRIVATE juce::juce_recommended_config_flags)
endif()

# JUCE-independent DSP regression checks run in GitHub Actions before packaging.
add_executable(MeowBankTests tests/test_audio_banks.cpp)
target_compile_features(MeowBankTests PRIVATE cxx_std_17)
enable_testing()
add_test(NAME MeowBankTests COMMAND MeowBankTests)
