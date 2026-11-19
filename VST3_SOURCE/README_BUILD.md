# MEOWCILLATOR 0.9.22 — Native JUCE for VST3/Standalone

Pure C++/JUCE, no WebView2 or embedded browser. Windows 64-bit JUCE 8.0.15/Visual Studio 2022. Source package only, not a compiled plugin. See ../LEEME_PRIMERO.md for features, tests and installation.

## Build locally
- Install Git, CMake, Visual Studio 2022 C++ Desktop workload.
- Run `BUILD_WINDOWS.bat` inside this folder.
- First compile/run MeowBankTests and CTest, then build the plugin.
- After compilation, install `build/Meowcillator_artefacts/Release/VST3/MEOWCILLATOR.vst3` into `C:\Program Files\Common Files\VST3\` with REAPER closed.

## Build using GitHub Actions
- Upload the `VST3_SOURCE` directory and `.github/workflows` preserving their repository paths.
- Workflow: `Compilar MEOWCILLATOR VST3 - Windows x64`.
- Artifact name: `MEOWCILLATOR-0922-NATIVE-LOW-LATENCY-Windows-VST3`.
- Keep the existing VST3 installed until the new one passes DAW testing.

No promise of zero absolute latency: audio drivers/DAW buffers are outside the plugin. In REAPER use your audio interface's ASIO driver or WASAPI at 128–256 samples where stable.
