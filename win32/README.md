# Building for Windows

## Cross-compiling on a Unix System

### Installing all necessary packages

#### On Debian/Ubuntu

    $ sudo apt-get install autoconf automake autopoint bash bison bzip2 \
                           cmake flex gettext git gcc g++ intltool \
                           libffi-dev libtool libtool-bin libltdl-dev \
                           libssl-dev libxml-parser-perl make openssl \
                           patch perl pkg-config scons sed unzip wget \
			   xz-utils

#### On 64-bit Debian/Ubuntu (additional packages)

    $ sudo apt-get install g++-multilib libc6-dev-i386

#### On other Unix systems

http://mxe.cc/#requirements

### Cloning Hydrogen from the github repository

If you have not already done it.

    $ git clone -b master https://github.com/hydrogen-music/hydrogen.git

### Setting the Hydrogen environment variable

Change to your Hydrogen directory.

    $ cd hydrogen
    
    $ export HYDROGEN=$PWD
    
    $ cd ..

### Cloning MXE from the github repository

Clone the master branch, since it should be more up-to-date than the stable branch.

    $ git clone -b master https://github.com/mxe/mxe.git

### Configuring MXE

    $ cd mxe

    $ export MXE=$PWD

Edit *Makefile* and set the value of *MXE_TARGETS* as follows (for 32-bit Windows target).

    MXE_TARGETS        := i686-w64-mingw32.shared

### Configuring and cross-compiling packages

Most *make* operations below take a considerable amount of time. The lengthy ones have been timed, in order to give you a rough estimate. Adjust your completion expectations according to the values presented and your system capabilities. All operations in this section should be executed in the MXE root directory, where the *Makefile* resides. Packages along with their dependencies are downloaded and cross-compiled automatically as needed.

#### Cross-compiling gcc

    $ make gcc

    real    13m10.244s
    user    30m51.456s
    sys     4m17.841s

#### Cross-compiling winpthreads

    $ make winpthreads

#### Configuring gcc and cross-compiling again

Edit *src/gcc.mk* and set the value of *$(PKG)_DEPS* as follows.

    $(PKG)_DEPS     := binutils gcc-gmp gcc-isl gcc-mpc gcc-mpfr winpthreads

Also set the value of *--enable-threads* as follows.

    --enable-threads=posix

Then cross-compile gcc again.

    $ make gcc

    real    8m32.152s
    user    24m34.296s
    sys     2m47.471s

#### Cross-compiling other packages

    $ make qt libarchive libsndfile

    real    70m17.737s
    user    199m0.451s
    sys     15m14.591s

### Cross-compiling Hydrogen

    $ cd $HYDROGEN
    
    $ cd win32
    
    $ mkdir windows_32_bit_build
    
    $ cd windows_32_bit_build
    
    $ export HYDROGEN_BUILD=$PWD
    
    $ cmake ../.. -DCMAKE_TOOLCHAIN_FILE=$MXE/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake

    Edit *$HYDROGEN/src/core/include/hydrogen/hydrogen.h* and add the line *#include <hydrogen/timehelper.h>* at the end of the includes
    
    $ make

    real    4m32.063s
    user    4m14.671s
    sys     0m17.105s

    $ cd ..

### Creating the bundle

    $ ./create_bundle.sh

This creates a directory *hydrogen_windows_32_bit*, which you can copy to your Windows machine and launch Hydrogen!

### Troubleshooting

When copying the directory above to your Windows machine, you will get a copy conflict. This is because Windows' case-insensitivity is put to the test, since two files named *Director.png* and *director.png* exist in the same directory. Resolve the conflict in whichever way you wish, since the aforementioned files seem to be unused.

Using the method described above, I experience Hydrogen crashing at startup. To fix this, download the precompiled libsndfile-1.dll from *http://www.mega-nerd.com/libsndfile/#Download* and put it in the Hydrogen directory.

### Bugs?

Bugs.