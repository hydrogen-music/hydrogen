#!/bin/bash
# Hydrogen Cross Compile Script

# clear the screen
clear

build_hydrogen(){
	# Passes either i686 or x86_64 for 32 or 64 bit respectively.
	echo "Now starting the building of Hydrogen for Windows. This will take quite a while and requires no interaction after the intial questions."
	if [ -z ${CLONEPATH%/*} ]; then
		read -e -p "Enter the path to the Hydrogen download: " -i "$HOME/build/hydrogen/" CLONEPATH
	fi
	HYDROGEN="${CLONEPATH%/*}/source"
	echo "Checking for MXE."
	if [ -d /opt/mxe ]; then
		if [ -f /opt/mxe/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake ] || [ -f /opt/mxe/usr/x86_64-w64-mingw32.shared/share/cmake/mxe-conf.cmake ]; then
			MXE_INSTALLED=1
			MXE=/opt/mxe
		fi
	else
		echo "mxe was not found, please run the mxe_installer.sh script first."
		exit
	fi
	
	#Build hydrogen itself now.
	echo "Now building Hydrogen."
	HYDROGEN_BUILD="$HYDROGEN/windows"
	if [ ! -e "$HYDROGEN_BUILD" ]; then
		mkdir "$HYDROGEN/windows"
	fi
	echo "We will now build Hydrogen at $HYDROGEN_BUILD"
	cd "$HYDROGEN_BUILD"
	if [ ! -e "$MXE/usr/$1-w64-mingw32.shared/bin/libjack.dll" ]; then
		while true; do	
			read -p "would you like to build Hydrogen with Jack support? (y / n) Note: If you have libjack.dll already installed in mxe, you can answer no here." yn
			case $yn in
				[Yy]* ) echo "You will now need to copy the libjack.dll file from a Windows machine (C:\Windows\SysWow64\libjack.dll) to your mxe directory.";
					echo "If you're building 64 bit, you will need to copy libjack64.dll and rename it to libjack.dll which can be found at (C:\Windows)."
					echo "We can try to automatically move the file to the proper location" 
					read -e -p "Enter the path to libjack.dll: " -i "$HOME" LIBJACK_PATH
					if [ -f $LIBJACK_PATH ]; then
						sudo cp $LIBJACK_PATH $MXE/usr/$1-w64-mingw32.shared/bin/	
					fi
					if [ ! -e $MXE/usr/$1-w64-mingw32.shared/include/jack ]; then
						ln -s ${CLONEPATH%/*}/jack2/common/jack $MXE/usr/$1-w64-mingw32.shared/include/jack
					fi
					break;;
				[Nn]* ) break;;
				* ) echo "Please answer yes or no.";;
			esac
		done
	else
		if [ ! -h $MXE/usr/$1-w64-mingw32.shared/include/jack ]; then
			if [ ! -e $MXE/usr/$1-w64-mingw32.shared/include/jack ]; then
				rm $MXE/usr/$1-w64-mingw32.shared/include/jack
			fi
			ln -s ${CLONEPATH%/*}/jack2/common/jack $MXE/usr/$1-w64-mingw32.shared/include/jack
		fi
		echo "libjack was found at $MXE/usr/$1-w64-mingw32.shared/bin/"
	fi
	cd ../..
	if [ -e CMakeCache.txt ]; then
		echo "Previous build detected. We will now remove the caches so the project will build properly."
		rm -rf _CPack_Packages CMakeFiles try
		rm -f CMakeCache.txt CPackConfig.cmake cmake_install.cmake CPackSourceConfig.cmake install_manifest.txt ladspa_listplugins Makefile uninstall.cmake
	fi
	cmake source -DCMAKE_TOOLCHAIN_FILE=$MXE/usr/$1-w64-mingw32.shared/share/cmake/mxe-conf.cmake $2
	export HYDROGEN
	export HYDROGEN_BUILD
	export MXE
	if [ ! -e jack_installer ]; then
		mkdir jack_installer
	fi
	cd jack_installer
	if [ $1 == "x86_64" ]; then
		if [ ! -e "Jack_v1.9.10_64_setup.exe" ]; then
			wget https://dl.dropboxusercontent.com/u/28869550/Jack_v1.9.10_64_setup.exe
		fi
	else
		if [ ! -e "Jack_v1.9.10_32_setup.exe" ]; then
			wget https://dl.dropboxusercontent.com/u/28869550/Jack_v1.9.10_32_setup.exe
		fi
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
	cd $CLONEPATH
	echo $PWD
	if [ -e $HYDROGEN/mxe ]; then
		if [ ! -h $HYDROGEN/mxe ]; then
			rm -rf $HYDROGEN/mxe
		fi
	fi
	ln -s $MXE/usr/$1-w64-mingw32.shared $HYDROGEN/mxe
	ln -s $MXE/usr/lib/gcc $HYDROGEN/gcc
	cpack -G NSIS
}


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
	echo " 1: Clone required repositories"
	echo " 2: Build Hydrogen"
	echo " 3: Experimental 64 Bit Compiling"
	echo " q: Exit"

	# Clear the error message
	ERR_MSG=""

	
	# Read the user input
	read SEL

	case $SEL in
		1)	#download the proper git repositories
			echo "This will clone the repositories for Hydrogen"
			read -e -p "Enter the path where Hydrogen should be built: " -i "$HOME/build/hydrogen/" CLONEPATH
			if [ ! -e "${CLONEPATH%/*}" ]; then
				echo "Now downloading Hydrogen."
				mkdir -p "${CLONEPATH%/*}"
				cd "${CLONEPATH%/*}"
				BUILD_DIR=$PWD
				#git clone https://github.com/hydrogen-music/hydrogen.git
				git clone https://github.com/mikotoiii/hydrogen.git

			else 
				if [ -f ${CLONEPATH%/*}/build.sh ]; then
					mv ${CLONEPATH%/*} ${CLONEPATH%/*}/../hydrogen.tmp
					mkdir -p ${CLONEPATH%/*}
					mv ${CLONEPATH%/*}/../hydrogen.tmp ${CLONEPATH%/*}/source
					echo "Hydrogen already downloaded to ${CLONEPATH%/*}."
				fi
				if [ ! -e ${CLONEPATH%/*}/source/jack2 ]; then
					cd ${CLONEPATH%/*}/source
					echo "Now downloading jack."
					git clone git://github.com/jackaudio/jack2.git
					cd ..
				fi
			fi
			;;
		2)	#Set the required variables
			build_hydrogen i686
			;;
		3)	#Experimental 64 Bit Compiling
			build_hydrogen x86_64 -DCMAKE_{C,CXX}_FLAGS=-m64
			;;
		q)	echo "Thank you for using the Hydrogen Cross Compiler. Goodbye."
			exit
			;;
		*) ERR_MSG="Please enter a valid option"
	esac

	# clear the screen again for re-display
	#clear
done