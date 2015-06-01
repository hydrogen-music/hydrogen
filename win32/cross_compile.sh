#!/bin/bash
# Hydrogen Cross Compile Script

# clear the screen
clear

while :
	do
	# If error exists, display it
	if [ "$ERR_MSG" != "" ]; then
		echo "Error: $ERR_MSG"
		echo ""
	fi
	
	#Check if there is a permenant installation of MXE. This will save a LOT of time.
	if [ -d "/opt/mxe/" ]; then
		# MXE is already installed
		MXE_INSTALLED = 1
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
			if (uname -a | grep x86_64); then
				sudo apt-get install g++-multilib libc6-dev-i386
			fi
			;;
		2)	#download the proper git repositories
			echo "This will clone the repositories for Hydrogen and MXE."
			read -e -p "Enter the path where Hydrogen should be built: " -i "$HOME/build/hydrogen_win32/" CLONEPATH
			if [ -d "$CLONEPATH" ]; then
				cd "$CLONEPATH"
			else
				mkdir "$CLONEPATH"
				cd "$CLONEPATH"
			fi
			export BUILD_DIR=$PWD
			#git clone https://github.com/hydrogen-music/hydrogen.git
			git clone https://github.com/mikotoiii/hydrogen.git
			if [ "$MXE_INSTALLED" != "1" ]; then
				git clone https://github.com/mxe/mxe.git
			fi
			;;
		3)	#Set the required variables
			echo "Now starting the building of Hydrogen for Windows. This will take quite a while and requires no interaction after the intial questions."
			
			export HYDROGEN=$CLONEPATH/hydrogen
			if [$MXE_INSTALLED != 1]; then
				if [ -z "$MXE" ]; then
					export MXE=$CLONEPATH/mxe
				fi
				# Ask if MXE should be installed
				echo "Would you like to permenantly install MXE into /opt/mxe to save time for future builds?"
				while true; do
				    read -p "Do you wish to install this program?" yn
				    case $yn in
					[Yy]* ) PERMENANT_INSTALL=1; break;;
					[Nn]* ) exit;;
					* ) echo "Please answer yes or no.";;
				    esac
				done

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
			else
				export MXE=/opt/mxe
			fi

			#Build hydrogen itself now.
			mkdir $HYDROGEN/win32/windows_32_bit_build
			export HYDROGEN_BUILD=$HYDROGEN/win32/windows_32_bit_build
			cd $HYDROGEN_BUILD
			cmake ../.. -DCMAKE_TOOLCHAIN_FILE=$MXE/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake
			make
			sh ../create_bundle.sh

			#Check if PERMENANT_INSTALL is set to 1, and copy the files over
			if [ "$PERMENANT_INSTALL" == 1]; then
				sudo mv $MXE /opt/mxe
			fi
			;;
		4)	#Clean up the files
			echo "Now cleaning up the files. This process will move the built hydrogen into your home directory and delete the build files. If MXE was not permenantly installed, it will remove that too."
			mv $HYDROGEN_BUILD/windows_32_bit_build $HOME/hydrogen_windows_32_bit_build
			rm -rf $HYDROGEN
			if [ "$MXE_INSTALLED" != 1]; then
				rm -rf $MXE
			fi
			;;
		5)	echo "Thank you for using the Hydrogen Cross Compiler. Goodbye."
			exit
			;;
		*) ERR_MSG="Please enter a valid option"
	esac

	# clear the screen again for re-display
	clear
done