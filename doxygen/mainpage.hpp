/**
 * \mainpage Overview
 *
 * \tableofcontents
 *
 *  Hydrogen is an advanced drum machine for GNU/Linux, Mac and
 *  Windows. Its main goal is to provide professional, yet simple and
 *  intuitive, pattern-based drum programming. Unlike other drum
 *  machines the C++ code base of Hydrogen and its slim Qt5-based GUI
 *  are tailored to be both memory-efficient and quite gentle in terms
 *  of CPU load. This makes it an excellent choice for older hardware
 *  or big setups consisting of numerous real-time audio programs.
 *
 * \section structureCode Structure of the code
 * 
 * All C++ code of Hydrogen can be found in the
 * [src](https://github.com/hydrogen-music/hydrogen/tree/main/src)
 * folder. The particular structure of this directory is meant to
 * imitate the overall design of the application. 
 *
 * For a more thorough introduction please see the [general
 * design](https://github.com/hydrogen-music/hydrogen/wiki/General-code-design)
 * on Github.
 *
 * \subsection structureCore Core
 *
 * Hydrogen consists of a \b core, which is responsible for, among
 * others, generating all audio, holding representations of
 * all patterns, drumkits etc., for all interactions with the
 * operation system and its audio driver, and for interpreting and
 * sending MIDI signals.
 *
 * \subsection structureGUI GUI
 * The \b GUI, which was written in Qt5, is designed to be a completely
 * separate entity. It is able to query and set various resources of
 * the core and serves as the main method for the user to interact
 * with the application (core).
 *
 * \section structureDocumentation Structure of the documentation
 *
 * Both the documentation of the code base and the overall one you are
 * reading right now are still under active development. If you have
 * some insights into Hydrogen you want to share, we would be very
 * happy about your contribution.
 *
 * \subsection structureModules Modules
 *
 * Since this project grew naturally (and quite big), the
 * encapsulation of related classes/functions into namespaces might
 * not be as good as it could be. To still provide an overview about
 * separate topics, the documentation of the classes was grouped into
 * logical units you can access in [Modules](modules.html). \ref
 * docCore contains all classes and functions of the core while \ref
 * docGUI encapsulates everything related to the Qt5-based GUI.
 *
 * \subsection structureNM Namespace Members
 * Lists all members of the \b H2Core namespace encapsulating
 * most classes and functions, which can be found in the *src/core/* 
 * folder (see \ref structureCode). Since Hydrogen does only contain
 * this one namespace, this page might not be the most informative
 * one. \ref structureDS might be a better alternative.
 *
 * \subsection structureDS Data Structures
 * This page gives insights into all (documented) data structures of
 * Hydrogen.
 *
 * \subsection structureFiles Files
 * The Files page provides you with lists all files touched by Doxygen
 * while creating this documentation. It reproduces the folder
 * hierarchy of the code (see \ref structureCore) and allows you to
 * view both the documentation and source code of each individual file.
 *
 * \section questions Questions?
 *
 * Get in touch with us via our [mailing
 * list](http://lists.sourceforge.net/mailman/listinfo/hydrogen-devel)
 * or by filing an issue on our
 * [GitHub](https://github.com/hydrogen-music/hydrogen) page.
 *
 * \section contribute Want to contribute?
 *
 * Hydrogen is a community-driven project and critically depends on
 * the commitment of volunteers. So, even if you are not an
 * experienced C++ developer (yet), there are countless ways to
 * contribute to this project, like improving the documentation and
 * manual, translating the user manual, tweaking the design, making a
 * donation
 * etc. We are happy about all your efforts to contributed. The more
 * people we are, the more awesome features we can provide. ;)
 */
