# Building for Windows

## Cross-compiling on a Unix System

### Installing all necessary packages

#### On Debian/Ubuntu

    $ sudo apt-get install autoconf automake autopoint bash bison bzip2 \
                           cmake flex gettext git gcc g++ intltool \
                           libffi-dev libtool libltdl-dev libssl-dev \
                           libxml-parser-perl make openssl patch perl \
                           pkg-config scons sed unzip wget xz-utils

#### On 64-bit Debian/Ubuntu (additional packages)

    $ sudo apt-get install g++-multilib libc6-dev-i386

#### On other Unix systems

http://mxe.cc/#requirements

### Cloning MXE from the github repository

Clone the master branch, since it should be more up-to-date than the stable branch.

    $ git clone -b master https://github.com/mxe/mxe.git

### Configuring MXE

    $ cd mxe

Edit *Makefile* and set the value of *MXE_TARGETS* as follows.

    MXE_TARGETS        := i686-w64-mingw32.shared

### Configuring and cross-compiling packages

All *make* operations below take time. They should be executed in the MXE root directory, where the *Makefile* resides. Packages along with their dependencies are downloaded and cross-compiled.

#### Cross-compiling gcc

    $ make gcc

#### Cross-compiling winpthreads

    $ make winpthreads

#### Configuring gcc and cross-compiling again

Edit *src/gcc.mk* and set the value of *$(PKG)_DEPS* as follows.

    $(PKG)_DEPS     := binutils gcc-cloog gcc-gmp gcc-isl gcc-mpc gcc-mpfr winpthreads

Also set the value of *--enable-threads* as follows.

    --enable-threads=posix

    $ make gcc

#### Cross-compiling other packages

    $ make qt libarchive libsndfile

### Cross-compiling Hydrogen

    $ cd /path/to/hydrogen

    $ mkdir build

    $ cd build

    $ cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/mxe/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake

    $ make
