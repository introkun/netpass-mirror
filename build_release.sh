#!/bin/bash
make clean
make

VERSION_MAJOR=0
VERSION_MINOR=1
VERSION_MICRO=1

# allows easier upload
cp netpass.3dsx netpass.3dsx.txt

# strip elf
arm-none-eabi-strip netpass.elf

../makerom -f cia -v -target t -exefslogo -o netpass.cia \
	-elf netpass.elf -rsf netpass.rsf \
	-major 0 -minor 1 -micro 0 \
	-icon netpass.smdh

cp netpass.cia netpass.cia.txt