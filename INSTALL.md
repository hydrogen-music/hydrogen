------------------------------------------------------------------------------
                       H Y D R O G E N          Drum machine
------------------------------------------------------------------------------

BUILDING AND INSTALLING HYDROGEN
================================

Contents:

1. System Requirements
2. Download
3. Binary Packages
4. Prerequisites to Build from Source
5. Build and Install from Source

1. System Requirements
----------------------

Hydrogen is supported on the following operating systems:

  * Linux/Unix/BSD
  * Mac OS X

Hydrogen is *not* supported on Windows because there are currently no
Windows developers on the project... and so the Windows components
have fallen into disrepair.

Hydrogen is known to work well on fairly old systems like this (as of
2009):

  Pentium III 500 MHz
  198 MB RAM
  Consumer-Grade (cheap) audio card
  Keyboard and Mouse
  1x1 USB MIDI Interface (optional)

2. Download
-----------

Hydrogen can be downloaded as a binary package, source distribution,
or you can check out the current development version.
These can be accessed on the Hydrogen home page:

	http://www.hydrogen-music.org/

The source code for the current development version can be checked out
via git:

	$ git clone git://github.com/hydrogen-music/hydrogen.git

3. Binary Packages
------------------

Debian (GNU/Linux) and Ubuntu (GNU/Linux):

	Hydrogen can usually be installed with apt:

	# apt-get install hydrogen

	However, if you wish to have a more current version of
	Hydrogen, the Hydrogen dev's typically maintain a .deb package
	for Debian stable, testing, and some Ubuntu distributions.
	Note that apt takes care of any library dependencies that you
	have.

Other GNU/Linux:

	Check your package management system for the package
	'hydrogen.'

Mac OS X:

	They Hydrogen home page has a binary package available for OS
	X.  Extract the ZIP archive and it will create a hydrogen.app
	folder.

4. Prerequisites to Build from Source
-------------------------------------

In order to build from source, you will need the following libraries
installed on your system, and the development header files:

	REQUIRED
	* Qt 5 Library
	* Qt 5 SDK (moc, uic, etc.)
	* GNU g++ compiler (>=4.0, 3.x might work)
        * cmake (>=2.6)
        * libsndfile >=1.0.18
	* zlib and libtar -OR- libarchive
	  OS X: You will probably need to build libarchive from source.
	* At least 1 audio and 1 midi driver
	* OS X: Xcode

	DRIVERS AVAILABLE
	* JACK Audio Connection Kit (>=0.103.0)
	* ALSA (Advanced Linux Sound Architecture)
	* OSS
	* PortAudio (v18, not v19)
	* PortMIDI
	* CoreAudio (OS X)
	* CoreMidi (OS X)

	OPTIONAL SUPPORT
    	* LASH (Linux Audio Session Handler)
        * JACK Session need Jack Audio Connection Kit(>=0.120.0/1.9.7) 
	* liblrdf for LADSPA plugins
	* librubberband2 for lib - RUBBERBAND support (experimental)
	  currently it is recommended that you disable this config option
	  to ensure backwards compatibility with songs created under 0.9.5
	  which use rubberband. install the rubberband -cli package beside
	  librubberband2 . rubberband works properly even if this option
	  is disabled. if available, hydrogen locate an installed
	  rubberband-cli binary.

	REQUIRED PACKAGES IN DEBIAN-BASED SYSTEMS
	qtbase5-dev qtbase5-dev-tools qttools5-dev qttools5-dev-tools libqt5xmlpatterns5-dev
	libarchive-dev libsndfile1-dev libasound2-dev liblo-dev libpulse-dev
	libcppunit-dev liblrdf-dev liblash-compat-dev
	librubberband-dev

	In addition, either the libjack-jackd2-dev or libjack-dev
	package must be present. Which one to pick depends on whether
	JACK2 or JACK1 is installed on your system. If none is
	present, you are free to choose one of them. 

	ON OS X USING BREW
	libsndfile jack pulseaudio cppunit libarchive qt5

On a single, 500MHz processor Hydrogen takes about 1.5 hours to build.

5. Build and Install from Source
--------------------------------

If you wish to build a package for your operating system, skip down to
the end of this section.

This instructions are thought for building hydrogen on linux.
We're providing a set of qmake project files for windows and
mac osx builds. 
Please read the relevant wiki pages for more informations:

https://github.com/hydrogen-music/hydrogen/wiki/Building-Hydrogen-from-source-(LINUX)
https://github.com/hydrogen-music/hydrogen/wiki/Building-Hydrogen-from-source-(MAC-OSX)
https://github.com/hydrogen-music/hydrogen/wiki/Packaging-for-Windows

 5.1 build
 ---------

After you have all the prerequisites, building and installing will
look like this:

	$ tar xjf hydrogen-0.9.6.tar.bz2
	$ cd hydrogen-0.9.6
	$ mkdir build && cd build
	$ cmake ..
	$ make && sudo make install

All the following cmake commands should be executed in a build directory :

If you wish to configure features like LADSPA plugins,
or debugging symbols, get more information like this:

	$ cmake -L ..

For possible make targets:

    $ make help

To change the directory where hydrogen is installed, it is done like
this:

	$ cmake -DCMAKE_INSTALL_PREFIX=/opt/hydrogen ..
	$ make && sudo make install

Uninstalling Hydrogen is done like this:

	$ sudo cmake uninstall

Note that cmake is a build system and not a package manager.  While we
make every effort to ensure that Hydrogen uninstalls cleanly, it is
not a guarantee.

Cmake macros should detect the correct Qt settings and location of your libraries,
but sometimes it needs a little help.  If Hydrogen fails to build, some
environment variables could help it.

	$ QTDIR=/opt/lib/qt4 OSS_PATH="/usr/lib/oss/lib" OSS_INCLUDE="/usr/lib/oss/include" cmake ..

 5.2 cmake helper
 ----------------

 Alternatively you could use the cmake helper : ./build.sh

 To remove and build hydrogen from scratch

    $ ./build.sh r m

___CREATING A BINARY PACKAGE___

If you are a package maintainer and wish for your packaging scripts to
be included in the Hydrogen source tree, we would be happy to work
with you.  Please contact the developer mailing list (see the Hydrogen
home page).  The instructions below are for the package systems that
have been contributed so far.

___CREATING A .DEB PACKAGE___

In order to create a .deb package for Debian or Debian-based systems
(like Ubuntu), you first need the debhelper package:

	# apt-get install debhelper

To build the Hydrogen package:

    $ tar xjf hydrogen-0.9.6.tar.bz2
    $ cd hydrogen-0.9.6
	$ cd linux
	$ fakeroot dpkg-buildpackage

This will place the .deb package and description files in the parent
directory.  If you wish to change the version number for the archive,
edit linux/debian/changelog to set the version. To install the newly
created deb package run the following (substitute the deb package
name with the version your build created):

	$ cd ..
        $ sudo dpkg -i hydrogen_0.9.6_amd64.deb

