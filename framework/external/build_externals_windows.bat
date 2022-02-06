@echo on

:: You might need to change this path to point to your Visual Studio installation's MSBuild.exe
set msbuildPath="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"



:: Build SDL 2

pushd sdl

pushd SDL2\VisualC
%msbuildPath% SDL.sln /target:SDL2     /property:Configuration=Debug   /property:Platform=Win32 -verbosity:minimal
%msbuildPath% SDL.sln /target:SDL2main /property:Configuration=Debug   /property:Platform=Win32 -verbosity:minimal
%msbuildPath% SDL.sln /target:SDL2     /property:Configuration=Release /property:Platform=Win32 -verbosity:minimal
%msbuildPath% SDL.sln /target:SDL2main /property:Configuration=Release /property:Platform=Win32 -verbosity:minimal
%msbuildPath% SDL.sln /target:SDL2     /property:Configuration=Debug   /property:Platform=x64   -verbosity:minimal
%msbuildPath% SDL.sln /target:SDL2main /property:Configuration=Debug   /property:Platform=x64   -verbosity:minimal
%msbuildPath% SDL.sln /target:SDL2     /property:Configuration=Release /property:Platform=x64   -verbosity:minimal
%msbuildPath% SDL.sln /target:SDL2main /property:Configuration=Release /property:Platform=x64   -verbosity:minimal
popd

call copy_sdl.bat no_pause

popd



:: Build zlib + minizip

pushd zlib

pushd _vstudio
%msbuildPath% zlib.sln /target:zlib    /property:Configuration=Debug   /property:Platform=x86 -verbosity:minimal
%msbuildPath% zlib.sln /target:minizip /property:Configuration=Debug   /property:Platform=x86 -verbosity:minimal
%msbuildPath% zlib.sln /target:zlib    /property:Configuration=Release /property:Platform=x86 -verbosity:minimal
%msbuildPath% zlib.sln /target:minizip /property:Configuration=Release /property:Platform=x86 -verbosity:minimal
%msbuildPath% zlib.sln /target:zlib    /property:Configuration=Debug   /property:Platform=x64 -verbosity:minimal
%msbuildPath% zlib.sln /target:minizip /property:Configuration=Debug   /property:Platform=x64 -verbosity:minimal
%msbuildPath% zlib.sln /target:zlib    /property:Configuration=Release /property:Platform=x64 -verbosity:minimal
%msbuildPath% zlib.sln /target:minizip /property:Configuration=Release /property:Platform=x64 -verbosity:minimal
popd

call copy_zlib.bat no_pause

popd



:: Build libogg + libvorbis

pushd ogg-vorbis

pushd libogg\win32\VS2015
%msbuildPath% libogg.sln /target:libogg /property:Configuration=Debug /property:Platform=Win32 -verbosity:minimal
%msbuildPath% libogg.sln /target:libogg /property:Configuration=Release /property:Platform=Win32 -verbosity:minimal
%msbuildPath% libogg.sln /target:libogg /property:Configuration=Debug /property:Platform=x64 -verbosity:minimal
%msbuildPath% libogg.sln /target:libogg /property:Configuration=Release /property:Platform=x64 -verbosity:minimal
popd

pushd libvorbis\win32\VS2010
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
popd

call copy_libogg.bat no_pause
call copy_libvorbis.bat no_pause

popd



:: Done
pause
