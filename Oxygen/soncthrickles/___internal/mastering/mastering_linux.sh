#!/bin/bash

pushd ../..

DestDir=../_MASTER
OutputDir=$DestDir/sonic3air_game



# Preparations
rm -rf $DestDir
mkdir $DestDir



# Make sure the needed binaries are all up-to-date

svn up ../../librmx
svn up ../../framework/external
svn up ../lemonscript
svn up ../oxygenengine
svn up ../soncthrickles

pushd ./build/_cmake/build
	cmake -DCMAKE_BUILD_TYPE=Release ..
	cmake --build . -j 6
popd



# Build the master installation

cp -r _master_image_template $OutputDir
cp data/images/icon.png $OutputDir/data/icon.png

cp sonic3air_linux $OutputDir
chmod +x $OutputDir/sonic3air_linux
#patchelf --set-rpath '$ORIGIN' $OutputDir/sonic3air_linux		# Not needed any more, see CMAKE_EXE_LINKER_FLAGS in CMakeLists.txt

cp source/external/discord_game_sdk/lib/x86_64/libdiscord_game_sdk.so $OutputDir

cp ___internal/mastering/setup_linux.sh $OutputDir
chmod +x $OutputDir/setup_linux.sh



# Add Oxygen engine

cp -r ../oxygenengine/_master_image_template $OutputDir/bonus/oxygenengine
mkdir $OutputDir/data
cp -r ../oxygenengine/data $OutputDir/bonus/oxygenengine/data

cp ../oxygenengine/oxygenapp_linux $OutputDir/bonus/oxygenengine/
chmod +x $OutputDir/bonus/oxygenengine/oxygenapp_linux
#patchelf --set-rpath '$ORIGIN' $OutputDir/bonus/oxygenengine/oxygenapp_linux



# Complete S3AIR dev

cp -r scripts $OutputDir/bonus/sonic3air_dev/scripts



# Pack everything as a .tar.gz file

pushd $DestDir
	tar -czvf sonic3air_game.tar.gz sonic3air_game
popd



# Done

popd


