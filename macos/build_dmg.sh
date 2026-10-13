#!/bin/bash

# Hydrogen
# Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
# Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
#
# http://www.hydrogen-music.org
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# Default application bundle name
SRC_APP="hydrogen.app"

# Default dmg image name
DMG_PATH="Hydrogen.dmg"

# Option: enable verbosity
VERBOSE=0

# Option: open image after creating it
OPEN=0

# Option: Translation directory
TRANSLATIONS="data/i18n"

# Option: directory to search for built plugin bundles (Hydrogen.clap /
# Hydrogen.vst3 / hydrogen.lv2). Empty => plugin staging disabled. When set, any
# bundles found are copied into a "Plugins" folder in the disk image so users can
# drag them into ~/Library/Audio/Plug-Ins/{CLAP,VST3} (and the .lv2 bundle into
# an LV2 path). Like the Linux .tar.xz, the bundles ship against the same Qt as
# the app — a fully self-contained (Qt-bundled) plugin is a follow-up.
PLUGIN_DIR=""


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
  -t Translation directory (default: data/i18n)
  -p Directory to search for built plugin bundles (Hydrogen.clap,
     Hydrogen.vst3, hydrogen.lv2); staged into a "Plugins" folder in the image

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

while getopts ":vhot:p:" opt; do
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
        t)
            TRANSLATIONS="$OPTARG"
            ;;
        p)
            PLUGIN_DIR="$OPTARG"
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

verbose "Deploying translations"
I18N_DEST="$DMG_ROOT/Hydrogen.app/Contents/Resources/data/i18n"
mkdir -p "$I18N_DEST"
find "$TRANSLATIONS" -name '*.qm' -exec cp {} "$I18N_DEST" \;

if [ -n "$PLUGIN_DIR" ]; then
	verbose "Staging plugin bundles from $PLUGIN_DIR"
	PLUGIN_DEST="$DMG_ROOT/Plugins"
	staged_any=0
	for bundle in "Hydrogen.clap" "Hydrogen.vst3" "hydrogen.lv2"; do
		# VST3 may be emitted under a per-config subdir (e.g. Release/).
		src=""
		if [ -e "$PLUGIN_DIR/$bundle" ]; then
			src="$PLUGIN_DIR/$bundle"
		else
			src=$(find "$PLUGIN_DIR" -maxdepth 3 -name "$bundle" -print -quit 2>/dev/null)
		fi
		if [ -n "$src" ] && [ -e "$src" ]; then
			mkdir -p "$PLUGIN_DEST"
			cp -R "$src" "$PLUGIN_DEST/" || error "Can't copy $src"
			verbose "  + $bundle"
			staged_any=1
		fi
	done
	if (( staged_any )); then
		cat > "$PLUGIN_DEST/README.txt" <<'PLUGINREADME'
Hydrogen audio plugins
======================

Copy the bundles into your user plugin folders, then rescan in your DAW:

  Hydrogen.clap  ->  ~/Library/Audio/Plug-Ins/CLAP/
  Hydrogen.vst3  ->  ~/Library/Audio/Plug-Ins/VST3/
  hydrogen.lv2   ->  ~/Library/Audio/Plug-Ins/LV2/   (or another LV2_PATH dir)

The plugin opens Hydrogen's editor as a separate window; keep Hydrogen.app
installed so the plugin can launch it.
PLUGINREADME
	else
		verbose "  (no plugin bundles found in $PLUGIN_DIR)"
	fi
fi

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
