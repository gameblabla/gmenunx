#!/bin/bash

APP_NAME="esoteric"
RG350_HOME="/media/data/local/home"
LAUNCHER="/usr/local/sbin/frontend_start"
RG350_IP="root@10.1.1.2"
TARGET_DIR="esoteric-beta"
BRANCH_NAME="master"
MAJOR_VERSION=1
MINOR_VERSION=0

# probably don't edit below here

function log () {
	if [ ${verbose} == "true" ]; then
		echo $1
	fi
}

function checkDependencies() {
	local cont="true";
	log "checking we have all our dependencies..."

	for dep in "curl" "rsync" "scp" "git"; do
		log "checking for : '${dep}'"
		local installed=`command -v ${dep}`
		if [[ -z ${installed} ]]; then
			echo "This script requires ${dep}"
			cont="false"
		else
			log "${dep} is installed at : ${installed}"
		fi
	done;

	if [ ${cont} != "true" ]; then
		echo "dependencies not all found, stopping"
		exit 1
	fi
	log "dependencies all found"
}

# call this psssing in the semver value
function tagRepo () {
	local semver=$1
	local build_number=${BRANCH_NAME}-${semver}
	log "tagging the repo with : ${build_number}"
	$(git tag -a ${build_number} $BRANCH_NAME -m "Version $semver")
	$(git push origin ${build_number} $BRANCH_NAME)
}

# call this using : incrementSemVer result_var current_semver
function incrementSemVer () {

	if [ $# -ne 2 ]; then
		echo "this function needs a ref var passed in for the result, and the current semver"
		exit 2
	fi
	local version=$2
	local char="."
	local count=$(awk -F"${char}" '{print NF-1}' <<< "${version}")

	if (( ${count} == 2 )); then
		major=$(echo $version | cut -d. -f1)
		minor=$(echo $version | cut -d. -f2)
		patch=$(echo $version | cut -d. -f3)
		let patch=($patch+1)
	else
		echo "invalid semver : ${version}"
		exit 2
	fi

	if [ "$major" -ne "${MAJOR_VERSION}" ] || [ "$minor" -ne "${MINOR_VERSION}" ]; then
		major="${MAJOR_VERSION}"
		minor="${MINOR_VERSION}"
		patch=0
	fi

	local semver="${major}.${minor}.${patch}"
	log "incremented semver : ${semver}"
	eval $1="${semver}"

}

# call this using : getSemVer result_var
function getSemVer () {

	if [ $# -eq 0 ]; then
		echo "this function needs a ref var passed in for the result"
		exit 2
	fi
	$(git fetch --tags)
	local last_tag=$(git describe --abbrev=0 --tags)
	local major=${MAJOR_VERSION}
	local minor=${MINOR_VERSION}
	local patch=-1

	if [[ $? -eq 0 ]]; then
		log "last tag on server : ${last_tag}"
		local version=$(echo ${last_tag} | grep -o '[^-]*$')
		#log "raw version : ${version}"
		local char="."
		local count=$(awk -F"${char}" '{print NF-1}' <<< "${version}")
		
		if (( ${count} == 2 )); then
			log "well formatted previous version tag found"
			major=$(echo ${version} | cut -d. -f1)
			minor=$(echo ${version} | cut -d. -f2)
			patch=$(echo ${version} | cut -d. -f3)
		fi
	fi

	local semver="${major}.${minor}.${patch}"
	log "current version: ${semver}"
	eval $1="${semver}"

}

function launchLinkExists {
	log "checking if a launchLink exists"
	local cmd="ssh ${RG350_IP} /bin/busybox sh -c \"if [ -L ${LAUNCHER} ]; then echo \"EXISTS\"; fi\""
	local result=`${cmd}`
	if [ "EXISTS" == ${result} ]; then
		return 1
	else
		return 0
	fi
}

function removeLaunchLink {
	log "removing launch link"
	local cmd="ssh ${RG350_IP} /bin/busybox sh -c \"if [ -L ${LAUNCHER} ]; then rm -rf ${LAUNCHER}; fi\""
	${cmd}
	if [ $? -eq 0 ]; then
		echo "removed launcher link successfully"
	else
		echo "failed to remove launch link, quitting"
		exit 1
	fi
}

function installLaunchLink {
	log "installing launch link"
	local cmd="ssh ${RG350_IP} /bin/busybox sh -c \"ln -fs ${RG350_HOME}/${TARGET_DIR}/esoteric ${LAUNCHER}\""
	log ${cmd}
	${cmd}
	if [ $? -eq 0 ]; then
		echo "installed launcher link successfully"
	else
		echo "failed to install launch link, quitting"
		exit 1
	fi
}

function fullSync {
	local buildFolder="/dist/RG350/esoteric/"
	log "doing a full sync from '.${buildFolder}' to rg-350 : ${RG350_IP}"
	local cmd="rsync -r `pwd`${buildFolder} ${RG350_IP}:${RG350_HOME}/${TARGET_DIR}"
	log "running : ${cmd}"
	${cmd}
	if [ $? -ne 0 ]; then
		echo "full sync failed"
		exit 1
	fi
	return $?
}

function binaryDeploy {
	log "doing a quick binary only deploy"
	local cmd="scp ./objs/RG350/esoteric ${RG350_IP}:${RG350_HOME}/${TARGET_DIR}"
	log ${cmd}
	${cmd}
	return $?
}

function myClean () {
	local target="${1}"
	case ${target} in
		all )
			log "doing a clean for all targets"
			for myTarget in ${validTargets[*]}; do
				myClean ${myTarget}
			done
			;;
		linux )
			log "doing a linux clean"
			if [[ -v ${CROSS_COMPILE} ]]; then
				unset CROSS_COMPILE
			fi
			local cmd="make -f ./Makefile.linux clean${REDIRECT}"
			eval ${cmd}
			;;
		rg350 )
			log "doing a rg350 clean"
			export CROSS_COMPILE=/opt/gcw0-toolchain/usr/bin/mipsel-linux-
			local cmd="make -f ./Makefile.rg-350 clean${REDIRECT}"
			eval ${cmd}
			;;
	esac
}

