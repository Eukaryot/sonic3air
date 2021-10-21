@echo off

set InputDir=libvorbis
set OutputDir=..\..

echo.
echo Creating output directories...
if not exist %OutputDir%\include\vorbis     mkdir %OutputDir%\include\vorbis
if not exist %OutputDir%\lib\x86\libvorbis  mkdir %OutputDir%\lib\x86\libvorbis
if not exist %OutputDir%\lib\x86d\libvorbis mkdir %OutputDir%\lib\x86d\libvorbis
if not exist %OutputDir%\lib\x64\libvorbis  mkdir %OutputDir%\lib\x64\libvorbis
if not exist %OutputDir%\lib\x64d\libvorbis mkdir %OutputDir%\lib\x64d\libvorbis
if not exist %OutputDir%\bin\x86            mkdir %OutputDir%\bin\x86
if not exist %OutputDir%\bin\x86d           mkdir %OutputDir%\bin\x86d
if not exist %OutputDir%\bin\x64            mkdir %OutputDir%\bin\x64
if not exist %OutputDir%\bin\x64d           mkdir %OutputDir%\bin\x64d

echo.
echo Copying includes...
copy %InputDir%\include\vorbis\* %OutputDir%\include\vorbis

echo.
echo Copying LIBs...
copy "%InputDir%\win32\VS2010\Win32\Debug\*.lib" %OutputDir%\lib\x86d\libvorbis
copy "%InputDir%\win32\VS2010\Win32\Release\*.lib" %OutputDir%\lib\x86\libvorbis
copy "%InputDir%\win32\VS2010\x64\Debug\*.lib" %OutputDir%\lib\x64d\libvorbis
copy "%InputDir%\win32\VS2010\x64\Release\*.lib" %OutputDir%\lib\x64\libvorbis

echo.
pause
