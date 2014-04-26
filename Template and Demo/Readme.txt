This demo will produce a ~22MHz clock signal on the SCLK line (GPIO 11).
The kernel.img should work fine on its own as a test subject (along with the contents of the disk folder). Use that image to test that the RPi setup is working.
Then try to compile the provided main.c using the provided make.sh and try again with the new kernel.img. This will ensure your cross compiling toolchain is setup correctly.

From there the possibilities are endless!
Muahahaha....
