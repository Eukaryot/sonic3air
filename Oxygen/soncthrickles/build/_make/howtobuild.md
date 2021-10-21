# Building using Make

## Nintendo Switch
1. [Follow the instructions from devkitPro to get your build environment set up.](https://devkitpro.org/wiki/Getting_Started#Setup)
2. Install the following packages using pacman:
```
sudo (dkp-)pacman -Syu
sudo (dkp-)pacman -S switch-pkg-config devkitA64 switch-tools switch-sdl2 switch-glad switch-glm switch-libogg switch-libopus switch-libvorbis switch-libtheora
```
3. In this directory (Oxygen/soncthrickles/build/_make):
```
make PLATFORM=Switch
```
Output will be at `bin/Switch/sonic3air.nro`.