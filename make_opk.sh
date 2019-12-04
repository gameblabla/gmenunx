#!/bin/sh

OPK_NAME=esoteric.opk
ASSETS_PATH="dist/RG350/esoteric"

if [ ! -d ${ASSETS_PATH} ]; then
    echo "Can't find ${ASSETS_PATH}, You need to run a make dist first"
    exit 1
fi
echo "Making opk : ${OPK_NAME}"

# create default.gcw0.desktop
cat > default.gcw0.desktop <<EOF
[Desktop Entry]
Name=350teric
Comment=Esoteric App Launcher
Exec=esoteric
Terminal=false
Type=Application
StartupNotify=true
Icon=logo
Categories=applications;
EOF

# create opk
FLIST="${ASSETS_PATH}/*"
FLIST="${FLIST} default.gcw0.desktop"
#FLIST="${FLIST} logo.png"

rm -f ${OPK_NAME}
mksquashfs ${FLIST} ${OPK_NAME} -all-root -no-xattrs -noappend -no-exports

cat default.gcw0.desktop
rm -f default.gcw0.desktop

