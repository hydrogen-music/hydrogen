------------------------------------------------------------------------------
                       H Y D R O G E N          Drum machine
------------------------------------------------------------------------------

BUILDING AND INSTALLING HYDROGEN
================================

Content:

1. [System Requirements](#system-requirements)
2. [Binary Packages](#binary-packages)
3. [Build and Install from Source](#build-and-install-from-source)
4. [Generate the Documentation](#generate-the-documentation)


## System Requirements

Hydrogen is supported on the following operating systems:

  * Linux/Unix/BSD
  * Mac OS X
  * Windows 10 (maybe 7)

## Binary Packages

### Mac OS X and Windows

Hydrogen can be downloaded as a binary package, source distribution,
or you can check out the current development version.

Binary packages can be accessed on the project's Github page:

- [https://github.com/hydrogen-music/hydrogen/releases](https://github.com/hydrogen-music/hydrogen/releases)

If you are interested in older versions or installers, you can check
out the Hydrogen home page:

- [http://www.hydrogen-music.org/](http://www.hydrogen-music.org/)

To get the latest version with the latest features on **Mac OS X**,
you can install Hydrogen using **Mac Ports**.

- [Installing on Mac OS X with MacPorts](https://github.com/hydrogen-music/hydrogen/wiki/Building-Hydrogen-from-source-(MAC-OSX)#method-1-building-everything-with-macports
)

### Linux

Please check your package management system for the package called
_hydrogen_. In **Debian (GNU/Linux)**-based distributions this can be
done with `apt`

``` bash
$ sudo apt-get install hydrogen
```

## Build and Install from Source

### Mac OS X

If you intend to build Hydrogen from source on Mac OS X, please read
the relevant wiki pages for more information:

- [Building Hydrogen from source on Mac OS X](https://github.com/hydrogen-music/hydrogen/wiki/Building-Hydrogen-from-Source-(macOS))
  
### Windows

For building Hydrogen on Windows, please check out the following wiki
page:

- [Packaging Hydrogen for Windows](https://github.com/hydrogen-music/hydrogen/wiki/Packaging-for-Windows)

### Linux

#### Prerequisites

The source code for the current development version can be checked out
via git:

``` bash
$ git clone git://github.com/hydrogen-music/hydrogen.git
```

In order to build Hydrogen from source, you will need the following
libraries and development header files installed on your system:

##### Required

- Qt 5 Library
- Qt 5 SDK (moc, uic, etc.)
- GNU g++ compiler (>=4.0, 3.x might work)
- cmake (>=2.6)
- libsndfile >=1.0.18
- zlib and libtar *OR* libarchive
- OS X: Xcode
- At least one of the following audio and midi driver

##### Audio and Midi Drivers

- JACK Audio Connection Kit (>=0.103.0)
- ALSA (Advanced Linux Sound Architecture)
- OSS
- PortAudio (v18, not v19)
- PortMIDI
- CoreAudio (OS X)
- CoreMidi (OS X)

##### Optional Support

- liblo for OSC (Open Sound Control)
- LASH (Linux Audio Session Handler)
- NSM (Non Session Manager)
- liblrdf for LADSPA plugins
- librubberband2 (Rubberband support is experimental)

Currently it is recommended that you disable the rubberband config
option (done by default) to ensure backwards compatibility with songs
created under 0.9.5 which use rubberband. Install the `rubberband-cli`
package beside `librubberband2`. Rubberband works properly even if
this option is disabled. If available, Hydrogen locates the installed
`rubberband-cli` binary.

##### Packages Required on Debian-based Systems

In order to build Hydrogen on Debian-based Systems, you can use the
following command to install all basic and some optional requirements.

``` bash
$ sudo apt-get install cmake qtbase5-dev qtbase5-dev-tools  \
	qttools5-dev qttools5-dev-tools libqt5xmlpatterns5-dev  \
	libqt5svg5-dev libarchive-dev libsndfile1-dev libasound2-dev  \
	liblo-dev libpulse-dev libcppunit-dev liblrdf-dev  \
	liblash-compat-dev librubberband-dev
```

In addition, either the `libjack-jackd2-dev` or `libjack-dev` package
must be present to enable the support of the **JACK** audio
driver. [Which one to
pick](https://github.com/jackaudio/jackaudio.github.com/wiki/Q_difference_jack1_jack2)
depends on whether JACK2 or JACK1 is installed on your system. If none
is present, either package will work.

#### Building and Installing Hydrogen

After you have installed all the prerequisites, building and
installing will look like this:

``` bash
$ git clone git://github.com/hydrogen-music/hydrogen.git
$ cd hydrogen
$ mkdir build && cd build
$ cmake ..
$ make && sudo make install
```

#### Running Hydrogen

After installation, Hydrogen's binaries can be found in `CMAKE_INSTALL_PREFIX/bin`.
If this path is not in your `PATH` environment variable, consider adding it to it.

If Hydrogen doesn't start, and you have the above message :

```
… error while loading shared libraries: libhydrogen-core-1.1.0.so …
```

it's because Hydrogen's shared library is not found.
You can verify this with the following command

```bash
ldd CMAKE_INSTALL_PREFIX/bin/hydrogen | grep 'not found'
```

To fix it, you can use `LD_PRELOAD` or `LD_LIBRARY_PATH` environment variables,
or configure `ldconfig` (see man ldconfig, man ld.so).

Another option is to set the `cmake` option
`-DCMAKE_INSTALL_PREFIX=/usr`, recompile, and reinstall Hydrogen.
But be aware that you will certainly overwrite Hydrogen files that you might have
installed with your distribution's package manager.

see [issue#677](https://github.com/hydrogen-music/hydrogen/issues/677)

#### Build Script

Alternatively you could use our custom build script
[./build.sh](https://github.com/hydrogen-music/hydrogen/blob/master/build.sh). This
is the recommended way if you are actively developing new
features or bug fixes for Hydrogen (all characters in squared brackets
are optional and do not have to be included in the command).

| Command    | Functionality                                                     |
| ---        | ---                                                               |
| `r[m]`     | Remove all build, temporary, and cached files.                    |
| `c[lean]`  | Remove all cached files.                                          |
| `m[ake]`   | Launch the build process.                                         |
| `mm`       | Launch the build process using `ccache`.                          |
| `d[oc]`    | Build the documentation of Hydrogen.                              |
| `g[raph]`  | Draw the dependency graphs of the Hydrogen code using `graphviz`. |
| `h[elp]`   | Show all supported build options.                                 |
| `[e]x[ec]` | Execute the Hydrogen binary.                                      |
| `t[est]`   | Run the unit tests.                                               |
| `p[kg]`    | Build a source package.                                           |
| `z`        | Build Hydrogen using `ccache` and execute the resulting binary.   |

Using `ccache` to build Hydrogen, `./build.sh mm`, will result in a
compilation, which takes a little longer than the one with the usual
`make` command. But in all further runs, only the recently-modified
components will be recompiled. This can marginally speed up development.


#### Additional Build Features and Uninstall

All the following `cmake` commands should be executed in a build
directory :

If you wish to configure features like **LADSPA plugins**,
or **debugging symbols**, get more information like this:

``` bash
$ cmake -L ..
```

For possible **make targets**:

``` bash
$ make help
```

To change the directory Hydrogen will be installed in, you have to
provide the `-DCMAKE_INSTALL_PREFIX` option during the configuration
of your custom build (the default path is */usr/local/*).

``` bash
$ cmake -DCMAKE_INSTALL_PREFIX=/opt/hydrogen ..
$ make && sudo make install
```

**Uninstalling** Hydrogen is done like this:

``` bash
$ sudo cmake uninstall
```

Note that `cmake` is a build system and not a package manager.  While
we make every effort to ensure that Hydrogen uninstalls cleanly, it is
not guaranteed.

`cmake` macros should detect the correct Qt settings and location of
your libraries, but sometimes it needs a little help.  If Hydrogen
fails to build, some environment variables could help it.

``` bash
$ QTDIR=/opt/lib/qt4 OSS_PATH="/usr/lib/oss/lib" OSS_INCLUDE="/usr/lib/oss/include" cmake ..
```

## Generate the Documentation

Apart from the [official manuals and
tutorial](http://hydrogen-music.org/doc/), Hydrogen does also feature
an extended documentation of its code base.

After installing the requite `Doxygen` package

```bash
$ sudo apt-get install doxygen
```

run the following command

``` bash
$ ./build.sh d
```

It will produce two folders, *build/docs/html/* and *build/docs/latex*,
containing the documentation as HTML and LaTeX, respectively. The HTML
version is recommended since it provides a more friendly way to navigate
through the set of created files. You can view them using your
favorite browser, e.g.

``` bash
$ firefox build/docs/html/index.html
```
