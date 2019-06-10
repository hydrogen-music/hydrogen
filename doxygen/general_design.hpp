/**
 * \page selectedTopics Selected Topics
 * \tableofcontents
 *
 * A couple of summaries that aim for giving you a accessible
 * introduction to the main aspects of Hydrogen.
 *
 * Most importantly, the overall concept of the application is sketched
 * in section \ref generalDesign. It can be considered a must-read
 * regardless of which part of Hydrogen you are interested in.
 *
 * \section generalDesign General Design
 *
 * Viewed on an abstract and slightly oversimplified level Hydrogen
 * can be considered to consist of two independent parts: its **core**
 * and its **GUI** (both written in C++ and the latter using
 * Qt5). They are not just separated physically at the source code
 * level (see \ref structureCode) but also in terms of their
 * responsibilities.
 *
 * \subsection generalCore Responsibilities of the core
 *
 * - Creates the audio by combining samples from the drumkit
 * - Contains the audio engine and controls playback/transport
 * - Talks to the sound card and operation system
 * - Loading and saving of songs and preferences
 * - Holding representations of patterns, drumkits, samples, songs
 *   etc. 
 * - Receives, processes, and sends messages like MIDI or OSC
 * - Encapsulated in the \ref H2Core namespace
 * - Runs in real-time.
 *
 * \subsection generalGUI Responsibilities of the GUI
 *
 * - Displays the current state of the core, like transport position,
 *   selected song, and pattern
 * - Used to alter the state of the core (e.g. by changing
 *   preferences, creating and modifying patterns, or adjusting
 *   volumes)
 * - Conveniently bundles properties in views, like the mixer or the
 *   pattern and instrument editor
 * - Updated 20 times per second and is not real-time save
 *
 * \subsection generalInteraction Interaction between the core and GUI
 *
 * ![sketch of interaction](../../doxygen/interaction.svg)
 *
 * As sketched above the core of Hydrogen can be considered its heart,
 * which is pulsating in real-time and takes care of all the heavy
 * lifting. It exposes an API to the GUI using its various classes and
 * has its state altered directly through it when e.g. the user is
 * pressing the play button or adjusting the volume of an instrument.
 *
 * But the core also needs to propagate information back to the
 * GUI. This is done by pushing an \ref H2Core::Event to the \ref
 * H2Core::EventQueue. Whenever the GUI is updated (20 times per
 * second) it checks its \ref EventListener whether there are
 * unprocessed events it has to handle.
 *
 * \section startup Startup
 *
 * For those of you not familiar with Qt or C++ programming in general
 * it might not be obvious at all how a complex application like
 * Hydrogen is starting up when invoked via command line (at least for
 * me it wasn't). Since this knowledge might be essential for the bug
 * you tackle, I try to sketch the overall startup process in this
 * section (in order).
 *
 * 1. The entry point after invocation is the \ref main function in
 * [src/gui/src/main.cpp](https://github.com/hydrogen-music/hydrogen/blob/master/src/gui/src/main.cpp). 
 * 2. It parses the command line arguments and sets the log level.
 * 3. Creates an instance of the \ref H2Core::Logger singleton.
 * 4. \ref H2Core::Object::bootstrap the \ref H2Core::Object and \ref
 *    H2Core::Filesystem class using the logger.
 * 5. Creates an instance of the \ref MidiMap, \ref
 *    H2Core::Preferences, and \ref LashClient.
 * 6. Merges the command line arguments with the preferences and use
 *    the result to customize the appearance, set paths, and load
 *    default content as well as restores the last LASH session if
 *    enabled.
 * 7. Creates an instance of \ref H2Core::Hydrogen.
 *
 *    This will start the core of part of the application (see \ref
 *    generalDesign) and trigger a number of routines before returning
 *    to the context of the \ref main function again.
 *    1. It creates an instance of \ref H2Core::EventQueue, \ref
 *       MidiActionManager, \ref NsmClient, \ref OscServer,
 *       and constructs the \ref H2Core::Hydrogen object.
 *    2. The construction of the \ref H2Core::Hydrogen object itself
 *       triggers the creation of the \ref H2Core::Timeline and \ref
 *       H2Core::CoreActionController.
 *    3. It sets up the audio engine by calling \ref
 *       H2Core::audioEngine_init.
 *    4. This sets the global variables defined in
 *    [src/core/src/hydrogen.cpp](https://github.com/hydrogen-music/hydrogen/blob/master/src/core/src/hydrogen.cpp),
 *       creates the metronome instrument, and an instance of the \ref
 *       H2Core::Effects, \ref H2Core::AudioEngine, and \ref
 *       H2Core::Playlist singleton.
 *    5. The audio engine itself is now initialized.
 *    6. Next, the audio driver is set by \ref
 *       H2Core::audioEngine_startAudioDrivers.
 *
 * 8. Starts the \ref NsmClient using \ref
 *    H2Core::Hydrogen::startNsmClient.
 * 9. Creates the \ref MainForm, which itself will create the instance
 *    of the \ref HydrogenApp singleton and thus initiate the whole
 *    GUI.
 * 10. Displays the \ref MainForm.
 * 11. Loads a drumkit if specified using command line arguments.
 * 12. Now Hydrogen is fully initialized and the Qt object enters an
 *    infinite loop. It will response to user interaction and checks
 *    for events queued by the core 20 times per second.
 */
