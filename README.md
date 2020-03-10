# Hydrogen drum machine

[![Travis Build Status](https://travis-ci.org/hydrogen-music/hydrogen.svg?branch=master)](https://travis-ci.org/hydrogen-music/hydrogen)
[![Coverity Scan Build](https://scan.coverity.com/projects/2965/badge.svg?flat=1)](https://scan.coverity.com/projects/2965)
[![Packaging status](https://repology.org/badge/tiny-repos/hydrogen.svg)](https://repology.org/project/hydrogen/versions)


![logo][logo]  
Hydrogen is an advanced drum machine for GNU/Linux, Mac and Windows.
It's main goal is to bring professional yet simple and intuitive pattern-based drum programming.

**Official Website:** http://www.hydrogen-music.org

[logo]: http://hydrogen-music.org/images/icon48.png

### Features

#### General

 * Very user-friendly, modular, fast and intuitive graphical interface based on QT 5.
 * Sample-based stereo audio engine, with import of sound samples in wav, au and aiff formats 
 * Support of samples in compressed FLAC file.

#### Sequencer and mixer

 * Pattern-based sequencer, with unlimited number of patterns and ability to chain patterns into a song.
 * Up to 192 ticks per pattern with individual level per event and variable pattern length.
 * Unlimited instrument tracks with volume, mute, solo, pan capabilities.
 * Multi layer support for instruments (up to 16 samples for each instrument).
 * Sample Editor, with basic cut and loop functions. 
 * Time-stretch and pitch functions via rubberband cli.
 * Playlist with scripting support
 * Advanced tab-tempo
 * Director Window with a visual metronome and song position tags
 * Timeline with variable tempo
 * Import/Export single patterns
 * Midi-Learn functionality for many gui elements
 * Multiple patterns playing at once.
 * Ability to import/export song files.
 * Unique human velocity, human time, pitch and swing functions.

#### Other features
 
* JACK, ALSA, PulseAudio, PortAudio, CoreAudio and OSS audio drivers.
* ALSA MIDI, JACK MIDI, CoreMidi and PortMidi input with assignable midi-in channel (1..16, ALL).
* Import/export of drumkits.
* Export song to wav, aiff, flac or file.
* Export song to midi file.
* Export song to LilyPond format.

### Screenshots
<details>
  <summary>Expand to view Hydrogen screenshots</summary>

#### Hydrogen Main Window
![main-window][screenshot1]

#### Hydrogen File Browser
![audio-filebrowser][screenshot2]

#### Hydrogen Sample Editor
![sample-editor][screenshot3]

#### Hydrogen Sound Library
![soundlibrary][screenshot4]

[screenshot1]: http://hydrogen-music.org/images/screenshots/main-window.png
[screenshot2]: http://hydrogen-music.org/images/screenshots/audio-filebrowser.png
[screenshot3]: http://hydrogen-music.org/images/screenshots/sample-editor.png
[screenshot4]: http://hydrogen-music.org/images/screenshots/soundlibrary.png

</details>

### Installation
More details in the [INSTALL.md](INSTALL.md) file.

### Packaging Status
<details>
  <summary>Expand to see the status of Hydrogen in the package ecosystem</summary>
  
  [![Packaging status](https://repology.org/badge/vertical-allrepos/hydrogen.svg?header=Hydrogen)](https://repology.org/project/hydrogen/versions)

</details>

### License
GPLv2 (more details in the [COPYING](./COPYING) file.)

Happy drumming!  :smiley:
