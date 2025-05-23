#!/bin/bash
# Hydrogen Cross Compile Script

FATBUILD=false

show_interactive_menu(){
	#clear the screen
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
		echo " 1: Clone required repositories"
		echo " 2: Build Hydrogen 32Bit"
		echo " 3: Build Hydrogen 64Bit"
		echo " 4: Clean Cmake and CPack Cache Files"
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
					git clone https://github.com/hydrogen-music/hydrogen.git

				else 
					if [ -f ${CLONEPATH%/*}/build.sh ]; then
						mv ${CLONEPATH%/*} ${CLONEPATH%/*}/../hydrogen.tmp
						mkdir -p ${CLONEPATH%/*}
						mv ${CLONEPATH%/*}/../hydrogen.tmp ${CLONEPATH%/*}/source
						echo "Hydrogen already downloaded to ${CLONEPATH%/*}."
					fi
				fi
				if [ ! -e ${CLONEPATH%/*}/source/jack2 ]; then
					cd ${CLONEPATH%/*}/source
					echo "Now downloading jack."
					git clone git://github.com/jackaudio/jack2.git
					cd ..
				fi
				;;
			2)	#32 Bit Compiling
				build_32bit
				;;
			3)	#64 Bit Compiling
				build_64bit
				;;
			4)	#Clean CMake Files
				cd $HYDROGEN_BUILD
				rm -r CMakeCache.txt CMakeFiles cmake_install.cmake CPackConfig.cmake _CPack_Packages CPackSourceConfig.cmake install_manifest.txt ladspa_listplugins Makefile src try uninstall.cmake
				rm ../mxe ../gcc
				;;

			q)	echo "Thank you for using the Hydrogen Cross Compiler. Goodbye."
				exit
				;;
			*) ERR_MSG="Please enter a valid option"
		esac

		# clear the screen again for re-display
		#clear
	done
}


build_32bit(){
			build_hydrogen i686
}

build_64bit(){
			build_hydrogen x86_64 -DCMAKE_{C,CXX}_FLAGS=-m64 -DWIN64:BOOL=ON
}
mxe_files(){
			# This will search for the required files to package and put their names into a file to be read by cmake.
			# This is necessary due to version differences with the libraries and compilers that mxe uses across build systems.

	#set the dir to search
	if [ "$1" == "64" ]; then
	    mxedir="/opt/mxe/usr/x86_64-w64-mingw32.shared/bin"
	    gccdir="/opt/mxe/usr/lib/gcc/x86_64-w64-mingw32.shared"
	else
	    mxedir="/opt/mxe/usr/i686-w64-mingw32.shared/bin"
	    gccdir="/opt/mxe/usr/lib/gcc/i686-w64-mingw32.shared"
	fi
	if [ -d windows ]; then
		hydrogendir=`pwd`"/windows"
	else
		hydrogendir=`pwd`
	fi
	extralibs="$hydrogendir/extralibs"

	mkdir $extralibs

	#Make arrays for the filenames to loop through.
	declare -a libs=("liblzo2-2" "libgnurx" "libsndfile" "libFLAC" "libogg" "libvorbis" "libvorbisenc" "zlib1" "libwinpthread"  "libarchive" "libbz2" "liblzma" "libnettle" "libxml2" "libpng16" "libportmidi" "libportaudio" "libiconv" "libiconv"  "liblo" "jack" "libzstd" "libssp" "libopus-0")
	declare -a gcclibs=("libgcc" "libstdc++")

	#special stuff for qt5 handling
	declare -a qtlibs=("Qt5Core" "Qt5Xml." "Qt5Network." "Qt5Gui" "Qt5Widgets")
	libs+=("libpcre-1" "libharfbuzz-0" "libfreetype-6" "libglib-2" "libintl-8" "libpcre2-16" "libcrypto" "libssl")
	qtdir="$mxedir/../qt5/bin"
	platforms="$extralibs/platforms"
	mkdir -p $platforms
	cp "$qtdir/../plugins/platforms/qwindows.dll" "$platforms/"

	#loop through the libs, and put them into a text file.
	cd $mxedir
	for mylibs in "${libs[@]}"
	do
		cp `ls -v $mylibs*dll| tail -n 1` $extralibs
	done
	#loop through the qt files and put them into a text file
	cd $qtdir
	for myqtlibs in "${qtlibs[@]}"
	do
		cp `ls -v $myqtlibs*dll| tail -n 1` $extralibs
		spa=" "
	done
	#find the latest gcc dir from mxe
	cd $gccdir
	echo `ls -v | tail -n 1` > $hydrogendir/../gccversion.txt
	#loop through the gcclibs
	cd $mxedir
	for mygcclibs in "${gcclibs[@]}"
	do
            cp `ls -v $mygcclibs*dll| tail -n 1` $extralibs
	done
}

setqt5(){
	if [ "$1" == "64" ]; then
		qt5dir="/opt/mxe/usr/x86_64-w64-mingw32.shared/qt5/lib/cmake"
	else
		qt5dir="/opt/mxe/usr/i686-w64-mingw32.shared/qt5/lib/cmake"
	fi

	qtprefix="-DCMAKE_PREFIX_PATH=$qt5dir"
}

cleanbuild(){
	if [ -d windows ]; then
		cd windows
	fi
	if [ -f CMakeCache.txt ]; then
		rm -rf _CPack_Packages CMakeFiles try src extralibs
		rm -f CMakeCache.txt CPackConfig.cmake cmake_install.cmake CPackSourceConfig.cmake install_manifest.txt ladspa_listplugins Makefile uninstall.cmake
	fi
}

build_hydrogen(){
	# Passes either i686 or x86_64 for 32 or 64 bit respectively.
	if [ -z "$HYDROGEN" ] || [ ! -d $HYDROGEN ]; then
		echo "Now starting the building of Hydrogen for Windows. This will take quite a while and requires no interaction after the initial questions."
		if [ -z ${CLONEPATH%/*} ]; then
			read -e -p "Enter the path to the Hydrogen download (with a trailing /): " -i "$HOME/build/hydrogen/" CLONEPATH
		fi

		HYDROGEN="${CLONEPATH}"
		if [ ! -d $HYDROGEN ]; then
			echo "Hydrogen source not found in $HYDROGEN."
			exit
		fi
	fi

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
	HYDROGEN_BUILD=$HYDROGEN"windows"
	if [ ! -e "$HYDROGEN_BUILD" ]; then
		mkdir $HYDROGEN"windows"
	fi
	echo "We will now build Hydrogen at $HYDROGEN_BUILD"
	cd "$HYDROGEN_BUILD"
	if [ -e ../CMakeCache.txt ]; then
		echo "Previous build detected. We will now remove the caches so the project will build properly."
		rm -rf _CPack_Packages CMakeFiles try
		rm -f CMakeCache.txt CPackConfig.cmake cmake_install.cmake CPackSourceConfig.cmake install_manifest.txt ladspa_listplugins Makefile uninstall.cmake
	fi

	PATH=/opt/mxe/usr/bin:$PATH;

	if [ "$1" == "x86_64" ]; then
	    MXE_CMAKE_BIN=/opt/mxe/usr/bin/x86_64-w64-mingw32.shared-cmake 
	else
	    MXE_CMAKE_BIN=/opt/mxe/usr/bin/i686-w64-mingw32.shared-cmake 
	fi
	
	$MXE_CMAKE_BIN $4 ../ $2 $3 -DWANT_FAT_BUILD:BOOL=$FATBUILD -DWANT_DEBUG:BOOL=OFF

	export HYDROGEN
	export HYDROGEN_BUILD
	export MXE
	

	#Bundle jack_installer if wanted..
	if [ $FATBUILD = true ]; then
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
				wget http://sourceforge.net/projects/audacity/files/audacity/2.0.5/LADSPA_plugins-win-0.4.15.exe
			fi
			cd ..
		fi
	fi

	cd $HYDROGEN_BUILD
	
	if [ -e $HYDROGEN/mxe ]; then
		if [ ! -h $HYDROGEN/mxe ]; then
			rm -rf $HYDROGEN/mxe
		fi
	fi
	if [ -e $HYDROGEN/extralibs ]; then
		rm -rf $HYDROGEN/extralibs
	fi
	if [ ! -e $HYDROGEN/mxe ]; then
		ln -s $MXE/usr/$1-w64-mingw32.shared $HYDROGEN/mxe
	fi
	if [ ! -e $HYDROGEN/gcc ]; then
		ln -s $MXE/usr/lib/gcc $HYDROGEN/gcc
	fi
	cpack -G NSIS
}

usage(){
	echo -e "\nManual mode:\t\tcross_compile.sh [-f] [-d SOURCE_DIR] [-c] -b i686|x86_64"
	echo -e "Interactive mode:\tcross_compile.sh -i"
	echo -e "Usage: \n\t-i:\tUse interactive mode \n\t-b:\tBuild hydrogen. Valid values: i686 or x86_64 \n\t-f:\tFat build (includes Jack and Ladspa installers). Only useful in combination with -b \n\t-c:\tClean the CMake files from the windows directory. Used if building fails, or compiling a different version.\n\t-r:\tBuild release packages. This will build both the 32 bit and 64 bit installers for releases."
}

fatbuild=false

while getopts "d:fob:icr" o; do
	case "${o}" in
		d)
			HYDROGEN=${OPTARG}
			if [ ! -d $HYDROGEN ]; then
				echo "Hydrogen source not found in $HYDROGEN."
				exit
			fi
			;;
		f)
			FATBUILD=true
			;;
		b)
			arch=${OPTARG}

			if [ "$arch" != "x86_64" ]; then
				cleanbuild
				mxe_files 32
				setqt5
				build_32bit
			else
				cleanbuild
				mxe_files 64
				setqt5
				build_64bit
			fi
			;;
		i)
			show_interactive_menu
			;;
		c)
			cleanbuild
			;;
		r)
			cleanbuild
			mxe_files 32
			setqt5
			build_32bit
			cleanbuild
			mxe_files 64
			setqt5
			build_64bit
			;;
		*)
			usage
			;;
	esac
done
if [ $OPTIND -eq 1 ]; then usage; fi
shift $((OPTIND-1))
