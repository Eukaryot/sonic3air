@echo off

set InputDir=libogg
set OutputDir=..\..

echo.
echo Creating output directories...
if not exist %OutputDir%\include\ogg     mkdir %OutputDir%\include\ogg
if not exist %OutputDir%\lib\x86\libogg  mkdir %OutputDir%\lib\x86\libogg
if not exist %OutputDir%\lib\x86d\libogg mkdir %OutputDir%\lib\x86d\libogg
if not exist %OutputDir%\lib\x64\libogg  mkdir %OutputDir%\lib\x64\libogg
if not exist %OutputDir%\lib\x64d\libogg mkdir %OutputDir%\lib\x64d\libogg
if not exist %OutputDir%\bin\x86         mkdir %OutputDir%\bin\x86
if not exist %OutputDir%\bin\x86d        mkdir %OutputDir%\bin\x86d
if not exist %OutputDir%\bin\x64         mkdir %OutputDir%\bin\x64
if not exist %OutputDir%\bin\x64d        mkdir %OutputDir%\bin\x64d

echo.
echo Copying includes...
copy %InputDir%\include\ogg\* %OutputDir%\include\ogg

echo.
echo Copying LIBs...
copy "%InputDir%\win32\VS2015\Win32\Debug\libogg.lib" %OutputDir%\lib\x86d\libogg
copy "%InputDir%\win32\VS2015\Win32\Release\libogg.lib" %OutputDir%\lib\x86\libogg
copy "%InputDir%\win32\VS2015\x64\Debug\libogg.lib" %OutputDir%\lib\x64d\libogg
copy "%InputDir%\win32\VS2015\x64\Release\libogg.lib" %OutputDir%\lib\x64\libogg

rem -- Static build, i.e. no DLLs to copy

echo.
pause