function myBuild () {

	export DEBUG=${debugLevel}
	local target=$1

	case ${target} in
		all )
			log "doing a build for all targets"
			for myTarget in ${validTargets[*]}; do
				myBuild ${myTarget}
			done
			;;
		linux )
			log "doing a linux build"
			if [[ -v ${CROSS_COMPILE} ]]; then
				unset CROSS_COMPILE
			fi
			local cmd="make -f ./Makefile.linux all dist${REDIRECT}"
			eval ${cmd}
			if [[ $? -eq 0 ]]; then
				echo "binary: ./dist/LINUX/esoteric/esoteric"
			fi
			;;
		rg350 )
			log "doing a rg350 build"
			export CROSS_COMPILE=/opt/gcw0-toolchain/usr/bin/mipsel-linux-
			local cmd="make -f ./Makefile.rg-350 all dist${REDIRECT}"
			eval ${cmd}
			;;
		* )
			echo "unsupported build target : ${target}"
			exit 1
			;;
	esac
}

function makePackage () {

	if [[ $# -lt 1 ]]; then
		echo "makePackage needs a target"
		exit 1
	fi
	local target=$1
	local version=`date +%Y-%m-%d-%H:%m:%S`
	if [[ $# -gt 1 ]]; then
		version=$2
	fi

	case ${target} in
		all )
			log "building packages for all targets"
			for myTarget in ${validTargets[*]}; do
				makePackage ${myTarget} ${version}
			done
			;;
		linux )
			log "building a linux package"
			if [[ -v ${CROSS_COMPILE} ]]; then
				unset CROSS_COMPILE
			fi
			local cmd="make -f ./Makefile.linux dist${REDIRECT}"
			local artifact="${APP_NAME}-${version}.zip"
			eval ${cmd}
			local cmd="(cd dist/LINUX/esoteric; zip -r ../../../${artifact} .${REDIRECT}; cd ../../../;)"
			eval ${cmd}
			echo "linux package created at : ${artifact}"
			;;
		rg350 )
			log "building a rg350 package"
			export CROSS_COMPILE=/opt/gcw0-toolchain/usr/bin/mipsel-linux-
			local cmd="make -f ./Makefile.rg-350 dist${REDIRECT}"
			local artifact="${APP_NAME}-${version}.opk"
			eval ${cmd}
			local cmd="./make_opk.sh \"${artifact}\" \"${version}\"${REDIRECT}"
			eval ${cmd}
			echo "rg350 package created at : ${artifact}"
			;;
		* )
			echo "no package defined for ${target}"
	esac

}

function showHelp () {
    echo "Usage:"
	echo "    -b                 Do a build for a valid target."
    echo "    -c                 Full clean when \${target} = [all ${validTargets[@]}]"
	echo "    -d x               set debug level [valid levels are 0 - 5, default is 2]"
	echo "    -h                 Display this help message."
	echo "    -i                 install launcher script when \${target} = ['rg350']."
	echo "    -p                 Make a package when \${target} = ['rg350']."
	echo "    -r                 Tag the last commit as a release"
	echo "    -s                 Full sync to target when \${target} = ['rg350']."
	echo "    -u                 uninstall launcher script when \${target} = ['rg350']."
	echo "    -t target          Set \${target} = [${validTargets[@]}]"
	echo "    -v                 verbose output"
	echo "    -V                 verbose and build output"
}

