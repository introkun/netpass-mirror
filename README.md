# NetPass: a new way to experience StreetPass

![qr code](https://gitlab.com/Sorunome/3ds-streetpass/-/raw/release_builds/qr.png)  
Scan QR code to download!

GBAtemp thread: https://gbatemp.net/threads/a-new-way-to-experience-streetpass.653810/

Introducing NetPass: In the current state of the world, the 3DS is, sadly, on decline. That makes getting StreetPass'es harder and harder, due to fewer and fewer people taking their 3DS with them on a walk. This is where NetPass comes in!  
Unlike StreetPass, NetPass works over the internet. Upon opening NetPass, you can pick various locations to go to, i.e. the train station, or a town plaza. Upon entering a location, you get passes of others who are in the same location! And, while you are at the location, others who enter it can get passes with you. But beware! You can only switch locations once every 10 hours!

## Prerequisites

- [devkitPro](https://devkitpro.org/wiki/Getting_Started)

After installing devkitPro, you will need to download the following using [devkitPro pacman](https://devkitpro.org/wiki/devkitPro_pacman) or the devkitPro updater:

- 3ds-dev
- 3ds-curl

In other words, you'll need to run the following command in a Terminal/command prompt (with administrator/root privileges):

```bash
dkp-pacman -S 3ds-dev 3ds-curl
```

In order to create the `.CIA` file you need `makerom` executable on your `$PATH`
You can get it precompilated on https://github.com/3DSGuy/Project_CTR/releases and then copy it to `$DEVKITPRO/tools/bin` 

## Compilation

This project ships with a [Makefile](Makefile), which is meant to simplify the compilation process. If you're unfamiliar with them, you can find out more about GNU Make [here](https://www.gnu.org/software/make/).

```bash
make
```

To build `.cia` files, run

```bash
./build_release.sh
```