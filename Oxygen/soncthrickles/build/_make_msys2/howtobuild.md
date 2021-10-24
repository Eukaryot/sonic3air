# Building using Make on MSYS2

## Windows
1. Install the following packages using pacman:
```
pacman -Syu
pacman -S make git mingw-w64-i686-gcc mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-minizip-git mingw-w64-x86_64-libswift mingw-w64-x86_64-glew mingw-w64-x86_64-glm mingw-w64-x86_64-libogg mingw-w64-x86_64-opus mingw-w64-x86_64-libvorbis mingw-w64-x86_64-libtheora mingw-w64-x86_64-zlib
```
2. In this directory (Oxygen/soncthrickles/build/_make_msys2):
```
make PLATFORM=Windows
```
Output will be at `bin/Windows/sonic3air.exe`.
