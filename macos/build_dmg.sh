#!/bin/bash


# Default application bundle name
SRC_APP="hydrogen.app"

# Default dmg image name
DMG_PATH="Hydrogen.dmg"

# Option: enable verbosity
VERBOSE=0

# Option: open image after creating it
OPEN=0


# Print message if verbose mode is enabled
function verbose {
	if (( $VERBOSE )); then
		echo $@
	fi
}


# Show error message and exit (with cleanup)
function error {
	echo $@ >&2
	clean_up
	exit 1
}


# Show usage info
function usage {
	echo "Usage: build_dmg.sh [-vho] [hydrogen.app] [hydrogen.dmg]"
}


# Print help message
function show_help {
	cat<<EOF
Build Hydrogen .dmg image

Usage: build_dmg.sh [-vho] [hydrogen.app] [hydrogen.dmg]

  -v Be verbose
  -h Show this help message
  -o Open image afterwards

EOF
}


# Verify source app bundle
function verify_app {
	APP="$1"
	if [ ! -d "$APP" ]; then
		error "Can't find $APP"
	fi
	if [ ! -x "$APP/Contents/MacOS/hydrogen" ]; then
		error "Can't find executable in $APP"
	fi
}


# Perform clean up; remove temporary directories
function clean_up {
	rm -rf "$DMG_ROOT"
}




# Parse options

while getopts ":vho" opt; do
	case $opt in
		v)
			VERBOSE=1
			;;
		h)
			show_help
			exit
			;;
		o)
			OPEN=1
			;;
		\?)
			echo "Unknown option: $OPTARG"
			usage;
			exit 1;
			;;
	esac
done
shift $((OPTIND - 1))


if (( $# > 0 )); then
	SRC_APP="$1"
fi
if (( $# > 1 )); then
	DMG_PATH="$2"
fi
if (( $# > 2 )); then
	usage;
	exit 1
fi


# Real work

if [ -f "$DMG_PATH" ]; then
	error "Output file exists"
fi

verify_app "$SRC_APP"

DMG_ROOT=`mktemp -d`

verbose "Copying application bundle"
cp -r "$SRC_APP" "$DMG_ROOT/Hydrogen.app" || error "Can't copy $SRC_APP"

verbose "Deploying Qt libraries"
macdeployqt "$DMG_ROOT/Hydrogen.app" || error "macdeployqt failed"

verbose "Deploying additional assets"
ln -s /Applications "$DMG_ROOT/Applications"
cp "${BASH_SOURCE%/*}/DS_Store" "$DMG_ROOT/.DS_Store"

verbose "Creating dmg image"
hdiutil create -srcfolder "$DMG_ROOT" -volname "Hydrogen" -fs HFS+ -format UDZO "$DMG_PATH" || error "Creating dmg image failed"

verbose "Cleaning up"
clean_up

if (( $OPEN )); then
	open "$DMG_PATH"
fi
