#!/bin/sh

set -e

POS="${0%/*}"; test "$POS" = "$0" && POS=.

target="${0##*/}"; target="${target%.update.sh}"

if [ "$#" != 1 ]; then
    echo "Usage: $0 <git repository of ${target##*/}>"
    exit 1
fi

origin="$(readlink -f -v -- "$1")"

cd "$POS"; POS=.

rm -rf "$target"
git rm -r --quiet --ignore-unmatch "$target"
git --git-dir="$origin/.git" archive "$(jq --raw-output '.["GitCommit"]' "$target".sw.json)" --prefix "$target"/ | tar x
git add -f "$target"
