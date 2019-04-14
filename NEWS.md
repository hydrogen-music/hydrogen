# 1.0.0-beta1*8000
- Usage of `auto` (in those contexts the type can be inferred by the
  object name, which is consistently used through the code), more
  consistent naming, and explicit `nullptr` instead of `NULL` in
  /src/core/src/hydrogen.cpp and some other files.
- Removing unused (obsolete) `Synth::m_pAudioOutput` and
  `Synth::setAudioOutput()`. Only the corresponding object and
  function of the `Hydrogen` class are used in the code base.
- Renaming
  - class AudioOutput -> AudioDriver
  - class MidiOutput -> MidiDriverOutput
  - class MidiInput -> MidiDriverInput
  - /src/core/include/hydrogen/IO/AudioOutput.h -> 
	/src/core/include/hydrogen/IO/AudioDriver.h
  - /src/core/include/hydrogen/IO/MidiOutput.h -> 
    /src/core/include/hydrogen/IO/MidiDriverOutput.h
  - /src/core/include/hydrogen/IO/MidiInput.h -> 
    /src/core/include/hydrogen/IO/MidiDriverInput.h
  - /src/core/src/IO/midi_input.cpp ->
	/src/core/src/IO/midi_driver_input.cpp
  - /src/core/src/IO/midi_output.cpp ->
    /src/core/src/IO/midi_driver_output.cpp
  - Hydrogen::getAudioOutput() -> Hydrogen::getAudioDriver()
  - Hydrogen::setAudioOutput() -> Hydrogen::setAudioDriver()
  - Hydrogen::setMidiOutput() -> Hydrogen::setMidiDriverOutput()
  - Hydrogen::setMidiInput() -> Hydrogen::setMidiDriverInput()
  - Hydrogen::__song -> Hydrogen::m_pSong
  - m_pMidiDriver -> Hydrogen::m_pMidiDriverInput
  - m_pMidiDriverOut -> Hydrogen::m_pMidiDriverOutput
- Introducing `Hydrogen::setMidiDriverInput()`,
  `Hydrogen::getMidiDriverInput()`, `Hydrogen::setMidiDriverOutput()`,
  `Hydrogen::getMidiDriverOutput()`, `Hydrogen::getAudioDriver()`, and
  `Hydrogen::setAudioDriver()` functions.
- Moving the `m_pAudioDriver`, `m_pMidiDriver`, and `m_pMidiDriverOut`
  objects into the `Hydrogen` class. 
