@echo off

set InputDir=SDL2
set OutputDir=..\..

echo.
echo Creating output directories...
if not exist %OutputDir%\include\sdl  mkdir %OutputDir%\include\sdl
if not exist %OutputDir%\lib\x86\sdl  mkdir %OutputDir%\lib\x86\sdl
if not exist %OutputDir%\lib\x86d\sdl mkdir %OutputDir%\lib\x86d\sdl
if not exist %OutputDir%\lib\x64\sdl  mkdir %OutputDir%\lib\x64\sdl
if not exist %OutputDir%\lib\x64d\sdl mkdir %OutputDir%\lib\x64d\sdl

echo.
echo Copying includes...
copy %InputDir%\include\* %OutputDir%\include\sdl

echo.
echo Copying LIBs...
copy %InputDir%\VisualC\Win32\Debug\sdl2.lib %OutputDir%\lib\x86d\sdl
copy %InputDir%\VisualC\Win32\Release\sdl2.lib %OutputDir%\lib\x86\sdl
copy %InputDir%\VisualC\Win32\Debug\sdl2main.lib %OutputDir%\lib\x86d\sdl
copy %InputDir%\VisualC\Win32\Release\sdl2main.lib %OutputDir%\lib\x86\sdl
copy %InputDir%\VisualC\x64\Debug\sdl2.lib %OutputDir%\lib\x64d\sdl
copy %InputDir%\VisualC\x64\Release\sdl2.lib %OutputDir%\lib\x64\sdl
copy %InputDir%\VisualC\x64\Debug\sdl2main.lib %OutputDir%\lib\x64d\sdl
copy %InputDir%\VisualC\x64\Release\sdl2main.lib %OutputDir%\lib\x64\sdl

:: Done
echo.
if "%1"=="" pause
