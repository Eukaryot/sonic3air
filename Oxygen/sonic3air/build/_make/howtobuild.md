# Building using Make

**Warning: The Switch build is currently not maintained and may need a manual update of the makefile!**
**This Makefile was last updated for stable version 26.03.28.0 and should be compiled using the lastest version of DevkitPro**

## Nintendo Switch
1. [Follow the instructions from devkitPro to get your build environment set up.](https://devkitpro.org/wiki/Getting_Started#Setup)
2. Install the following packages using pacman:

On Linux:
```
sudo (dkp-)pacman -Syu
sudo (dkp-)pacman -S switch-pkg-config devkitA64 switch-tools switch-sdl2 switch-glad switch-glm switch-libogg switch-libopus switch-libvorbis switch-libtheora
```
On Windows (10/11):
```
pacman -Syu
pacman -S switch-pkg-config devkitA64 switch-tools switch-sdl2 switch-glad switch-glm switch-libogg switch-libopus switch-libvorbis switch-libtheora
```

3. In this directory (Oxygen/sonic3air/build/_make):
```
make PLATFORM=Switch
```
Output will be at `bin/Switch/sonic3air.nro`.
