#!/bin/sh

#
# written by Magnus Oman to solve the libcrypto dependency issue
#

if test $(echo $0 | grep "\.app/Contents/MacOS/Hydrogen"); then
  HYDROGEN_DIR="$(dirname "$0")"
else
  HYDROGEN_DIR="$PWD"
fi

export DYLD_FALLBACK_LIBRARY_PATH="$DYLD_FALLBACK_LIBRARY_PATH":"$HYDROGEN_DIR/../Frameworks"
exec "$HYDROGEN_DIR/Hydrogen-bin" "$@"
