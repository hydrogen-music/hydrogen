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
 * \section introductionAE Introduction to the audio engine
 *
 * Under construction - I still haven't figured out this whole
 * behemoth.
 *
 * But as a very superficial sketch:
 */
