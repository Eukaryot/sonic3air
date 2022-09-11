@echo on

call ../get_msbuild_path.bat


:: Build libogg + libvorbis
:: This is built with /MT flag, by setting the _CL_ environment variable, which enforces usage of non-DLL Release Runtime Library.
::  -> This way, the VS projects don't necessarily need that set manually in the vcxproj before building.

@echo.
@echo.
@echo === Building libbogg + libvorbis ===

pushd libogg\win32\VS2015
set _CL_=/MT
%msbuildPath% libogg.sln /target:libogg /property:Configuration=Debug /property:Platform=Win32 -verbosity:minimal
%msbuildPath% libogg.sln /target:libogg /property:Configuration=Release /property:Platform=Win32 -verbosity:minimal
%msbuildPath% libogg.sln /target:libogg /property:Configuration=Debug /property:Platform=x64 -verbosity:minimal
%msbuildPath% libogg.sln /target:libogg /property:Configuration=Release /property:Platform=x64 -verbosity:minimal
set _CL_=
popd

pushd libvorbis\win32\VS2010
set _CL_=/MT
%msbuildPath% vorbis_static.sln /target:libvorbis_static /property:Configuration=Debug   /property:Platform=Win32 -verbosity:minimal
%msbuildPath% vorbis_static.sln /target:libvorbisfile    /property:Configuration=Debug   /property:Platform=Win32 -verbosity:minimal
%msbuildPath% vorbis_static.sln /target:vorbisdec        /property:Configuration=Debug   /property:Platform=Win32 -verbosity:minimal
%msbuildPath% vorbis_static.sln /target:libvorbis_static /property:Configuration=Release /property:Platform=Win32 -verbosity:minimal
%msbuildPath% vorbis_static.sln /target:libvorbisfile    /property:Configuration=Release /property:Platform=Win32 -verbosity:minimal
%msbuildPath% vorbis_static.sln /target:vorbisdec        /property:Configuration=Release /property:Platform=Win32 -verbosity:minimal
%msbuildPath% vorbis_static.sln /target:libvorbis_static /property:Configuration=Debug   /property:Platform=x64   -verbosity:minimal
%msbuildPath% vorbis_static.sln /target:libvorbisfile    /property:Configuration=Debug   /property:Platform=x64   -verbosity:minimal
%msbuildPath% vorbis_static.sln /target:vorbisdec        /property:Configuration=Debug   /property:Platform=x64   -verbosity:minimal
%msbuildPath% vorbis_static.sln /target:libvorbis_static /property:Configuration=Release /property:Platform=x64   -verbosity:minimal
%msbuildPath% vorbis_static.sln /target:libvorbisfile    /property:Configuration=Release /property:Platform=x64   -verbosity:minimal
%msbuildPath% vorbis_static.sln /target:vorbisdec        /property:Configuration=Release /property:Platform=x64   -verbosity:minimal
set _CL_=
popd

call copy_libogg.bat no_pause
call copy_libvorbis.bat no_pause


:: Done
echo.
if "%1"=="" pause
