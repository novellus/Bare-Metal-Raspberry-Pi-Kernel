Bare-Metal-Raspberry-Pi-Kernel
==============================

A bare metal kernel for the Raspberry Pi, written in C.

Each folder, except "disk", is a different kernel.
Complie main.c with the make.sh file; you will need an appropriate cross compiler (https://launchpad.net/gcc-arm-embedded).

Copy the resulting kernel.img, and the contents of the disk folder (minus the liscense file), onto a blank SD-Card.
Pop the card into the RPI, and go.

In explanation of the magic "gpio" number used in many of the kernels, the BCM2835 (RPI chipset) documentation is provided here. Specifically check out page 90+, and remember that address 0x7E200000 in that document is mapped to the physical address 0x20200000.
https://drive.google.com/file/d/0Byd6ngUnOQEeTXdxMkE0Mkx6Rms/edit?usp=sharing
