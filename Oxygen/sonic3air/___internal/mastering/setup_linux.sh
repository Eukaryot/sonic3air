#!/bin/bash

# Escape spaces (needed for the Exec part below, but for some reason not for Icon)
basepath1="$PWD"
basepath2="${PWD// /\\ }"
outpath=${XDG_DATA_HOME:-$HOME/.local/share}/applications/sonic3air.desktop

if [ "$1" = "--remove" ]; then

rm $outpath

else

# Build .desktop launcher file
/bin/cat <<EOM > "$outpath"
[Desktop Entry]
Name=Sonic 3 A.I.R.
Type=Application
Encoding=UTF-8
Exec=$basepath2/sonic3air_linux
Icon=$basepath1/data/icon.png
StartupWMClass=sonic3air_linux
Terminal=false
Categories=Game;
EOM

# Set executable flag
chmod +x $outpath

fi

