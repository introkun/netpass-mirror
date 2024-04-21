#!/bin/bash
make clean
make

source version.env

# allows easier upload
cp netpass.3dsx netpass.3dsx.txt

# strip elf
$DEVKITARM/bin/arm-none-eabi-strip netpass.elf

makerom -f cia -v -target t -exefslogo -o netpass.cia \
	-elf netpass.elf -rsf netpass.rsf \
	-major $NETPASS_VERSION_MAJOR -minor $NETPASS_VERSION_MINOR -micro $NETPASS_VERSION_MICRO \
	-icon netpass.smdh

cp netpass.cia netpass.cia.txt