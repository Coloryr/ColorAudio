sudo apt install flex bison bc libncurses-dev -y

https://snapshots.linaro.org/gnu-toolchain/14.0-2023.06-1/arm-linux-gnueabihf/gcc-linaro-14.0.0-2023.06-x86_64_arm-linux-gnueabihf.tar.xz

# boot

export PATH=$PATH:~/Desktop/t113/arm-none-eabi/bin
git clone https://github.com/szemzoa/awboot
cd awboot
cd tools
gcc mksunxi.c -o mksunxi
cd ../
修改board.h的串口
make -j16

# linux
wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.14.2.tar.xz
tar -xf linux-6.14.2.tar.xz
cd linux-6.14.2
export PATH=$PATH:~/Desktop/t113/arm-none-linux-gnueabihf/bin
make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- sunxi_defconfig
make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- menuconfig
make -j32 ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- zImage dtbs modules

# tina5
sudo apt install -y ack antlr3 asciidoc autoconf automake autopoint binutils bison build-essential \
bzip2 ccache cmake cpio curl device-tree-compiler fastjar flex gawk gettext gcc-multilib g++-multilib \
git gperf haveged help2man intltool libc6-dev-i386 libelf-dev libfuse-dev libglib2.0-dev libgmp3-dev \
libltdl-dev libmpc-dev libmpfr-dev libncurses5-dev libncursesw5-dev libpython3-dev libreadline-dev \
libssl-dev libtool lrzsz mkisofs msmtp ninja-build p7zip p7zip-full patch pkgconf python2.7 python3 \
python3-pyelftools python3-setuptools qemu-utils rsync scons squashfs-tools subversion swig texinfo \
uglifyjs upx-ucl unzip vim wget xmlto xxd zlib1g-dev

sudo -i
curl https://mirrors.bfsu.edu.cn/git/git-repo > /usr/bin/repo
chmod +x /usr/bin/repo
exit
echo export REPO_URL='https://mirrors.bfsu.edu.cn/git/git-repo' >> ~/.bashrc
source ~/.bashrc
git config --global credential.helper store 

mkdir tina-sdk
cd tina-sdk
repo init -u https://sdk.aw-ol.com/git_repo/D1_Tina_Open/manifest.git -b master -m tina-d1-h.xml
repo sync
repo start coloraudio --all
cd ../

git clone https://github.com/DongshanPI/100ASK_T113-PRO_TinaSDK5.git
cd 100ASK_T113-PRO_TinaSDK5
git submodule update --init 
cp ./* -rfvd ../tina-sdk
cd ../

git clone https://github.com/DongshanPI/100ASK_T113-Pro_TinaSDK.git
cd 100ASK_T113-Pro_TinaSDK 
git submodule update --init 
cp ./* -rfvd ../tina-sdk
cd ../

repo forall -vc "git status"
repo forall -vc "git reset --hard"

cd tina-sdk
source build/envsetup.sh
lunch

# buildroot
wget https://buildroot.org/downloads/buildroot-2025.02.tar.gz
tar -zxf buildroot-2025.02.tar.gz
cd buildroot-2025.02
make menuconfig

# xfel
https://github.com/xboot/xfel
xfel version
xfel spinand
xfel spinand erase 0 0x8000000
xfel spinand write 0 awboot-boot-spi.bin
xfel spinand write 0x40000 arch/arm/boot/dts/sun8i-t113s-coloraudio-t113-v1.dtb
xfel spinand write 0x80000 arch/arm/boot/zImage

xfel spinand erase 0x800000 0x7800000
xfel spinand write 0x800000 output/images/rootfs.ubi

