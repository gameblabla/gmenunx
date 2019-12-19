#!/bin/bash

APP_NAME="esoteric";
OPK_EXTENSION=".opk";
ASSETS_PATH="dist/RG350/esoteric";
argCount=$#
OPK_NAME="${APP_NAME}${OPK_EXTENSION}"

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

# create default.gcw0.desktop
cat > default.gcw0.desktop <<EOF
[Desktop Entry]
Name=350teric
Comment=Esoteric App Launcher
Exec=${APP_NAME}
Terminal=false
Type=Application
StartupNotify=true
Icon=logo
Categories=applications;
Version=${VERSION}
EOF

# create opk
FLIST="${ASSETS_PATH}/*"
FLIST="${FLIST} default.gcw0.desktop"

if [ -f ${OPK_NAME} ]; then
    echo "removing already existing file : ${OPK_NAME}"
    rm -f ${OPK_NAME}
fi

mksquashfs ${FLIST} ${OPK_NAME} -all-root -no-xattrs -noappend -no-exports

cat default.gcw0.desktop
rm -f default.gcw0.desktop

echo "opk created at : ${OPK_NAME}"
exit 0
