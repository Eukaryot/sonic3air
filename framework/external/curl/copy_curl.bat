@echo off

set InputDir=curl
set OutputDir=..\..

echo.
echo Creating output directories...
if not exist %OutputDir%\include\curl mkdir %OutputDir%\include\curl
if not exist %OutputDir%\lib\x86\curl mkdir %OutputDir%\lib\x86\curl
if not exist %OutputDir%\lib\x86d\curl mkdir %OutputDir%\lib\x86d\curl
if not exist %OutputDir%\lib\x64\curl mkdir %OutputDir%\lib\x64\curl
if not exist %OutputDir%\lib\x64d\curl mkdir %OutputDir%\lib\x64d\curl

echo.
echo Copying includes...
copy %InputDir%\include\curl\* %OutputDir%\include\curl

echo.
echo Copying LIBs...
copy "%InputDir%\build\Win32\VC14.30\LIB Release - DLL Windows SSPI\libcurl.lib" %OutputDir%\lib\x86\curl
copy "%InputDir%\build\Win32\VC14.30\LIB Release - DLL Windows SSPI\libcurl.lib" %OutputDir%\lib\x86d\curl
copy "%InputDir%\build\Win64\VC14.30\LIB Release - DLL Windows SSPI\libcurl.lib" %OutputDir%\lib\x64\curl
copy "%InputDir%\build\Win64\VC14.30\LIB Release - DLL Windows SSPI\libcurl.lib" %OutputDir%\lib\x64d\curl

:: Done
echo.
if "%1"=="" pause
