#!/bin/bash

pushd ../..

DestDir=../_MASTER
OutputDir=$DestDir/sonic3air_game



# Preparations

echo
echo Recreating output directory: $DestDir
rm -rf $DestDir
mkdir $DestDir

# If using SVN, update the repos first
if [ -d "../sonic3air/.svn" ]; then
	svn up ../../librmx
	svn up ../../framework/external
	svn up ../lemonscript
	svn up ../oxygenengine
	svn up .
fi



# Make sure the needed binaries are all up-to-date

echo
echo CMake build
pushd ./build/_cmake/build
	cmake -DCMAKE_BUILD_TYPE=Release ..
	cmake --build . -j 6
popd



# Build data packages

echo
echo Rebuilding packages in master image template

# Update auto-generated C++ script binding reference and run script nativization
./sonic3air_linux -dumpcppdefinitions -nativize

# Build data packages and meta data
./sonic3air_linux -pack
mv "enginedata.bin" "_master_image_template/data"
mv "gamedata.bin" "_master_image_template/data"
mv "audiodata.bin" "_master_image_template/data"
mv "audioremaster.bin" "_master_image_template/data"
cp -r "data/metadata.json" "_master_image_template/data"

# Copy scripts
cp "saves/scripts.bin" "_master_image_template/data"



# Build the master installation

echo
echo Building sonic3air_game installation

cp -r _master_image_template $OutputDir
cp data/images/icon.png $OutputDir/data/icon.png

cp sonic3air_linux $OutputDir
chmod +x $OutputDir/sonic3air_linux
#patchelf --set-rpath '$ORIGIN' $OutputDir/sonic3air_linux		# Not needed any more, see CMAKE_EXE_LINKER_FLAGS in CMakeLists.txt

cp source/external/discord_game_sdk/lib/x86_64/libdiscord_game_sdk.so $OutputDir

# Leave out the remastered music, as the Linux build can use an internal downloader
rm $OutputDir/data/audioremaster.bin

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

echo
echo Packing as tar.gz

pushd $DestDir
	tar -czvf sonic3air_game.tar.gz sonic3air_game
popd



# Done

popd


