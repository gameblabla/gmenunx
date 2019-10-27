#!/bin/bash

export CROSS_COMPILE=/opt/gcw0-toolchain/usr/bin/mipsel-linux- && \
export DEBUG=5

if [ $# -ne 1 ]; then
make -f ./Makefile.rg-350 clean
	if [ $? -ne 0 ]; then
		echo "make -f ./Makefile.rg-350 clean failed"
		exit 1
	fi
fi

make -f ./Makefile.rg-350
if [ $? -ne 0 ]; then
	echo "make -f ./Makefile.rg-350 failed"
	exit 1
fi

if [ $# -ne 1 ]; then
	make -f ./Makefile.rg-350 dist
	if [ $? -ne 0 ]; then
		echo "make -f ./Makefile.rg-350 dist failed"
		exit 1
	fi
fi

echo "
/*                     *\\

    Make has finished

*\                     */"

printf "runing :: \nscp ./objs/rg-350/gmenunx root@10.1.1.2:/media/data/local/home\n"

tries=0
success=-1
while [[ $success -ne 0 && $tries -lt 10 ]]; 
do
	scp ./objs/rg-350/gmenunx root@10.1.1.2:/media/data/local/home
	success=$?
	tries=$[$tries+1]
done
if [[ $success -ne 0 ]]; then
	printf "Failed to copy the app over. \nEnter terminal mode on the rg350, or move your "
	printf "/usr/local/sbin/frontend_start "
	printf "file for a while\n"
else
	printf "ssh to :: root@10.1.1.2\nand run :: \n"
	printf "kill -9 \`pidof -s ash\` && ./gmenunx\n"
	printf "or\n"
	printf "strace -p\`pidof frontend_start\` -s99999 -e trace= -e write\n"
fi
