@echo on

call ../get_msbuild_path.bat


:: Build SDL2
:: This is built with /MT flag, by setting the _CL_ environment variable, which enforces usage of non-DLL Release Runtime Library.
::  -> This way, the VS projects don't necessarily need that set manually in the vcxproj before building.

@echo.
@echo.
@echo === Building SDL2 ===

pushd SDL2\VisualC
set _CL_=/MT
%msbuildPath% SDL.sln /target:SDL2     /property:Configuration=Debug   /property:Platform=Win32 -verbosity:minimal
%msbuildPath% SDL.sln /target:SDL2main /property:Configuration=Debug   /property:Platform=Win32 -verbosity:minimal
%msbuildPath% SDL.sln /target:SDL2     /property:Configuration=Release /property:Platform=Win32 -verbosity:minimal
%msbuildPath% SDL.sln /target:SDL2main /property:Configuration=Release /property:Platform=Win32 -verbosity:minimal
%msbuildPath% SDL.sln /target:SDL2     /property:Configuration=Debug   /property:Platform=x64   -verbosity:minimal
%msbuildPath% SDL.sln /target:SDL2main /property:Configuration=Debug   /property:Platform=x64   -verbosity:minimal
%msbuildPath% SDL.sln /target:SDL2     /property:Configuration=Release /property:Platform=x64   -verbosity:minimal
%msbuildPath% SDL.sln /target:SDL2main /property:Configuration=Release /property:Platform=x64   -verbosity:minimal
set _CL_=
popd

call copy_sdl.bat no_pause


:: Done
echo.
if "%1"=="" pause
