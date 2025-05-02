# Building and installing Hydrogen

---

Content:

1. [Build and Install from Source](#build-and-install-from-source)
2. [Build an AppImage](#build-an-appimage)
3. [Generate the Documentation](#generate-the-documentation)

---

This guide is Linux specific. If you intend to build it on another platform, please check out the following resources:

- **[Building Hydrogen on Mac OS X](https://github.com/hydrogen-music/hydrogen/wiki/Building-Hydrogen-from-Source-(macOS))**
- **[Building Hydrogen on Windows](https://github.com/hydrogen-music/hydrogen/wiki/Building-Hydrogen-from-source-(Windows))**

## Build and Install from Source

### Prerequisites

The source code for the current development version can be checked out
via git:

``` bash
$ git clone git://github.com/hydrogen-music/hydrogen.git
```

In order to build Hydrogen from source, you will need the following
libraries and development header files installed on your system:

#### Required

- Qt 5 Library
- Qt 5 SDK (moc, uic, etc.)
- GNU g++ compiler (>=4.0, 3.x might work)
- cmake (>=2.6)
- libsndfile >=1.0.18
- zlib and libtar *OR* libarchive
- At least one of the following audio and midi driver

#### Audio and Midi Drivers

- JACK Audio Connection Kit (>=0.103.0)
- ALSA (Advanced Linux Sound Architecture)
- OSS
- PortAudio (v18, not v19)
- PortMIDI (>=2.0.1)

#### Optional Support

- liblo for OSC (Open Sound Control)
- NSM (Non Session Manager)
- liblrdf for LADSPA plugins
- librubberband2 (Rubberband support is experimental)

Currently it is recommended that you disable the rubberband config
option (done by default) to ensure backwards compatibility with songs
created under 0.9.5 which use rubberband. Install the `rubberband-cli`
package beside `librubberband2`. Rubberband works properly even if
this option is disabled. If available, Hydrogen locates the installed
`rubberband-cli` binary.

#### Packages Required on Debian-based Systems

In order to build Hydrogen on Debian-based Systems, you can use the
following command to install all basic and some optional requirements.

``` bash
$ sudo apt-get install cmake qtbase5-dev qtbase5-dev-tools  \
	qttools5-dev qttools5-dev-tools libqt5xmlpatterns5-dev  \
	libqt5svg5-dev libarchive-dev libsndfile1-dev libasound2-dev  \
	liblo-dev libpulse-dev libcppunit-dev liblrdf0-dev  \
	librubberband-dev
```

In addition, either the `libjack-jackd2-dev` or `libjack-dev` package
must be present to enable the support of the **JACK** audio
driver. [Which one to
pick](https://github.com/jackaudio/jackaudio.github.com/wiki/Q_difference_jack1_jack2)
depends on whether JACK2 or JACK1 is installed on your system. If none
is present, either package will work.

### Building and Installing Hydrogen

After you have installed all the prerequisites, building and
installing will look like this:

``` bash
$ git clone git://github.com/hydrogen-music/hydrogen.git
$ cd hydrogen
$ mkdir build && cd build
$ cmake ..
$ make && sudo make install
```

### Running Hydrogen

Some common problems encountered by user building from source:

##### Command not found / wrong version

In case you get the error

> ... command not found: hydrogen

or the wrong version of Hydrogen is starting up, your shell is not able to find
the one you just installed (you can check using `which hydrogen`).

After installation, Hydrogen's binaries can be found in `/usr/local/bin`.
If this path is not in your `PATH` environment variable (you can check using
`echo $PATH`), you should add

```bash
export PATH=/usr/local/bin:$PATH
```

to your `$HOME/.profile` file (or .bashrc, .zshrc, or .xprofile in case you
prefer those) and run `source ~/.profile`.

##### Error loading shared libraries

If Hydrogen doesn't start, and you get a message similar to:

> … error while loading shared libraries: libhydrogen-core-1.X.X.so …

your OS could not find the shared library you just compiled and installed along
with the program itself.

Seems like your Linux distribution is more user than developer focused. This is
not a big problem. It just added `/usr/local/bin` to `$PATH` but not `/usr/local/lib`
to `$LD_LIBRARY_PATH`.

Installing your local build to `/usr/local` is more or less standard. `/usr` should be
used by your package manager. This allows you to manually compile and install custom
Hydrogen versions without messing with the system's one. We just have to tell the system
where to find all required pieces.

You could use the `/usr` installation prefix when building Hydrogen. But I would strongly
discourage that. Instead, you could do one of these:

- run `sudo ldconfig` and check whether this already fixes your issue
- open Hydrogen via the command line using `LD_LIBRARY_PATH=/usr/local/lib hydrogen`
- add `export LD_LIBRARY_PATH=/usr/local/lib:${LD_LIBRARY_PATH}` to your `$HOME/.profile`
- set a symlink as you mentioned from `/usr/local/lib/libhydrogen-core.1.X.X.so` to
  `/usr/lib/`

see [issue#677](https://github.com/hydrogen-music/hydrogen/issues/677) for further insights.

### Build Script

Alternatively you could use our custom build script
[./build.sh](https://github.com/hydrogen-music/hydrogen/blob/main/build.sh). This
is the recommended way if you are actively developing new
features or bug fixes for Hydrogen (all characters in squared brackets
are optional and do not have to be included in the command).

| Command          | Functionality                                                     |
|------------------|-------------------------------------------------------------------|
| `r[m]`           | Remove all build, temporary, and cached files.                    |
| `c[lean]`        | Remove all cached files.                                          |
| `m[ake]`         | Launch the build process.                                         |
| `mm`             | Launch the build process using `ccache`.                          |
| `d[oc]`          | Build the documentation of Hydrogen.                              |
| `g[raph]`        | Draw the dependency graphs of the Hydrogen code using `graphviz`. |
| `h[elp]`         | Show all supported build options.                                 |
| `[e]x[ec]`       | Execute the Hydrogen binary.                                      |
| `t[est]`         | Run the unit tests.                                               |
| `p[kg]`          | Build a source package.                                           |
| `i[integration]` | Execute integration tests.                                        |
| `appimage`       | Build an AppImage.                                                |
| `z`              | Build Hydrogen using `ccache` and execute the resulting binary.   |

Using `ccache` to build Hydrogen, `./build.sh mm`, will result in a
compilation, which takes a little longer than the one with the usual
`make` command. But in all further runs, only the recently-modified
components will be recompiled. This can marginally speed up development.

In addition, `mm` and `mt` also create a `compile_commands.json` file within the
_build_ folder required for `clangd` to work properly as [LSP (language
server)](https://clangd.llvm.org/installation). Opening your IDE spawning
`clangd` _after_ building Hydrogen for the first time should give you modern C++
code editting features.

Note: if you want to run the integration tests, you need to have both `JACK`
server and `Go` installed.

### Additional Build Features and Uninstall

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

## Build an AppImage

For creating an _AppImage_ you have to install all required packages as
described in the previous step. Then you can simply run the dedicated
build script command

``` bash
$ ./build.sh appimage
```

This will download the latest toolchain required for AppImage
packaging (in case you haven't installed it yet) and creates a fresh
image in the _build-appimage_ folder.

To run it, ensure it's executable and call it from the console or open
it using double click.

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
