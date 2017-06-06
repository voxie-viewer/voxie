#!/bin/bash

set -e

POS="${0%/*}"; test "$POS" = "$0" && POS=.
POS="$(readlink -f -- "$POS")"

cd "$POS/../tools/build-dep"

shopt -s globstar

for URLFILE in **/*.url; do
    FN="${URLFILE%.url}"
    #echo "$FN.sha512sum"
    if [ ! -f "$FN.sha512sum" ]; then
        URL="$(cat "$URLFILE")"
        echo "Downloading $URL"
        curl --fail --progress-bar --location -o "$FN.new" "$URL"
        mv "$FN.new" "$FN"
        sha512sum "$FN" > "$FN.sha512sum.new"
        mv "$FN.sha512sum.new" "$FN.sha512sum"
    fi
done
