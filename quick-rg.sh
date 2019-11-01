#!/bin/bash

export CROSS_COMPILE=/opt/gcw0-toolchain/usr/bin/mipsel-linux-
export DEBUG=5

RG350_HOME="/media/data/local/home"
LAUNCHER="/usr/local/sbin/frontend_start"
RG350_IP="root@10.1.1.2"
TARGET_DIR="gmenunx-beta"

# probably don't edit below here

rsync=`which rsync`
if [ $? -ne 0 ]; then
	echo "This script requires rsync"
	exit 1
fi
scp=`which scp`
if [ $? -ne 0 ]; then
	echo "This script requires scp"
	exit 1
fi

function launchLinkExists {
	echo "checking if a launchLink exists"
	cmd="ssh ${RG350_IP} /bin/busybox sh -c \"if [ -L ${LAUNCHER} ]; then echo \"EXISTS\"; fi\""
	result=`${cmd}`
	if [ "EXISTS" == ${result} ]; then
		return 1
	else
		return 0
	fi
}

function removeLaunchLink {
	echo "removing launch link"
	cmd="ssh ${RG350_IP} /bin/busybox sh -c \"if [ -L ${LAUNCHER} ]; then rm -rf ${LAUNCHER}; fi\""
	${cmd}
	if [ $? -eq 0 ]; then
		echo "removed launcher link successfully"
	else
		echo "failed to remove launch link, quitting"
		exit 1
	fi
}

function installLaunchLink {
	echo "installing launch link"
	cmd="ssh ${RG350_IP} /bin/busybox sh -c \"ln -fs ${RG350_HOME}/${TARGET_DIR}/gmenunx ${LAUNCHER}\""
	echo ${cmd}
	${cmd}
	if [ $? -eq 0 ]; then
		echo "installed launcher link successfully"
	else
		echo "failed to install launch link, quitting"
		exit 1
	fi
}

function fullDeploy {
	echo "doing a full deploy from target folder to rg-350"
	cmd="rsync -r ./dist/rg-350/gmenunx/ ${RG350_IP}:${RG350_HOME}/${TARGET_DIR}"
	echo ${cmd}
	${cmd}
	if [ $? -ne 0 ]; then
		echo "full deployment failed"
		exit 1
	fi
	return $?
}

function binaryDeploy {
	echo "doing a quick binary only deploy"
	cmd="scp ./objs/rg-350/gmenunx ${RG350_IP}:${RG350_HOME}/${TARGET_DIR}"
	echo ${cmd}
	${cmd}
	return $?
}

function showHelp {
      echo "Usage:"
      echo "    -h                 Display this help message."
      echo "    -f                 Full clean, build, deploy, enable."
	  echo "    -q                 Quick, build, deploy binary only."
	  echo "    -i                 install"
	  echo "    -u                 uninstall"
}

function myMake {
	args="$*"
	cmd="make -f ./Makefile.rg-350 ${args}"
	echo "make command : ${cmd}"
	${cmd}
	if [ $? -ne 0 ]; then
		echo "make failed"
		exit 1
	fi
}

if [ $# -eq 0 ]; then
	showHelp
	exit 0
fi

while getopts ":hfqiu" opt; do

  case ${opt} in
	h )
		showHelp
		exit 0
      ;;
    f )
		myMake "clean all dist"
		fullDeploy
		installLaunchLink
		exit 0
      ;;
    q )
		myMake "all"
		binaryDeploy
		exit 0
      ;;
    i )
		installLaunchLink
		exit 0
      ;;
    u )
		removeLaunchLink
		exit 0
      ;;
    \? )
      showHelp
      ;;
    : )
      echo "Invalid option: $OPTARG requires an argument" 1>&2
	  showHelp
      ;;
  esac
done
shift $((OPTIND -1))
exit 0


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
