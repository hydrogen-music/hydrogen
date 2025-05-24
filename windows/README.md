# Building for Windows

## Compiling on Windows

Using the **PowerShell** script `Build-WinNative.ps1` you can compile Hydrogen
on Windows natively. Note that you need to setup a
[MSYS2](https://www.msys2.org/) environment first.

You have the following options available:
- `build`: compile all sources
- `test`: run all unit tests
- `installdeps`: install all MSYS2 package required for compilation
- `installdeps -qt5`: install all MSYS2 package required for Qt5 compilation
- `deploy`: create a `.exe` from the compiled sources (it will be located within
  the `build/` folder)
- `qt5`: configure the build process to use Qt5 instead of Qt6 (default)

## Cross-compiling on a Unix System

### The easy way:

#### Interactive method

1. Run the script `mxe_installer.sh` located in the windows folder.
2. Run the script `cross_compile.sh` with the `-i` switch located in the windows
   folder.
   
   ```
   ./cross_compile.sh -i
   ```

#### Manual package

Simply run the script with the `-b` switch and pass in the architecture you want
to build, either i686 or x86_64
	
If you want the "fat" package, which bundles with the JACK installer, and the
ladspa plugins, then pass in the `-f` as well.

```
./cross_compile.sh -b x86_64
```


### The manual way:

### Installing all necessary packages

#### On *nix systems (OS X Included)

Visit the page: http://mxe.cc/#requirements and install the pre-requesite packages.

### Cloning Hydrogen from the github repository

If you have not already done it.

```
git clone -b main https://github.com/hydrogen-music/hydrogen.git
```


### Setting the Hydrogen environment variable

Change to your Hydrogen directory.

``` bash
mv hydrogen source
cd source
export HYDROGEN=$PWD
cd ..
```


### Cloning MXE from the github repository

Clone the master branch, since it should be more up-to-date than the stable branch.

```
git clone -b master https://github.com/mxe/mxe.git
```


### Configuring MXE

``` bash
cd mxe
export MXE=$PWD
```

Edit `Makefile` and set the value of `MXE_TARGETS` as follows ().

For 32-bit Windows target: `MXE_TARGETS        := i686-w64-mingw32.shared`
For 64-bit Windows target: `MXE_TARGETS        := x86_64-w64-mingw32.shared`
For both 32 and 64 bit Windows target: `MXE_TARGETS        := i686-w64-mingw32.shared x86_64-w64-mingw32.shared`

### Configuring and cross-compiling packages

Most `make` operations below take a considerable amount of time. The lengthy
ones have been timed, in order to give you a rough estimate. If you're compiling
both 32 and 64 bit versions then you will need to roughly double all the times
listed below. Adjust your completion expectations according to the values
presented and your system capabilities. All operations in this section should be
executed in the MXE root directory, where the `Makefile` resides. Packages along
with their dependencies are downloaded and cross-compiled automatically as
needed.

#### Cross-compiling gcc

```
make gcc
```

> real    13m10.244s
> user    30m51.456s
> sys     4m17.841s

#### Cross-compiling winpthreads

```
make winpthreads
```

#### Configuring gcc and cross-compile again

There is a cyclical dependency problem here that requires gcc to be built once
normally, and then once with winpthreads. This is because winpthreads requires
gcc to build, but we need gcc built with winpthreads support (which it doesn't
have the first time you make it).

Edit `src/gcc.mk` and set the value of `$(PKG)_DEPS` as follows.

```
$(PKG)_DEPS     := binutils gcc-gmp gcc-isl gcc-mpc gcc-mpfr winpthreads
```

Also set the value of `--enable-threads` as follows.

```
--enable-threads=posix
```

Then cross-compile `gcc` again.

```
make gcc
```

> real    8m32.152s
> user    24m34.296s
> sys     2m47.471s

#### Cross-compiling other packages

```
make qt libarchive libsndfile portaudio portmidi fftw rubberband jack
```

> real    70m17.737s
> user    199m0.451s
> sys     15m14.591s

Due to a bug in JACK, you will need to edit the portaudio.mk file and set jack
as a dependency, then rebuild PortAudio
    
```
sed -i 's/:= gcc/:= gcc jack/g' $MXE/src/portaudio.mk
make portaudio
```

### Cross-compiling Hydrogen

You will want to change the `CMAKE_TOOLCHAIN_FILE` to reflect which version of
hydrogen you want (i686 = 32bit, x86_64 = 64bit).

``` bash
cd $HYDROGEN
```

    
For 32 bit compilations use
``` bash
cmake -DCMAKE_TOOLCHAIN_FILE=$MXE/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake
```

For 64 bit compilations use
``` bash
cmake -DWIN64:BOOL=ON ../ -DCMAKE_TOOLCHAIN_FILE=$MXE/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake -DCMAKE_{C,CXX}_FLAGS=-m64
```
    
``` bash
cpack -G NSIS
```

> real    4m32.063s
> user    4m14.671s
> sys     0m17.105s

`cpack` will create your installer for you, which you can then copy to a windows
machine, and install.
