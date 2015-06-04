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
		MXE_INSTALLED=1
		MXE=/opt/mxe
	fi

	# Write out the menu options...
	echo "Welcome to the Hydrogen Cross Compiler. We will now compile Hydrogen for Windows."
	echo "Select an option:"
	echo " 1: Prepare the system"
	echo " 2: Clone required repositories"
	echo " 3: Build Hydrogen"
	echo " 4: Experimental 64 Bit Compiling"
	echo " 5: Build Windows Installer"
	echo " 6: Clean up Files"
	echo " q: Exit"

	# Clear the error message
	ERR_MSG=""

	# Read the user input
	read SEL

	case $SEL in
		1)	#Prepares the system by getting the necessary packages to perform the cross compile
			echo "Now installing required packages"
			sudo apt-get install autoconf automake autopoint bash bison bzip2 cmake flex gettext git gcc g++ intltool libffi-dev libtool libtool-bin libltdl-dev libssl-dev libxml-parser-perl make openssl patch perl pkg-config scons sed unzip wget xz-utils nsis
			if (uname -a | grep x86_64); then
				sudo apt-get install g++-multilib libc6-dev-i386
			fi
			;;
		2)	#download the proper git repositories
			echo "This will clone the repositories for Hydrogen and MXE."
			read -e -p "Enter the path where Hydrogen should be built: " -i "$HOME/build/hydrogen_win32/" CLONEPATH
			if [ ! -e "${CLONEPATH%/*}" ]; then
				echo "Now downloading Hydrogen."
				mkdir -p "${CLONEPATH%/*}"
				cd "${CLONEPATH%/*}"
				BUILD_DIR=$PWD
				#git clone https://github.com/hydrogen-music/hydrogen.git
				git clone https://github.com/mikotoiii/hydrogen.git
			else 
				echo "Hydrogen already downloaded to ${CLONEPATH%/*}."
			fi
			if [ ! "$MXE_INSTALLED" == "1" ]; then
				echo "Now downloading MXE."
				git clone https://github.com/mxe/mxe.git
			else
				echo "MXE already downloaded to $MXE"
			fi
			if [ ! -e ${CLONEPATH%/*}/jack2 ]; then
				echo "Now downloading jack."
				git clone git://github.com/jackaudio/jack2.git
			fi
			;;
		3)	#Set the required variables
			echo "Now starting the building of Hydrogen for Windows. This will take quite a while and requires no interaction after the intial questions."
			if [ -z ${CLONEPATH%/*} ]; then
				read -e -p "Enter the path to the Hydrogen download: " -i "$HOME/build/hydrogen_win32/" CLONEPATH
			fi
			HYDROGEN="${CLONEPATH%/*}/hydrogen"
			echo "Checking for MXE."
			if [ ! "$MXE_INSTALLED" == "1" ]; then
				if [ -z "$MXE" ]; then
					MXE="${CLONEPATH%/*}/mxe"
				fi
				# Ask if MXE should be installed
				echo "MXE was not found. We will now build MXE."
				echo "Would you like to permenantly install MXE into /opt/mxe to save time for future builds?"
				while true; do
				    read -p "Do you wish to install this program? (y / n)" yn
				    case $yn in
					[Yy]* ) #move mxe files to /opt and build it there!
						sudo mv $MXE /opt/mxe
						cd /opt/mxe
						MXE=/opt/mxe
						PATH=/opt/mxe/usr/bin:$PATH;
						break;;
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
				make qt libarchive libsndfile portaudio portmidi fftw rubberband -j4 JOBS=4
			else
				echo "MXE found at $MXE"
			fi

			#Build hydrogen itself now.
			echo "Now building Hydrogen."
			HYDROGEN_BUILD="$HYDROGEN/win32/windows_32_bit_build"
			if [ ! -e "$HYDROGEN_BUILD" ]; then
				mkdir "$HYDROGEN/win32/windows_32_bit_build"
			fi
			echo "We will now build Hydrogen at $HYDROGEN_BUILD"
			cd "$HYDROGEN_BUILD"
			if [ ! -e "$MXE/usr/i686-w64-mingw32.shared/bin/libjack.dll" ]; then
				while true; do	
					read -p "would you like to build Hydrogen with Jack support? (y / n) Note: If you have libjack.dll already installed in mxe, you can answer no here." yn
					case $yn in
						[Yy]* ) echo "You will now need to copy the libjack.dll file from a Windows machine (C:\Windows\SysWow64\libjack.dll) to your mxe directory."; 
							echo "We can try to automatically move the file to the proper location" 
							read -e -p "Enter the path to libjack.dll: " -i "$HOME" LIBJACK_PATH
							if [ -f $LIBJACK_PATH ]; then
								sudo cp $LIBJACK_PATH $MXE/usr/i686-w64-mingw32.shared/bin/	
							fi
							if [ ! -e $MXE/usr/i686-w64-mingw32.shared/include/jack ]; then
								ln -s ${CLONEPATH%/*}/jack2/common/jack $MXE/usr/i686-w64-mingw32.shared/include/jack
							fi
							break;;
						[Nn]* ) break;;
						* ) echo "Please answer yes or no.";;
					esac
				done
			else
				echo "libjack.dll was found at $MXE/usr/i686-w64-mingw32.shared/bin/"
			fi
			cmake ../.. -DCMAKE_TOOLCHAIN_FILE=$MXE/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake
			make
			cd ..
			export HYDROGEN
			export HYDROGEN_BUILD
			export MXE
			./create_bundle.sh
			if [ ! -d $HOME/Hydrogen ]; then
				rm -rf $HOME/Hydrogen
			fi
			mv hydrogen_windows_32_bit $HOME/Hydrogen
			#Check if PERMENANT_INSTALL is set to 1, and copy the files over
			if [ "$PERMENANT_INSTALL" == "1" ]; then
				sudo mv $MXE /opt/mxe
			fi
			;;
		4)	#Experimental 64 Bit Compiling
			echo "Coming Soon"
			;;
		5)	#Build Windows Installer
			cd $HOME/Hydrogen
			cp -r $HYDROGEN ./src/Hydrogen
			if [ ! -e jack_installer ]; then
				mkdir jack_installer
			fi
			cd jack_installer
			if [ ! -e "Jack_v1.9.10_64_setup.exe" ]; then
				wget https://dl.dropboxusercontent.com/u/28869550/Jack_v1.9.10_64_setup.exe
			fi
			cd ..
			if [ ! -e plugins ]; then
				mkdir plugins
				cd plugins
				if [ ! -e ladspaplugs ]; then
					mkdir ladspaplugs
				fi
				cd ladspaplugs
				if [ ! -e "LADSPA_plugins-win-0.4.15.exe" ]; then
					wget http://downloads.sourceforge.net/audacity/LADSPA_plugins-win-0.4.15.exe
				fi
				cd ..
			fi
			cd ..
			if [ ! -e "gpl-3.0.txt" ]; then
				wget http://www.gnu.org/licenses/gpl-3.0.txt
			fi
			cp $HYDROGEN/win32/make_installer.nsi ./
			makensis make_installer.nsi
			;;
		6)	#Clean up the files
			echo "Now cleaning up the files. This process will move the built hydrogen into your home directory and delete the build files. If MXE was not permenantly installed, it will remove that too."
			mv $HYDROGEN_BUILD/windows_32_bit_build $HOME/hydrogen_windows_32_bit_build
			rm -rf $HYDROGEN
			rm -rf $HOME/Hydrogen
			if [ ! "$MXE_INSTALLED" == "1" ]; then
				rm -rf $MXE
			fi
			;;
		q)	echo "Thank you for using the Hydrogen Cross Compiler. Goodbye."
			exit
			;;
		*) ERR_MSG="Please enter a valid option"
	esac

	# clear the screen again for re-display
	#clear
done