#!/bin/sh

wget https://github.com/libuv/libuv/archive/master.zip
unzip master.zip
cd libuv-master
sh autogen.sh
./configure
make
make check
sudo make install
cd ..
cp libuv-master/include/*.h ./include/
cp libuv-master/.libs/* ./libs


