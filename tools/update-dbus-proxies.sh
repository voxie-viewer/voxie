#!/bin/sh

set -e

POS="${0%/*}"; test "$POS" = "$0" && POS=.

cd "$POS/../src"

cat ../scripts/de.uni_stuttgart.Voxie.xml |
#sed '/org\.qtproject\.QtDBus\.QtTypeName/s/value=".*"/value="QDBusVariant"/' |
qdbusxml2cpp -p Main/dbusproxies

(
    echo '#include <Voxie/scripting/dbustypes.hpp>'
    cat Main/dbusproxies.h
    rm -f Main/dbusproxies.h
) > Main/dbusproxies.hpp
sed 's,Main/dbusproxies.h,Main/dbusproxies.hpp,' -i Main/dbusproxies.cpp
sed 's,DBUSPROXIES_H_[0-9]*,VOXIE_DBUSPROXIES_H,g' -i Main/dbusproxies.hpp