# ----------------------------------- #

if [ $# -eq 0 ]; then
	showHelp
	exit 0
fi

REDIRECT=" 1>/dev/null"
debugLevel=2
requestedDebugLevel=""
requestedTarget=""
verbose="false"
target=""
doClean="false"
doTarget="false"
doDebug="false"
doBuild="false"
doQuick="false"
doSync="false"
doInstall="false"
doUninstall="false"
doRelease="false"
doPackage="false"

validTargets=("linux" "rg350")

while getopts ":bcd:hiprst:uvV" opt; do

  case ${opt} in
	b )	
		log "build flag set"
		doBuild="true"
	  	;;
	c )
		log "clean flag set"
		doClean="true"
		;;
	d )
		log "debug flag set"
		requestedDebugLevel=$OPTARG
		doDebug="true"
	  	;;
	h )
		log "help flag set"
		showHelp
      	;;
    i )
		log "install flag set"
		doInstall="true"
      	;;
	p )
		log "package flag set"
		doPackage="true"
		;;
	r )
		log "release flag set"
		doRelease="true"
	  	;;
	s )
		log "sync files flag set"
		doSync="true"
	  	;;
	t )
		log "target flag set"
		requestedTarget=$OPTARG
		doTarget="true"
	  	;;
    u )
		log "uninstal tag set"
		doUninstall="true"
      	;;
	v )
		verbose="true"
		log "verbose mode enabled"
	  	;;
	V )
		REDIRECT=""
		verbose="true"
		log "very verbose mode enabled"
	  	;;
    \? )
      	showHelp
      	;;
    : )
      	log "Invalid option: $OPTARG requires an argument" 1>&2
	  	showHelp
      	;;
  esac
done
shift $((OPTIND -1))

checkDependencies

# ok, time to actually do the requested work....

if [ ${doDebug} == "true" ]; then
	log "debug level override is requested"
	re_isInt='^[0-9]+$'
	if [[ ${requestedDebugLevel} =~ ${re_isInt} ]]; then
		#log "debug level is an int"
		if (( ${requestedDebugLevel} >= 0 && ${requestedDebugLevel} <= 5 )); then
			log "setting debug level to : ${requestedDebugLevel}"
			debugLevel=${requestedDebugLevel}
		else
			echo "debug level has to be an integer between 0 and 5";
			exit 1
		fi
	else
		echo "debug level has to be an integer between 0 and 5";
		exit 1
	fi
fi

if [ ${doTarget} == "true" ]; then
	if [ $requestedTarget == "all" ]; then
		target="all"
	else
		log "checking ${requestedTarget} is a valid target"
		for myTarget in ${validTargets[*]}; do
			log "checking ${myTarget} against ${requestedTarget}"
			if [ ${myTarget} == ${requestedTarget} ]; then
				log "setting target successfully to : ${myTarget}"
				target=${myTarget}
				break
			fi
		done
	fi
	if [[ -z ${target} ]]; then
		echo "${requestedTarget} is not a valid build target"
		echo "valid build targets are : [all ${validTargets[*]}]"
		exit 1
	fi
fi

if [ ${doClean} == "true" ] && [[ ! -z ${target} ]]; then
	echo "cleaning target : ${target}"
	myClean ${target}
fi

if [ ${doBuild} == "true" ] && [[ ! -z ${target} ]]; then
	echo "building for target : ${target}"
	myBuild ${target}
fi

if [ ${doSync} == "true" ] && [ ${target} != "linux" ] && [[ ! -z ${target} ]]; then
	echo "syncing files to device"
	fullSync
fi

if [ ${doInstall} == "true" ] && [ ${target} != "linux" ] && [[ ! -z ${target} ]]; then
	installLaunchLink
fi

if [ ${doUninstall} == "true" ] && [ ${target} != "linux" ] && [[ ! -z ${target} ]]; then
	removeLaunchLink
fi

if [ ${doPackage} == "true" ] && [[ ! -z ${target} ]]; then
	echo "creating a package for : ${target}"
	makePackage ${target}
fi

if [ ${doRelease} == "true" ]; then
	debugLevel=2
	echo "creating a release"
	# first we clean and build all
	myClean "all"
	myBuild "all"
	echo "build all done"
	# and then get the current semver
	current_semver=""
	getSemVer current_semver
	# increment it
	new_semver=""
	incrementSemVer new_semver ${current_semver}
	# tag the repo
	tagRepo ${new_semver}
	# package all flavours with our new release number
	makePackage "all" ${new_semver}
fi

log "finished"
exit 0
