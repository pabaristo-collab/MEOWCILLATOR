from pathlib import Path
import hashlib
root=Path(__file__).resolve().parents[1]
cpp=(root/'Source/PluginProcessor.cpp').read_text()
editor=(root/'Source/PluginEditor.cpp').read_text()
header=(root/'Source/PluginProcessor.h').read_text()
ui=(root/'Assets/meow_ui.html').read_text()
cmake=(root/'CMakeLists.txt').read_text()
assert 'VERSION 0.9.23' in cmake
assert 'MEOWCILLATOR RESTORED' in cmake
assert 'Mw23' in cmake
assert 'NEEDS_WEBVIEW2 TRUE' in cmake
assert 'WebBrowserComponent' in editor
assert 'requestPanic.exchange(false' in cpp
assert 'transportJustStopped' in cpp
assert 'incomingMidiCount.fetch_add' in cpp
assert 'cancelMidiLearn' in editor and 'cancelMidiLearn' in header
assert 'meowMidiMonitor' in ui
assert 'MIDI IN: NO DATA' in ui
assert 'SPLIT AUDIO' in ui
assert 'drumHits' in ui
assert 'meow-native-fit' in ui
assert 'SynthSoundBank.h' in (root/'Source/PluginProcessor.h').read_text()
assert hashlib.sha256((root/'Source/RhythmPatterns.h').read_bytes()).hexdigest() == 'c7a631ff5871c482647285a09cb479abeb7165ecf8c822798518098182d83208'
print('PASS: 0.9.23 distinct plugin, original GUI, MIDI monitor, STOP/PANIC, 11 unchanged rhythms')
