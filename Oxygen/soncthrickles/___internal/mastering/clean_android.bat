@echo off

pushd ..\..\build\_android

:: It seems to be necessary to delete all these directories, to ensure that Android can build after e.g. source files got added
rmdir /s /q .gradle
rmdir /s /q .idea
rmdir /s /q app\.cxx
rmdir /s /q app\build

popd


:: Done
pause
