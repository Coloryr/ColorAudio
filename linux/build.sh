wget https://www.python.org/ftp/python/2.7.18/Python-2.7.18.tgz
tar xf Python-2.7.18.tgz
cd Python-2.7.18
sudo apt-get install -y libsqlite3-dev
./configure --enable-optimizations
sudo make install -j32

sudo apt-get install -y git gnupg flex bison gperf build-essential zip curl libc6-dev x11proto-core-dev g++-multilib tofrodos markdown libxml2-utils xsltproc gawk texinfo build-essential gcc libncurses5-dev bison flex zlib1g-dev gettext libssl-dev autoconf libtool wget patch dos2unix tree u-boot-tools libelf-dev lz4 libgmp-dev libmpc-dev expect expect-dev cpio

sudo apt-get install -y universal-ctags vim