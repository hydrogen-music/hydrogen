# Changelog

All notable changes to this project will be documented in this file.

## [2.0.0] - XXXX-XX-XX

### Added

- PortMidi driver does open a input/output port which can be discovered and
  connected to by other applications when port was set to "None" in the
  Preferences (not supported on Windows).
- All keyboard shortcuts including the virtual keyboard can now be customized in
  Preferences > Shortcuts.
- Most actions available via MIDI or OSC can also be assigned to a keyboard
  shortcut. If those actions require input arguments, they are retrieved using
  popup dialogs.
- Align to grid action in Pattern Editor to align existing notes to current
  grid.
- Switching drumkits, replace the current kit by a new one, as well as adding,
  deleting, and renaming drumkit components can now be undone.
- Pattern and drumkit properties can be edited by double clicking the
  corresponding name in the header of the Pattern Editor.
- new OSC commands:
  - `NEW_PLAYLIST`
  - `OPEN_PLAYLIST`
  - `SAVE_PLAYLIST`
  - `SAVE_PLAYLIST_AS`
  - `PLAYLIST_ADD_SONG`
  - `PLAYLIST_ADD_CURRENT_SONG`
  - `PLAYLIST_REMOVE_SONG`
  - `LOAD_PREV_DRUMKIT` (cycling through drumkits)
  - `LOAD_NEXT_DRUMKIT` (cycling through drumkits)
- new MIDI actions:
  - `LOAD_PREV_DRUMKIT` (cycling through drumkits)
  - `LOAD_NEXT_DRUMKIT` (cycling through drumkits)
- `.h2playlist` files are now backed up by and can be restored from autosave
  files as well.
- CLI options:
  - `kitToDrumkitMap`: to extract a .h2map file from a drumkit.
- Patterns are now independent of Drumkits and the latter can switched without
  the need to adjust the patterns. Mapping between the two will be done using
  "instrument types".
- `<instrumentComponent>` and `<instrumentLayer>` elements in drumkit XML
  definitions contain two new elements: `<isMuted>` and `<isSoloed>`.
- Notes not rendered by the audio engine are now highlighted in the pattern
  editor (can be disabled in Preferences > Appearance > Interface > Indicate
  note playback).
- Notes stopped by stop notes or the mute group feature are now rendered with a
  tail showing their effective length in the pattern editor (can be disabled in
  Preferences > Appearance > Interface > Indicate effective note length).
