#!/bin/sh
# Hydrogen Cross Compile Script

# These variables hold the counters.
ERR_MSG=""

# clear the screen
clear

while :
	do
	# If error exists, display it
	if [ "$ERR_MSG" != "" ]; then
		echo "Error: $ERR_MSG"
		echo ""
	fi

	# Write out the menu options...
	echo "Welcome to the Hydrogen Cross Compiler. We will now compile Hydrogen for Windows."
	echo "Select an option:"
	echo " 1: Prepare the system"
	echo " 2: Clone required repositories"
	echo " 3: Build Hydrogen"
	echo " 4: Clean up Files"
	echo " 5: Exit"

	# Clear the error message
	ERR_MSG=""

	# Read the user input
	read SEL

	case $SEL in
		1)	#Prepares the system by getting the necessary packages to perform the cross compile
			echo "Now installing required packages"
			sudo apt-get install autoconf automake autopoint bash bison bzip2 cmake flex gettext git gcc g++ intltool libffi-dev libtool libtool-bin libltdl-dev libssl-dev libxml-parser-perl make openssl patch perl pkg-config scons sed unzip wget xz-utils
			if (uname -a | grep x86_64)
				sudo apt-get install g++-multilib libc6-dev-i386
			fi
			;;
		2)	#download the proper git repositories
			mkdir ~/build/hydrogen_win32
			cd ~/build/hydrogen_win32
			export BUILD_DIR=$PWD
			#git clone -b master https://github.com/hydrogen-music/hydrogen.git
			git clone -b master https://github.com/mikotoiii/hydrogen.git
			git clone -b master https://github.com/mxe/mxe.git
			git clone git://github.com/jackaudio/jack2.git
			;;
		3)	#Set the required variables
			echo "Now starting the building of Hydrogen for Windows. This will take quite a while and requires no interaction."
			
			export HYDROGEN=~/build/hydrogen_win32/hydrogen
			export MXE=~/build/hydrogen_win32/mxe
			#Modify the MXE Makefiles to allow for the cross compilation.
			sed -i 's/i686-w64-mingw32.static/i686-w64-mingw32.shared/g' $MXE/Makefile
			cd $MXE
			#Make gcc and winpthreads. gcc will need to be rebuilt once winpthreads is built.
			#Note: This needs to happen because winpthreads needs gcc to be built, but we need gcc built with winpthreads support to build hydrogen.
			#	this creates a cyclical dependancy problem that there seems to be no way around.
			make gcc
			make winpthreads
			sed -i 's/binutils gcc-gmp gcc-isl gcc-mpc gcc-mpfr mingw-w64/binutils gcc-gmp gcc-isl gcc-mpc gcc-mpfr winpthreads/g' $MXE/src/gcc.mk
			sed -i 's/--enable-threads=win32/--enable-threads=posix/g' $MXE/src/gcc.mk
			make gcc
			#Build the dependancies for hydrogen
			make qt libarchive libsndfile portaudio portmidi fftw rubberband -j7 JOBS=7

			#Build hydrogen itself now.
			mkdir $HYDROGEN/win32/windows_32_bit_build
			export HYDROGEN_BUILD=$HYDROGEN/win32/windows_32_bit_build
			cd $HYDROGEN_BUILD
			cmake ../.. -DCMAKE_TOOLCHAIN_FILE=$MXE/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake
			make
			sh ../create_bundle.sh
			;;
		4)	#Clean up the files
			echo "Now cleaning up the files. This process will move the built hydrogen into your home directory and delete the build files."
			mv $HYDROGEN_BUILD/windows_32_bit_build ~/hydrogen_windows_32_bit_build
			rm -rf $HYDROGEN
			rm -rf $MXE
			;;
		5)	echo "Thank you for using the Hydrogen Cross Compiler. Goodbye."
			exit
			;;
		*) ERR_MSG="Please enter a valid option"
	esac

	# clear the screen again for re-display
	clear
done