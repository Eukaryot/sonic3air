@echo off

set InputDir=.
set OutputDir=..\..

echo.
echo Creating output directories...
if not exist %OutputDir%\include\imgui          mkdir %OutputDir%\include\imgui
if not exist %OutputDir%\include\imgui\backends mkdir %OutputDir%\include\imgui\backends
if not exist %OutputDir%\lib\x86\imgui          mkdir %OutputDir%\lib\x86\imgui
if not exist %OutputDir%\lib\x86d\imgui         mkdir %OutputDir%\lib\x86d\imgui
if not exist %OutputDir%\lib\x64\imgui          mkdir %OutputDir%\lib\x64\imgui
if not exist %OutputDir%\lib\x64d\imgui         mkdir %OutputDir%\lib\x64d\imgui

echo.
echo Copying includes...
copy %InputDir%\imgui\*.h %OutputDir%\include\imgui
copy %InputDir%\imgui\backends\*.h %OutputDir%\include\imgui\backends

echo.
echo Copying LIBs...
copy %InputDir%\lib\Debug_x86\imgui.lib %OutputDir%\lib\x86d\imgui
copy %InputDir%\lib\Release_x86\imgui.lib %OutputDir%\lib\x86\imgui
copy %InputDir%\lib\Debug_x64\imgui.lib %OutputDir%\lib\x64d\imgui
copy %InputDir%\lib\Release_x64\imgui.lib %OutputDir%\lib\x64\imgui

:: Done
echo.
if "%1"=="" pause
