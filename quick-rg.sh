#!/bin/bash

export CROSS_COMPILE=/opt/gcw0-toolchain/usr/bin/mipsel-linux- && \
make -f ./Makefile.rg-350 clean && \
make -f ./Makefile.rg-350 && \
scp ./objs/rg-350/gmenunx root@10.1.1.2:/media/data/local/home && \
echo "
ssh to :: root@10.1.1.2
and run :: 
kill -9 \`pidof -s ash\` && ./gmenunx
or
strace -p\`pidof frontend_start\` -s99999 -e trace= -e write"
