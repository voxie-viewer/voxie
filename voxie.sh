#!/bin/sh

set -e

POS="${0%/*}"; test "$POS" = "$0" && POS=.
POS="$(readlink -f -- "$POS")"

: "${VOXIE_BUILD_DIR:=$POS/build}"
export VOXIE_BUILD_DIR

if [ "$1" != "${1#--build-dir=}" ]; then
    VOXIE_BUILD_DIR="${1#--build-dir=}"
    shift
fi

EXE="$VOXIE_BUILD_DIR/Main/voxie"

# If the first parameter is --build, try to build voxie
if [ "$1" = "--build" ]; then
    shift
    echo "Building voxie..."
    "$POS/tools/build.sh"
    echo "Building voxie finished."
elif [ ! -f "$EXE" ]; then
    echo "Could not find $EXE" >&2
    echo "Run 'tools/build.sh' or './voxie.sh --build' to build voxie" >&2
    exit 1
fi

GDB=
if [ "$1" = "--gdb" ]; then
    GDB="gdb --args"
    shift
elif [ "$1" = "--valgrind" ]; then
    GDB="valgrind"
    shift
elif [ "$1" != "${1#--valgrind=}" ]; then
    GDB="valgrind ${1#--valgrind=}"
    shift
fi

# Add Voxie directory to LD_LIBRARY_PATH
export LD_LIBRARY_PATH="$VOXIE_BUILD_DIR/Voxie${LD_LIBRARY_PATH+:$LD_LIBRARY_PATH}"

# Add all subdirectories of $VOXIE_BUILD_DIR starting with Plugin to VOXIE_PLUGIN_PATH
for i in "$VOXIE_BUILD_DIR"/Plugin*/; do
    export VOXIE_PLUGIN_PATH="${VOXIE_PLUGIN_PATH+$VOXIE_PLUGIN_PATH:}${i%/}"
done

# Add all subdirectories of $VOXIE_BUILD_DIR starting with Script or Ext to VOXIE_SCRIPT_PATH
for i in "$VOXIE_BUILD_DIR"/Script*/ "$VOXIE_BUILD_DIR"/Ext*/; do
    export VOXIE_SCRIPT_PATH="${VOXIE_SCRIPT_PATH+$VOXIE_SCRIPT_PATH:}${i%/}"
done

# Add ./scripts to $VOXIE_SCRIPT_PATH
export VOXIE_SCRIPT_PATH="${VOXIE_SCRIPT_PATH+$VOXIE_SCRIPT_PATH:}$POS/scripts"

# Set VOXIE_DIR
export VOXIE_DIR="$POS"

# Set VOXIE_MANUAL_FILE
export VOXIE_MANUAL_FILE="$VOXIE_BUILD_DIR/extra/manual.pdf"

# Run voxie
exec $GDB "$EXE" "$@"
