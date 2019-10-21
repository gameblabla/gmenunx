#!/bin/sh

OPK_NAME=gmenunx.opk

echo ${OPK_NAME}

# create default.gcw0.desktop
cat > default.gcw0.desktop <<EOF
[Desktop Entry]
Name=GmenuNX
Comment=GmenuNX
Exec=gmenu2x
Terminal=false
Type=Application
StartupNotify=true
Icon=generic
Categories=applications;
EOF

# create opk
FLIST="objs/rg-350/gmenunx"
FLIST="${FLIST} default.gcw0.desktop"
FLIST="${FLIST} assets/skins/Default/icons/generic.png"
FLIST="${FLIST} assets/skins"
FLIST="${FLIST} assets/translations"
FLIST="${FLIST} assets/rg-350/*"

rm -f ${OPK_NAME}
mksquashfs ${FLIST} ${OPK_NAME} -all-root -no-xattrs -noappend -no-exports

cat default.gcw0.desktop
rm -f default.gcw0.desktop
