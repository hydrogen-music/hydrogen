#!/bin/bash
#This script will try to install mxe for you.

MXE_INSTALLED=0

if [ -d /opt/mxe ]; then
	if [ -f /opt/mxe/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake ] || [ -f /opt/mxe/usr/x86_64-w64-mingw32.shared/share/cmake/mxe-conf.cmake ]; then
		MXE_INSTALLED=1
	fi
fi

install_mxe(){
	if [ $MXE_INSTALLED == 1 ]; then
		echo "mxe detected at /opt/mxe"
		exit
	fi
	mkdir -p $HOME/build
	cd $HOME/build
	git clone https://github.com/mxe/mxe.git
	sudo cp -pr mxe /opt/mxe
	cd /opt/mxe
	MXE=/opt/mxe
	PATH=/opt/mxe/usr/bin:$PATH;
	#Modify the MXE Makefiles to allow for the cross compilation.
	sed -i 's/i686-w64-mingw32.static/i686-w64-mingw32.shared x86_64-w64-mingw32.shared/g' $MXE/Makefile
	#Make gcc and winpthreads. gcc will need to be rebuilt once winpthreads is built.
	#Note: This needs to happen because winpthreads needs gcc to be built, but we need gcc built with winpthreads support to build hydrogen.
	#	this creates a cyclical dependency problem that there seems to be no way around.
	make gcc $1
	make winpthreads $1
	sed -i 's/binutils gcc-gmp gcc-isl gcc-mpc gcc-mpfr mingw-w64/binutils gcc-gmp gcc-isl gcc-mpc gcc-mpfr winpthreads/g' $MXE/src/gcc.mk
	sed -i 's/--enable-threads=win32/--enable-threads=posix/g' $MXE/src/gcc.mk
	make gcc $1

	# Disable WDM-KS in PortAudio since it causes crashes
	sed -i 's/--with-winapi=\(.*\),wdmks/--with-winapi=\1/' $MXE/src/portaudio.mk
	if grep wmdks $MXE/src/portaudio.mk; then
		echo "*** $MXE/src/portaudio.mk still references wdmks."
		exit 1
	fi

	#Build the dependencies for hydrogen
	make libarchive libsndfile portaudio portmidi fftw rubberband jack liblo qt5 -j4 JOBS=4 $1
	sed -i 's/:= gcc/:= gcc jack/g' $MXE/src/portaudio.mk
	make portaudio
}

while :
	do
	# If error exists, display it
	if [ "$ERR_MSG" != "" ]; then
		echo "Error: $ERR_MSG"
		echo ""
	fi
	
	# Write out the menu options...
	echo "Welcome to the MXE Installer. This will help install MXE on your system."
	echo "Note: this script was made using the requirement information found at http://mxe.cc/#requirements"
	echo "You can visit the above site for manual instructions on what packages are required. For instructions on how to build MXE you can visit this link: http://mxe.cc/#tutorial"
	echo "Select an option:"
	echo " 1: Debian and derrivatives"
	echo " 2: Fedora"
	echo " 3: FreeBSD (NOTE: No longer fully supported by MXE. See above requirements link.)"
	echo " 4: Frugalware"
	echo " 5: Gentoo"
	echo " 6: Mac OS X"
	echo " 7: openSUSE"
	echo " q: Exit"

	# Clear the error message
	ERR_MSG=""

	
	# Read the user input
	read SEL

	case $SEL in
		1)	#Debian and derivatives
			echo "Now installing required packages"
			sudo apt-get install autoconf automake autopoint bash bison bzip2 cmake flex gettext git gcc g++ gperf intltool libffi-dev libtool libtool-bin libltdl-dev libssl-dev libxml-parser-perl lzip make openssl patch perl pkg-config python ruby scons sed unzip wget xz-utils nsis
			if (uname -a | grep x86_64); then
				sudo apt-get install g++-multilib libc6-dev-i386
			fi
			install_mxe
			;;
		2)	#Fedora
			echo "Now installing required packages"
			yum install autoconf automake bash bison bzip2 cmake flex gcc-c++ gettext git gperf intltool make sed libffi-devel libtool openssl-devel patch perl pkgconfig python ruby scons unzip wget xz
			install_mxe EXCLUDE_PKGS='ocaml%'
			;;
		3)	#FreeBSD
			echo "FreeBSD is no longer fully supported. This may install the package; however, it may not work as expected. please visit http://mxe.cc/#requirements for more information."
			echo "Now installing required packages"
			pkg_add -r automake autoconf bash bison cmake coreutils flex gettext git glib20 gmake gperf gsed intltool libffi libtool openssl patch perl p5-XML-Parser pkgconf python ruby scons unzip wget
			install_mxe EXCLUDE_PKGS='ocaml%'
			;;
		4)	#Frugalware
			echo "Now installing required packages"
			pacman-g2 -S autoconf automake bash bzip2 bison cmake flex gcc gettext git gperf intltool make sed libffi libtool openssl patch perl perl-xml-parser pkgconfig python ruby scons unzip wget xz xz-lzma
			install_mxe EXCLUDE_PKGS='ocaml%'
			;;
		5)	#Gentoo
			echo "Now installing required packages"
			emerge sys-devel/autoconf sys-devel/automake app-shells/bash sys-devel/bison app-arch/bzip2 dev-util/cmake sys-devel/flex sys-devel/gcc sys-devel/gettext dev-vcs/git dev-util/gperf dev-util/intltool sys-devel/make sys-apps/sed dev-libs/libffi sys-devel/libtool dev-libs/openssl sys-devel/patch dev-lang/perl dev-perl/XML-Parser dev-util/pkgconfig dev-lang/python dev-lang/ruby dev-util/scons app-arch/unzip net-misc/wget app-arch/xz-utils
			install_mxe
			;;
		6)	#Mac OS X
			echo "You may be prompted to install a java runtime - this is not required."
			echo "Mac OS X versions ≤ 10.7 are no longer supported."
			while :
                                do
				echo "1) If you have macports installed select this."
				echo "2) If you do NOT have macports installed, this will install Rudix and then install MXE."
				read MAC
				case $MAC in
					1)#Macports IS installed
						sudo port install glib2 intltool p5-xml-parser gpatch scons wget xz
						make build-requirements
						install_mxe
						;;
					2)#Macports is NOT installed. Install Rudix
						curl -s https://raw.githubusercontent.com/rudix-mac/rpm/2014.6/rudix.py | sudo python - install rudix
						sudo rudix install glib pkg-config scons wget xz
						make build-requirements
						install_mxe
						;;
				esac

			done
			;;
		7)	#openSUSE
			echo "Now installing required packages"
			zypper install -R autoconf automake bash bison bzip2 cmake flex gcc-c++ gettext-tools git gperf intltool libffi-devel libtool make openssl libopenssl-devel patch perl perl-XML-Parser pkg-config python ruby scons sed unzip wget xz
			if (uname -a | grep x86_64); then
				zypper install -R gcc-32bit glibc-devel-32bit libgcc46-32bit libgomp46-32bit libstdc++46-devel-32bit
			fi
			install_mxe
			;;
		q)	echo "Thank you for using the MXE installer. Goodbye."
			exit
			;;
		*) ERR_MSG="Please enter a valid option"
	esac

	# clear the screen again for re-display
	#clear
done
