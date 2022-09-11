@echo on


pushd sdl
call build_sdl.bat no_pause
popd


pushd zlib
call build_zlib.bat no_pause
popd


pushd ogg-vorbis
call build_ogg-vorbis.bat no_pause
popd


:: Done
pause
