sudo apt install build-essential wget flex bison bc libncurses-dev git unzip -y

# http proxy

export http="http://192.168.201.1:1082"
export https="http://192.168.201.1:1082"
export http_proxy="http://192.168.201.1:1082"
export https_proxy="http://192.168.201.1:1082"

# tool

wget https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz
tar -xf arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz
mv arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi arm-none-eabi

wget https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz
tar -xf arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz
mv arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-linux-gnueabihf arm-none-linux-gnueabihf

export PATH=$PATH:~/arm-none-eabi/bin:~/arm-none-linux-gnueabihf/bin

# awboot

git clone https://github.com/szemzoa/awboot
cd awboot/tools
gcc mksunxi.c -o mksunxi
cd ../

git apply ../patchs/awboot.patch

make -j32

cd ../

#linux

wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.14.6.tar.xz
tar -xf linux-6.14.6.tar.xz

cp ./dts/sun8i-t113s-coloraudio-t113-v1.dts ./linux-6.14.2/arch/arm/boot/dts/allwinner
rm ./linux-6.14.2/arch/arm/boot/dts/allwinner/Makefile
cp ./patchs/dts/Makefile ./linux-6.14.2/arch/arm/boot/dts/allwinner

cd linux-6.14.6
make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- sunxi_defconfig
make -j32 ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- zImage dtbs modules
cd ../

# buildroot

wget https://buildroot.org/downloads/buildroot-2025.02.tar.gz
tar -zxf buildroot-2025.02.tar.gz
cd buildroot-2025.02
make menuconfig