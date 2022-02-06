@echo off

set InputDir=.
set OutputDir=..\..

echo.
echo Creating output directories...
if not exist %OutputDir%\include\zlib  mkdir %OutputDir%\include\zlib
if not exist %OutputDir%\lib\x86\zlib  mkdir %OutputDir%\lib\x86\zlib
if not exist %OutputDir%\lib\x86d\zlib mkdir %OutputDir%\lib\x86d\zlib
if not exist %OutputDir%\lib\x64\zlib  mkdir %OutputDir%\lib\x64\zlib
if not exist %OutputDir%\lib\x64d\zlib mkdir %OutputDir%\lib\x64d\zlib
if not exist %OutputDir%\include\minizip  mkdir %OutputDir%\include\minizip
if not exist %OutputDir%\lib\x86\minizip  mkdir %OutputDir%\lib\x86\minizip
if not exist %OutputDir%\lib\x86d\minizip mkdir %OutputDir%\lib\x86d\minizip
if not exist %OutputDir%\lib\x64\minizip  mkdir %OutputDir%\lib\x64\minizip
if not exist %OutputDir%\lib\x64d\minizip mkdir %OutputDir%\lib\x64d\minizip

echo.
echo Copying includes...
copy %InputDir%\zlib\*.h %OutputDir%\include\zlib
copy %InputDir%\zlib\contrib\minizip\*.h %OutputDir%\include\minizip

echo.
echo Copying LIBs...
copy %InputDir%\lib\Debug_x86\zlib.lib %OutputDir%\lib\x86d\zlib
copy %InputDir%\lib\Release_x86\zlib.lib %OutputDir%\lib\x86\zlib
copy %InputDir%\lib\Debug_x64\zlib.lib %OutputDir%\lib\x64d\zlib
copy %InputDir%\lib\Release_x64\zlib.lib %OutputDir%\lib\x64\zlib
copy %InputDir%\lib\Debug_x86\minizip.lib %OutputDir%\lib\x86d\minizip
copy %InputDir%\lib\Release_x86\minizip.lib %OutputDir%\lib\x86\minizip
copy %InputDir%\lib\Debug_x64\minizip.lib %OutputDir%\lib\x64d\minizip
copy %InputDir%\lib\Release_x64\minizip.lib %OutputDir%\lib\x64\minizip

:: Done
echo.
if "%1"=="" pause
