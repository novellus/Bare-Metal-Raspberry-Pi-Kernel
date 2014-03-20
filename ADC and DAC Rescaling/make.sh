arm-none-eabi-gcc -O3 -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -nostartfiles -g main.c -o kernel.elf
arm-none-eabi-objcopy kernel.elf -O binary kernel.img