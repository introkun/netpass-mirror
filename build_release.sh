#!/bin/bash
make clean
make

source version.env

# allows easier upload
cp out/netpass.3dsx out/netpass.3dsx.txt

# strip elf
$DEVKITARM/bin/arm-none-eabi-strip netpass.elf

makerom -f cia -v -target t -exefslogo -o out/netpass.cia \
	-elf out/netpass.elf -rsf meta/netpass.rsf \
	-major $NETPASS_VERSION_MAJOR -minor $NETPASS_VERSION_MINOR -micro $NETPASS_VERSION_MICRO \
	-icon meta/netpass.smdh \
	-banner meta/banner.bnr

cp out/netpass.cia out/netpass.cia.txt