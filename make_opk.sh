#!/bin/bash

APP_NAME="esoteric";
OPK_EXTENSION=".opk";
ASSETS_PATH="dist/RG350/esoteric";
argCount=$#
OPK_NAME="${APP_NAME}${OPK_EXTENSION}"
SUPPORTED_PLATFORMS=("gcw0" "retrofw")

# ----------------------------- #
echo "Enter make_opk.sh"

if [ ! -d ${ASSETS_PATH} ]; then
    echo "Can't find ${ASSETS_PATH}, You need to run a make dist first"
    exit 1
fi

if (( ${argCount} > 0 )); then
    echo "we got a name override : $1"
    OPK_NAME="${1}"
fi
if (( ${argCount} > 1 )); then
    echo "we got a version override : $2"
    VERSION="${2}"
fi

echo "Making opk : ${OPK_NAME}"

FLIST="${ASSETS_PATH}/*"

for myPlatform in ${SUPPORTED_PLATFORMS[*]}; do
    # create default.${myPlatform}.desktop
    cat > default.${myPlatform}.desktop <<EOF
[Desktop Entry]
Name=350teric
Comment=Esoteric App Launcher
Exec=${APP_NAME}
Terminal=false
Type=Application
StartupNotify=true
Icon=logo
Categories=applications;
X-OD-Manual=esoteric.man.txt
Version=${VERSION}
EOF

    FLIST="${FLIST} default.${myPlatform}.desktop"

done

# create opk
if [ -f ${OPK_NAME} ]; then
    echo "removing already existing file : ${OPK_NAME}"
    rm -f ${OPK_NAME}
fi

mksquashfs ${FLIST} ${OPK_NAME} -all-root -no-xattrs -noappend -no-exports

cat default.gcw0.desktop
rm -f default.*.desktop

echo "opk created at : ${OPK_NAME}"
exit 0
