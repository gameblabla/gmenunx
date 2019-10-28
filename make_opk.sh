#!/bin/sh

OPK_NAME=gmenunx.opk
ASSETS_PATH="dist/rg-350/gmenunx"

if [ ! -d ${ASSETS_PATH} ]; then
    echo "Can't fid ${ASSETS_PATH}, You need to run a make dist first"
    exit 1
fi
echo ${OPK_NAME}

# create default.gcw0.desktop
cat > default.gcw0.desktop <<EOF
[Desktop Entry]
Name=GmenuNX
Comment=GmenuNX
Exec=/mnt/GmenuNX/gmenunx
Terminal=false
Type=Application
StartupNotify=true
Icon=rg350
Categories=applications;
EOF

# create opk
FLIST="${ASSETS_PATH}/*"
FLIST="${FLIST} default.gcw0.desktop"
FLIST="${FLIST} assets/rg-350/icons/rg350.png"

rm -f ${OPK_NAME}
mksquashfs ${FLIST} ${OPK_NAME} -all-root -no-xattrs -noappend -no-exports

cat default.gcw0.desktop
rm -f default.gcw0.desktop