- Custom colors for 'mute' and 'solo'.
- When activating per-track JACK outputs, Hydrogen does now provided additional
  ports for metronome, playback track, and sample preview (#1150).
- Button in main toolbar indicating both MIDI input and output signals and
  opening a dialog showing the latest MIDI events.

### Changed

- Drumkit handling was reworked. Each song will now hold a proper drumkit.
  Tweaking its name, instruments etc. does not affect the kits in the Sound
  Library (user and system drumkit folder) unless it is explicitly saved to it.
  All actions in the Main Menu > Drumkit only affect the song's kit. All actions
  in the Sound Library do only affect the selected kit and not the one of the
  current song (except loading).
- `.h2song` file format changed. (forward compatible only with v1.2.4 and
  newer).
- Drumkit format changed:
  - Adjustments done in the Sample Editor are now stored in the kit as well
    (previously, this was only possible in songs).
  - Drumkit components (`<componentList>`) were dropped.
  - `sampleSelectionAlgo` was moved from `instrument` node into
    `instrumentComponent` nodes.
- (Instrument) Components are now independent of each other and can be added
  separately for each instrument.
- Notes in `.h2pattern` files as well as instruments in
  `.h2drumkits`/`drumkit.xml` do now have a new property `type`.
- Prerelease version suffix (alpha, beta, rc..) are now appended to the shared
  library of the core, `libhydrogen-core-X.X.X.so/.dll`.
- Preferences dialog can now be smoothly resized.
- MIDI:
  - Actions `>>_NEXT_BAR` and `<<_PREVIOUS_BAR` - as well as their OSC
    counterparts - do now wrap transport around the beginning and end of a song
    if loop mode is enabled. In pattern mode they move transport to be beginning
    and next is updating pending stacked patterns.
  - A MIDI control dialog listing all incoming and outgoing messages is now
    accessible via a button in the main toolbar.
  - All MIDI settings, which do not require a driver restart, were moved into
    the MIDI control dialog.
  - The MIDI channel presented in the Instrument Editor and the Settings is now zero-based (previously, it started at `1`).
  - Hydrogen does now only act on MIDI `START`, `CONTINUE`, and `STOP` messages,
    if "handling MIDI transport" is checked in the settings. Using this option
    it does now support `SONG_POSITION_POINTER` and `SONG_SELECT` as well.
  - `SONG_SELECT` messages will choose a pattern in pattern mode and a song from
    the current playlist in song mode.
  - When activating "handling MIDI clock" Hydrogen will adopt the tempo provided
    via MIDI Clock.
  - Only with both "handling MIDI clock" and "handling MIDI transport" activated
    Hydrogen will offer compliant MIDI sync support (#1598).
  - Hydrogen does also send `MIDI Clock`, `START`, `CONTINUE`, and `STOP`
    messages if the corresponding settings are checked.
  - `TAP_TEMPO` is now using a cumulative average (reset on tempo deviations
    larger than 20 bpm) to increase precision and is supported on Windows as
    well.
- PlaylistEditor:
  - All actions can now be undone and redone.
  - Songs can be loaded by keyboard selection and pressing Enter key.
  - Playlist can be rearranged using drag and drop.
- NSM:
  - Drumkits are no longer linked into the session folder
  - All drumkits in the session folder (proper drumkit folders containing
    samples. Linked ones are not supported) as available as session kits in the
    Sound Library.
  - New action in Main Menu to save kit to session folder.
- Online import dialog now allows for multiple downloads at once (#1143).
- `userVersion` element was added to `.h2song`, `.h2pattern`, and drumkit files.
  It can be set using the corresponding properties dialog and can help to tell
  different artifact versions apart.
- `formatVersion` element was added to `.h2song`, `.h2pattern`, `.h2playlist`,
  drumkit and config files. This integer will be increment each time the format
  will be changed.
- pre-fader gain does now include component gain as well.
- SongEditor:
  - Single cells can now be dragged and are affected by right-click popup menu
    without prior selection.
  - Switching between left-click selection or drawing was moved into the main
    toolbar.
  - Duplicating a pattern will duplicate all grid cells of the corresponding row
    as well.
  - Pattern Fill Dialog was reworked.
- PatternEditor:
  - Handling is now centered on interaction with existing notes. Only adding of
    new notes is dependent on the current grid.
  - The quantization button does now affect keyboard and mouse inputs too.
  - Alt key modifier can be used to temporarily turn off quantization.
  - Instruments of the current kit and instrument types of all notes in a
    pattern are now handled separately in the sidebar.
  - Single notes can now be dragged or altered using right-click popup menu
    without prior selection.
  - Notes not associated with an instrument of the current kit are now visible
    and can be handled like regular notes.
  - Pattern name is now shown together with other active patterns tab bar.
  - Property drawing in the properties ruler is now only bound to right mouse
    button.
  - Right-click drag in drum pattern and piano roll editor does not alter note properties anymore but draws notes.
  - Whether left mouse button interactions to select, draw, or editing existing
    notes can be set by buttons in the left-most part of the toolbar.
  - Note editing does now alter either note length or property. Not both.
  - Cursor, selection lasso, and selected and hovered notes are now synced
    between among all editors.
  - Property changes based on wheel and key event on the same set of notes as
    well as property drawing can now be undone/redone in a single action.
  - PianoRoll does now have a proper sidebar including per-line action in right
    click popup menu.
  - The hear notes button does now also affect sounds triggered by clicking the
    sidebar of the pattern editor.
- InstrumentEditor:
  - The "Layers" tab was replaced by a "Components" one containing collapsible
    versions of the previous view for all components.
  - Both components and layers can now be muted and soloed.
  - Adding, deleting, and replacing of layers can be undone.
- JACK per-track output ports are now mapped on drumkit switch or manipulation
  on instrument with same type. Ports of instruments without type aren't mapped
  at all (same as for notes) (#1071).
- Per default JACK per-track output ports are named according to the instrument
  types. But via Preferences > Audio > "Enforce instrument name instead of type"
  the previous naming scheme can be used.
- MIDI and audio drivers are now restarted separately in Preferences.
- Main toolbar was redesigned:
  - Status messages, CPU load, and number of XRuns were moved to the very bottom
    of Hydrogen.
  - Instead of showing text, all buttons are now icon based.
  - New buttons to switch between selection and drawing mode in Song and Pattern
    Editor.
  - New visibility buttons for playlist dialog, director, automation, playback
    track, and preferences dialog have been introduced.
  - The previous "S"/"P" button of the `BeatCounter` has been converted into a
    tap button. Press long to switch between the two beat counter modes or the
    usage of the plain tap tempo and use regular clicking to adjust tempo.
  - If JACK Timebase support was disabled in Preferences, the button is now
    hidden instead of disabled.
    

### Fixed

- Components can now carry arbitrary names and name duplication is handled
  properly.
- Fix sample selection in the presence of multiple components.
- Fix rendering of clicked layers in the Instrument Editor (#1350).
- Paths to songs and scripts are now properly saved relative to a `.h2playlist`
  file (in case the corresponding option was set).
- Stop notes do now stop all notes of the same instrument at the same position
  (e.g. in chords or using multiple patterns).
- MIDI export:
  - Does now respect virtual patterns (#513).
  - Does now add tempo changes and tags in case the Timeline is used (#933).
  - Does now add time signature events based on the length of the longest
    pattern within a column (#1680).
- MIDI import is now able to trigger multiple instruments using a single NOTE ON
  event (#1765).
- Fix mixup of JACK per-track output ports on instrument reordering (see Changed
  section for new behaviour) (#71).

### Removed

- Preferences options `restoreLastSong` and `restoreLastPlaylist` were dropped.
  Instead, both will always be restored automatically. In order to start with a
  blank song/playlist, load a new one before quitting.
- Component mixer strips have been dropped.
- `LASH` support has been dropped (#1649).
- `cmake` option `MAX_NOTES` has been dropped.

## [1.2.6] - 2025-07-29

### Added

- The AppImage version does now provide all drumkits in
  `/usr/share/hydrogen/data/drumkits/` and
  `/usr/local/share/hydrogen/data/drumkits/` via the "Sound Library" as session
  kits.

### Changed

- Sample loading in songs has been made more portable.

### Fixed

- Fix velocity automation for patterns that are not of size 4/4 (#2171).
- Fix saving and loading of sample files introduced in 1.2.5 (#2174).

## [1.2.5] - 2025-07-17

### Added

- CLI options `--log-colors` and `--no-log-colors` to enable and disable ANSI
  colors in log messages.
- CLI option `--user-data` for both `hydrogen` and `h2cli` to provide an
  alternative user-level data folder.
- CMake option `-DWANT_QT6` to build Hydrogen using Qt6 instead of Qt5.

### Changed

- The shortcut for starting/pausing transport can now be used while focusing
  spin boxes (like the BPM one) too (#2098).
- Combo boxes do not accept focus (and preventing shortcuts) anymore.
- LASH support has been deprecated and will be removed in version 2.0 of
  Hydrogen (#1649).
- Allow to select and copy version in About dialog (#2127).
- Number of parallel build jobs in `build.sh` is now set dynamically to number
  of virtual processors (#2128).
- `Changelog` was renamed `CHANGELOG.md` and converted from GNU-style changelog
  to markdown similar to https://keepachangelog.com/en/1.1.0/.
- Hydrogen no longer uses XSD files to validate XML files during loading or via
  `h2cli --check` but uses direct XML inspection instead.
- Minimal `CMake` version was bumped to `3.5`.
- Option `-qt5` was introduced to native Windows build script
  `windows/Build-WinNative.ps1` to build against Qt5 instead of Qt6 (default).
- `build.sh` script compiles against Qt6 per default.
  
### Fixed

- Fix compilation with LASH support enabled (#2076).
- Fix Hue slider in Preferences > Appearance > Color (#2081).
- Show the Crash Reporter and exit with return code `1` on unhandled exceptions.
- Fix crashes in SampleEditor (#2092).
- Fix track names in multi track export. When using just the file extension, the
  raw instrument names will be used (#2096).
- Fix import bug for drumkits created in version >= 2.0.
- Fix memory leakage for songs created in version >= 2.0.
- Fix memory leakage for notes with probability < 1.0.
- Fix incoming MIDI NOTE OFF handling.
- AppImage build folder is now removed on `build.sh r` (#2129).
- Fix potential crash with JACK audio driver on startup, teardown, or
  song/drumkit loading.
  
### Removed

- `Qt XmlPatterns` is no longer a dependency.
- Folder `linux/debian` containing outdated Debian package rules used by the
  previous development team to distributed Hydrogen as `.deb` package.
- Option `-32bit` was dropped in native Windows build script
  `windows/Build-WinNative.ps1`.

## [1.2.4] - 2024-12-07

### Added

- Forward compatibility for `.h2song`, `.h2pattern`, `.h2playlist`, and drumkit
  changes introduced in version 2.0.
- MIDI and OSC commands:
  - `CLEAR_SELECTED_INSTRUMENT`: to remove all notes of the selected pattern
    associated with the currently selected instrument.
  - `CLEAR_PATTERN`: to remove all notes of the selected pattern.
  - `INSTRUMENT_PITCH`: to adjust the pitch of an instrument.
- OSC commands:
  - `NOTE_ON` and `NOTE_OFF`: which are handled like incoming MIDI events
    without triggering their associated actions.
  - `CLEAR_INSTRUMENT`: to remove all notes of the selected pattern associated
    with the provided instrument number.
- CLI options:
  - `-O`/`--osc-port`: to use a custom OSC port in both `hydrogen` and `h2cli`.
  - `-L`/`--log-file`: to provide a path to an alternative log file.
  - `-T`/`--log-timestamps`: to add timestamps to all log messages.
  - `--config`: to use a different user-level config file.
  - `--compression-level`: for `h2cli` to set the trade-off between max. quality
    (`.mp3` and `.ogg`)/max. speed (`.flac`) (`0.0`) and max. compression
    (`1.0`) for exported audio files.
- Adding support for importing and exporting audio files of format `.mp3`,
  `.opus`, `.au`, `.caf`, `.voc`, `.w64`. Drumkits containing those formats can
  be loaded with older versions of Hydrogen too. `libsndfile` >= `1.1.0` is
  required on your system for MP3 support. (#2023)
- `X-NSM-Exec` entry added to `org.hydrogenmusic.Hydrogen.desktop` by
  @grammoboy2 (#2042).
- Delete key does now remove selected notes and notes under cursor in note
  properties ruler.
  
### Changed

- Brazilian translation updated.
- Grid lines in the Song Editor are now rendered dotted to emphasize that this
  is the space the patterns in rather than objects in their own right.
- Virtual keyboard strokes are now mapped exactly as incoming MIDI `NOTE_ON`
  events (respecting both "Use output note as input note" and hihat pressure
  groups). But do not trigger associated actions (#1770).
- CLI option `-d` understand driver names regardless of capitalization.
- `h2cli` option `-V` is now able to handle whitespaces between flag and
  argument.
- `h2cli` long option for `-k` is renamed `--drumkit` -> `kit` in order to align
  the naming with the one used in `hydrogen` CLI options.
- Smaller keyboard cursor size with resolution set to `off`.
- Rename "J.Master" button into "Timebase".

### Fixed

- Fix potential segfault on ill-formated notes in .h2song files.
- Fix buzzing sound during startup when using Port Audio (#1932).
- Fix build failure without precompiled headers (e.g. on Gentoo) (#1944).
- Fix persistent of hihat pressure group settings while changing/restarting MIDI
  drivers.
- Fix mapping of `NOTE_OFF` MIDI messages in hihat pressure groups.
- Fix segfault when using MIDI sense button in table of Preferences > MIDI after
  removing rows above it from the table.
- Fix synchronization problems while using JACK Timebase support (#1953).
- Fix compilation error on macOS with case-sensitive filesystem (#1938).
- Fix usability with large `QT_SCALE_FACTOR` (#1933).
- Fix MIDI, WAV, and LilyPond export as well as drumkit import and export with
  with non-ASCII filenames (#1957).
- Paths to songs and scripts are now properly saved relative to a `.h2playlist`
  file (in case the corresponding option was set).
- Fix grid line rendering with resolution set to `off` (#2015).
- Fix handling of adjacent tags with same content in Director by @aldimond
  (#2036).
- Fix copy/cut/paste in Piano Roll Editor.

### Removed

- Windows 32bit support dropped (due to upstream limitations).

## [1.2.3] - 2024-01-12

### Added

- Forward compatibility for `.h2song` and drumkit changes introduced in version
  2.0.
  
### Fixed

- Restore mute button state of master mixer strip on song load.
- Recorded MIDI notes were inserted ahead of the beat (#1851).
- Drumkit Property Dialog:
  - Images were written regardless whether one hits the ok or cancel button.
  - When using _Save As_ to create a new drumkit, the added image was put in the
    old drumkit folder instead and not properly copied into the new one.
- Allow to _Save As_ drumkits derived from kits not found on the current system.
- Audio Engine: In Song Mode with Loop Mode deactivated Hydrogen missed notes
  very close to the end of the song.
- Fix crash on playing back notes with custom length (#1852).
- macOS: fix naming of CoreMIDI header (#1865).
- Fix various rendering issues with custom length notes.
- Fix potential crash/failing startup on Windows in case PortAudio or PortMidi
  device is already occupied (#1893)
- Fix crash on shutdown, song export, or driver changes in the Preferences while
  using JACK on Linux (#1902, #1867, #1907)
- Pattern Editor:
  - Only delete stop notes clicked by the user. (#1859)
  - Proper undo of moving notes out of Drum Pattern Editor. (#1859)
  - Custom note lengths are now only drawn till the next stop note. (#1859)
  - Highlight selected stop notes too. (#1859)
  - Update selected notes visually on left and right keyboard movement. (#1859)
  - Fixed stop note color which was no different than the default note color
    (#1854).
  - Fixed grid line rendering on rational pattern size nominator.
  - Fixed grid line colors on very fine resolution.
- Fix broken file browser dialogs on Linux when using translations (#1908).
- Fix drumkit export on Windows (#1927).
- Timing drift (and artifacts) in playback track rendering on some hosts is
  fixed (#1920).

## [1.2.2] - 2023-09-09

### Added

- Hydrogen is now released as AppImage for Linux as well.
- Playlist dialog can now be resized and remembers geometry, position, as well
  as visibility.
- Save and restore Director position, geometry, and visibility.
- New and properly licensed AppStream metainfo files
  `org.hydrogenmusic.Hydrogen.metainfo.xml` replace old
  `org.hydrogenmusic.Hydrogen.appdata.xml`.
- Hydrogen shows an error dialog and exits on the first invocation with no
  `hydrogen.conf` file present on user and system level (application was not
  properly installed).
- A new `cmake` option `WANT_DYNAMIC_JACK_CHECK` was introduced. When set
  Hydrogen does check on startup whether `jackd`, `jackdbus`, or `pw-jack` is
  installed and disables JACK support in case none of them was found. This is
  intended for bundled builds, like AppImage and Flatpak, and can be overridden
  by setting the audio driver manually to "Jack" in the `hydrogen.conf` or by
  passing the `-d jack` CLI option.

### Changed

- Spanish translation updated.
- Minor tweaks in French and German translation.
- On Linux the order of audio drivers tried when selecting `Auto` changed from
  "JACK > ALSA > OSS > PulseAudio" to "JACK > PulseAudio > ALSA > OSS".

### Fixed

- Style combo box in Preferences > Appearance > Interface is working again.
- Fix segfault on hitting "Panic" button while transport was rolling.
- Instrument/strip-specific actions, like MIDI action `STRIP_MUTE_TOGGLE`, did
  void the instrument selection of the Instrument Editor if the specified
  instrument was not the currently selected one.
- Do not start playback at cursor when cursor in Song Editor is beyond the
  current song length.
- Fixed compatibility with PortMidi version 217 (Hydrogen v1.2.1 was
  incompatible). All versions of Hydrogen >=1.3 will, however, require on a more
  recent PortMidi version (at least v2.0.1) (#1795).
- Fixed allowed range of MIDI output notes to be [0,127] again (introduced in
  v1.2.0) (#1828).
- Hydrogen does now successfully startup even if no data folder is present on
  user and system level.
- Allow an arbitrary number of notes in a pattern (#1827).
- Fix playback track volume fader (#1449).

2023-06-08 the hydrogen team <hydrogen-devel@lists.sourceforge.net>

## [1.2.1] - 2023-06-08

### Changed

- Update French translation

### Fixed

- Fix reopening of last used Playlist. In addition, in case the PlaylistDialog
  was opened at the end of the last session - when "Reopen last used playlist"
  is checked in the Preferences - the dialog will be reopened too at the same
  position.
- Fix spurious marking of opened songs as modified.
- Fix MIDI (output) feedback for metronome toggling and pan setting.
- Fix superfluous MIDI event - Action bindings. An incoming MIDI event can be
  mapped to an Action only once.
- Fix tool tips of MIDI-learnable widgets. All bounded MIDI events will be
  shown.
- Fix MIDI note output for channel 16 (previously only channel 1-15 were
  accessible in the InstrumentEditor).
- Fix spurious tempo changes to 120bpm when switching songs or between pattern
  and song mode (#1779 and #1785).
- Support `START`, `CONTINUE`, and `STOP` type System Realtime MIDI messages in
  PortMidi and CoreMidi.
- Fix MIDI action binding to incoming `MMC_DEFERRED_PLAY` event.
- Fix missing MIDI driver restart when adjusting corresponding parameters in
  Preferences.
- Fix MIDI Machine Control (`MMC`) event type handling on Windows (#1773).
- Fix loading of legacy drumkits. All layers but the first one were dropped
  during drumkit upgrade (#1759).
- Fix MIDI input handling with "Discard MIDI messages after action has been
  triggered" checked. Incoming `NOTEON` message were dropped without triggering
  a sound (#1751).
- Fix beat and bar calculation in pattern mode (#1741).
- Fix compilation in GCC with -Werror=format-security (#1739).
- Explicit usage of Python3 in stats.py script.
- Fix build against Musl by @nekopsykose.
- Omit git commit hash in displayed version of release builds.

## [1.2.0] - 2023-04-07

### Added

- Introducing keyboard shortcut for the Open Pattern dialog.
- Allow for opening more than one Pattern at once.
- Implement missing `EFFECT_LEVEL_RELATIVE` MIDI action.
- Drumkit properties dialog does now feature a table listing all contained
  samples and associated licenses.
- OSC commands:
  - `/Hydrogen/LOAD_DRUMKIT`
  - `/Hydrogen/UPGRADE_DRUMKIT`
  - `/Hydrogen/VALIDATE_DRUMKIT`
  - `/Hydrogen/EXTRACT_DRUMKIT`
  - `/Hydrogen/BPM`
- `h2cli` options:
  - `--upgrade`: to upgrade a drumkit.
  - `--check`: to validate a drumkit.
  - `--extract`: to extract the content of a drumkit.
- Crash reporting: Fatal errors will now show a GUI report including details to
  report and potential hints about the cause.

### Changed

- Remembering paths in all export/import/save/open dialogs.
- If the Timeline is activated, the tempo set using the BPM widget, BeatCounter,
  Tap Tempo, or MIDI/OSC commands is used left of the first tempo marker.
- The tempo provided by an external JACK timebase master overwrites all internal
  tempo settings.
- The BPM widget switches to read-only mode and displays the current playback
  speed when the Timeline is activated.
- Activation of the Timeline is now stored in each individual `.h2song` file.
- Autosave files will be hidden. The interval they are stored with as well
  whether there is an autosave at all can be set via the Preferences. Hydrogen
  will inform the user whether there are unsaved changes to recover taken from
  the autosave file.
- Hydrogen is now able to recover changes applied to a new and empty song in
  case they are discarded or the session end untimely (using autosave files).
- Multiple actions can be assigned to a single MIDI event.
- The virtual keyboard is now decoupled from the "Hear New Notes" button in the
  Pattern Editor and can be used to play back notes in song mode with playback
  rolling too.
- Mutable warning dialogs are shown when saving/exporting a drumkit containing
  samples of mismatching license and when saving/exporting a drumkit or song
  containing a copyleft license or one requiring an attribution.
- All actions accessible via right-clicking in the SoundLibrary do affect the
  stack drumkits and not the instrument list of the current song.
- All actions accessible via the Drumkits tab of the main menu do affect the
  loaded drumkit using the current song's custom instrument and component list.
- GUI:
  - Improved scalability (most PNG images were replaced by SVGs, hardcoded PNG
    labels are now directly drawn by Qt, and spin boxes, buttons, and combo
    boxes are now based on native Qt widgets).
  - Improved internationalization (all labels are translatable now and support
    UTF-8).
  - Improved accessibility (widgets were increased to fill available space,
    preference option to (de- and) increase font size).
  - All colors can be altered via the preferences.
  - Input widgets (rotary, fader, combo box, spin box, button) do support both
    mouse and keyboard input (e.g. setting numerical values).
  - Input widgets are faintly highlighted when hovered and more strongly
    highlighted when clicked (focuses). Only the focused widget can receive
    keyboard input.
  - MIDI-learnable widgets now show their corresponding MIDI action and it's
    binding in the tooltip.
  - It's now possible to jump to the beginning of the currently playing pattern
    by clicking its position on the ruler in the pattern editor.
  - The length of patterns can now be changed while transport is rolling.
- PreferencesDialog > Appearance tab overhaul:
  - Drop previous font options in favor for three different levels of font
    (without exposing their point sizes).
  - Ability to decrease or increase the overall font size.
  - All settings in the Appearance tab - except the overall layout and the
    scaling policy - take effect immediately (no restart required anymore).
  - Drop the "fixed" coloring methods and rename "steps" to "Custom".
  - Via a colored button the particular line color can now be adjusted using a
    QColorDialog.
  - Custom color tab to alter all colors in the GUI (big thanks to the Muse4
    team for the original code).
  - All options set in the Appearance tab can be imported and exported into and
    shared via dedicated `.h2theme` files.
  - Overhauled MIDI Table:
    - Per effect or layer setting are now done in dedicated spin boxes instead
      of providing individual actions for each.
    - Only spin boxes for parameters required for a particular action are shown.
    - Rows can be removed by unsetting both the Event and Action combo box.
  - Clicking the widget showing the status messages does now open a dropdown
    menu displaying the last 100 messages.
- Set CoreAudio (macOS) buffer size to control latency
- New fast exponential ADSR envelope processing
- Start/end/loop frame slider selection and motion as well as velocity/pan
  envelope editing in Sample Editor have been reworked.
- SongEditor UX improvements:
  - Tags can now be inserted at all possible columns.
  - Hovering a Tag displays its content.
  - Changed behavior of the Tempo Marker dialog: When clicking an existing Tempo
    Marker it can be moved, edited, or deleted. Clicking a column with no Tempo
    Marker present allows to create a new one at an arbitrary location.
  - Timeline is deactivated automatically when switching to Pattern Mode or a
    JACK timebase master is present.
  - The tempo used left of the first tempo marker is painted in a darker color.
  - The currently used Tempo Marker gets highlighted.
  - Tags have been moved into Timeline (next to the Tempo Markers) in order to
    make room to accommodate the cursor in the ruler to highlight the current
    position.
  - Tags can be inserted by left-clicking the bottom area of the Timeline (above
    the ruler).
  - Clicking the ruler is now always enabled and automatically switches
    transport into Song Mode.
  - Full-size playhead.
  - The icons in the pattern list indicating whether a pattern is playing in
    stacked pattern mode are now colored and can have four different states: on,
    off, off next (pattern is played till the end and then turned off), and on
    next (pattern is played as soon as transport is looped again).
  - In song mode the pattern editor can be locked meaning that always the
    bottom-most pattern of the current column the playhead resides in as well as
    all other playing notes are shown. Pattern selection is done automatically
    when moving into a different column.
- PatternEditor UX tweaks:
  - Relocating transport by clicking the ruler is now supported (like in the
    SongEditor) and automatically switches transport into Pattern Mode.
  - Full-size playhead.
  - The ruler was decoupled from the currently selected pattern. It always has
    the size of the largest playing pattern and always shows the transport
    position using a playhead. Whether or not the current pattern is played back
    is indicated by a full-height cursor.
  - All note properties except of the note key can now be altered in both the
    drum pattern editor and the piano roll editor by right-clicking and dragging
    a note.
  - All notes of the currently playing patterns will be hinted in stacked
    pattern mode, when selecting a virtual pattern, or in case the pattern
    editor is locked in song mode. Even those notes exceeding the length of the
    current pattern are shown.
- OSC commands `/Hydrogen/STRIP_SOLO_TOGGLE/X` and
  `/Hydrogen/STRIP_MUTE_TOGGLE/X` can now be called without any argument too.
- `h2cli`: Drumkit supplied using `-k` option can now be either the name of an
  install kit or an absolute path to a kit (does not have to be located in the
  Hydrogen's drumkit folder).

### Fixed

- All available audio drivers can now be chosen via CLI.
- Fix dithering of SongEditor when viewing the playback track and resizing the
  application or for very small size (#1379).
- Fix rewinding to beginning of pattern in pattern mode with no pattern inserted
  in SongEditor (#932).
- Fix display of tags and tempo marker while loading a song (introduced in
  1.1.0) (#1393).
- Default MIDI driver is now picked with the system's capabilities in mind.
- Remove unwanted samples at the end of the exported song (to `.wav`) (#946).
- Fix undefined tempo before first tempo marker (#416).
- Fix Song export to keep writing till all notes and FX have faded out.
- Notes at first ticks in pattern were missed when relocation transport using
  the position ruler.
- Fix playhead glitches when adding toggling patterns at the end of the song
  after transport was loop at least once.
- Deactivating loop mode result stopping transport at the end of the song even
  if transport was already looped at least once.
- Relocation of the playhead use JACK is now also support in case transport is
  not rolling.
- Hydrogen is now able to handle multiple drumkits featuring the same name in
  the SoundLibrary. The drumkit's absolute path will be used as unique handler
  from now on.
- Mixing instruments from different drumkit in one Song works again.
- Hydrogen shows a now a warning dialog when exporting/saving date to read-only
  folders.
- Fix text overflow in Director.

## [1.1.1] - 2021-12-05

### Fixed

- Fix preferences dialog "OK" button behaviour (#1375).
- Fix rewinding to beginning of pattern in pattern mode with no pattern inserted
  in SongEditor (#932).
- Fix display of tempo marker while loading a song (introduced in 1.1.0)
  (#1393).
- Fix LADSPA issues, most audible on PulseAudio (#1403).
- Fix window placement when screen sizes change (#1369).
- Explicitly set latency target for PortAudio (Windows) audio driver, enabling
  much lower audio latency on Windows.

## [1.1.0] - 2021-09-04

### Added

- Keyboard cursor-driven pattern, song and note properties editing.
- Note selection and movement in pattern editors.
- New OSC commands (new song, open song, save song (as), quit, toggle looped
  playback, toggle timeline, toggle JACK transport, toggle JACK timebase master,
  add/delete timeline marker, toggle song/pattern playback mode).
- Custom pan law support in mixer.
  
### Changed

- NSM support reworked.
- Deprecating JACK-session.
- Instrument main pitch shift offset.
- Custom pattern size support with representation in note values.
- Allow audio device selection for CoreAudio and PortAudio.
- Attempt to find fallback Audio drivers if user preferred driver fails, as if
  the "Auto" driver option were selected.
  
### Fixed

- Many bug fixes including application crashes and audio glitches.

## [1.0.2] - 2021-04-11

### Added

- User-selectable translation language.

### Fixed

- Bug fixes including critical startup bug on macOS Big Sur.

## [1.0.1] - 2020-08-29

### Changed

- Disabled "development mode" warning.

### Fixed

- Fixed compilation with libtar.
- Fixed JACK transport tempo drift when using non-integer bpm values.
- Fix crash on saving ladspa settings.

2020-08-03 the hydrogen team <hydrogen-devel@lists.sourceforge.net>

## [1.0.0] - 2020-08-03

### Added

- QT 5 support.
- New default drumkit: GMRockKit.
- Probability note property.
- Velocity automation.
- OSC support.
- MIDI feedback.
- Fullscreen mode.

### Changed

- Menu redesign.

## [0.9.7] - 2016-11-01

### Added

- The color of the SongEditors squares is now configurable.
- Added support for MIDI cymbal choking.
- Added support for MIDI hihat pressure control.
- Added hihat pressure groups.
- Added basic non session manager (NSM) support.
- Added instrument components.
- Basic lilypond export.
- New windows cross compilation script.
- Support for soundlibrary images.
- Configurable sample selection algorithm.
- Donation dialog.
- MIDI action:
  - `SELECT_PREV_PATTERN_RELATIVE`.

## [0.9.6] - 2014-08-03

### Added

- New build system (`cmake`).
- Add undo for song/pattern editor.
- Jack-session support.
- Jack-MIDI support.
- Several bug fixes.
- Tabbed interface.
- Several small changes to the GUI.
- Improve ExportSong add use of TimeLineBPM, RubberbandBatch processor and
  different types of resample interpolation.

## [0.9.5] - 2011-03-15

### Added

- Multi-track export.
- LADI support.
- Maximum number of bars is now configurable.
- Added czech translation.
- Added `.ogg`, `.flac`, `.aiff` export support.
- Added some new commandline parameter for no-GUI version.
- Added `rubberband-cli` support.
- Added `NO_GUI_SUPPORT` to build a version of hydrogen without a GUI.
- Added support for app bundles on OSX.
- Non destructive sample editor.
- Piano roll editor.
- Instrument midi out.
- Destructive MIDI recording.
- Support for MIDI "note off" messages.
- Virtual patterns.
- Time line to change song tempo and add tags.
- Director.

### Changed

- Load playlists at startup.
- MIDI-learn works now with shift-click on some GUI elements.
- Several improvements on sample editor.
- XML handling is now done by QtXml instead of TinyXML.
- Improved support for non-ASCII filenames / strings.
- Remove direct dependencies to `libflac`.

### Fixed

- Fixed several export song failures.

## [0.9.4] - 2009-09-12

### Added

- QT4 port.
- Autosave.
- Mute groups for instruments.
- New drumkit manager (with downloadable drumkits).
- Save and load patterns.
- JACK transport master.
- Beatcounter.
- Playlist editor.
- MMC.
- Lead / lag (pattern editor).
- Audio file browser.
- MIDI Autosense.
- Change post master fader fx return to pre master fader fx return.
- Several new translations.
- New manual.
- Switched buildsystem from `autotools` to `scons`.
- Soundlibrary browser.

### Changed

- JACK port follows instrument names.

### Fixed

- Tons of bugfixes.

## [0.9.3.1] - 2008-02-05

### Added

- Added LASH support (Jaakko Sipari).

### Fixed

- Patch from Lubomir Kundrak - Compilation with gcc-4.3.
- Compilation fix for new libFLAC++.

## [0.9.2] - 2005-07-23

### Added

- New graphics.
- Automatic audio driver selection.
- New PortAudio and PortMidi drivers.
- Mac Os X port.
- Random pitch variations.
- New instrument editor.
- Low pass filter.
- Insert/delete a range of patterns in song editor.

### Changed

- Follow playhead in song editor.
- Pattern size increased up to 4 bars.

### Fixed 

- Bug fix in export song (using JACK driver).
- Bug fix in export song (wrong samplerate).

## [0.9.1] - 2004-11-28

### Added

- New ALSA driver.
- New french tutorial and manual page (thanks to Pierre 'AlSim' Chapuis).

### Fixed

- Various bug fixes.

## [0.9.0] - 2004-09-08

### Added

- Multi layer support for instruments (up to 16 samples).
- Multiple patterns playing at once.
- Added FLAC files support for songs and drumkits.
- Added pitch and gain properties per instrument.
- Added a new selectable user interface (single panel).
- Ability to set the note length in pattern editor.
- Export song to standard MIDI file.

### Changed

- Improved song and pattern editor (selections, copy/move, etc..).
- Better jack-transport support.

## [0.8.2] - 2004-03-15

### Added

- Audio file preview in load instrument dialog.
- 4 Ladspa FX send per instrument.
- Show recent used songs.
- QT Style selection option in Preferences Dialog.
- New keybindings.
- `LRDF` support (optional).
- Virtual keyboard (using qwertyuiop...).
- Ability to record midi-in or virtual keyboard notes in a pattern.

### Changed

- JACK transport improvements.
- Several GUI improvements.
- Better MIDI-in support.

### Fixed

- Bug fix in load/save song.

## [0.8.1] - 2003-12-18

### Added

- MIDI and CPU activity widgets.
- JACK transport slave mode.
- Multiple JACK outputs.
- Resizable song editor.
- Seek in song clicking in the song editor.
- New mixer.
- 32 instruments.
- Custom pattern size.
- Per instrument output (JACK driver).
- `MMC`/`MTC` support (experimental).
- i18n support.

## [0.8.0] - 2003-05-25

### Added

- Delay FX.
- Assignable JACK ports in preferences file.
- Assignable MIDI-in channel (1..16, ALL).
- Drumkit support (load, save, import, export).
- Acoustic drumkit included.

### Changed

- Various GUI improvements.

### Fixed

- Bug fix in ALSA MIDI Driver.

## [0.7.6] - 2003-03-24

### Added

- Humanize function.
- Swing function.
- New child frame interface.
- Stereo peak visualization in mixer.
- New song editor.

### Changed

- Improved pattern editor.

### Fixed

- Bug fix in note velocity editor.

## [0.7.5] - 2003-02-23

### Added

- Song loop button.
- Velocity bar in pattern editor.
- Instrument pan in mixer.
- Demo songs.

### Changed

- Better graphical user interface.

### Fixed

- Thread safe audio engine.
