#!/bin/sh

OPK_NAME=gmenunx.opk
ASSETS_PATH="dist/RG350/gmenunx"

if [ ! -d ${ASSETS_PATH} ]; then
    echo "Can't find ${ASSETS_PATH}, You need to run a make dist first"
    exit 1
fi
echo "Making opk : ${OPK_NAME}"

# create default.gcw0.desktop
cat > default.gcw0.desktop <<EOF
[Desktop Entry]
Name=GmenuNX
Comment=App Launcher
Exec=gmenunx
Terminal=false
Type=Application
StartupNotify=true
Icon=rg350
Categories=applications;
EOF

#cat > install.gcw0.desktop <<EOF
#[Desktop Entry]
#Name=Install GmenuNX
#Comment=Installs GmenuNX
#Exec=/bin/sh -C scripts/install.sh
#Terminal=true
#Type=Application
#StartupNotify=true
#Icon=rg350
#Categories=applications;
#EOF

#cat > uninstall.gcw0.desktop <<EOF
#[Desktop Entry]
#Name=Uninstall GmenuNX
#Comment=Uninstalls GmenuNX
#Exec=/bin/sh -C scripts/uninstall.sh
#Terminal=true
#Type=Application
#StartupNotify=true
#Icon=rg350
#Categories=applications;
#EOF

# create opk
FLIST="${ASSETS_PATH}/*"
FLIST="${FLIST} default.gcw0.desktop"
#FLIST="${FLIST} install.gcw0.desktop"
#FLIST="${FLIST} uninstall.gcw0.desktop"
FLIST="${FLIST} assets/rg-350/icons/rg350.png"

rm -f ${OPK_NAME}
mksquashfs ${FLIST} ${OPK_NAME} -all-root -no-xattrs -noappend -no-exports

cat default.gcw0.desktop
rm -f default.gcw0.desktop
#rm -f install.gcw0.desktop
#rm -f uninstall.gcw0.desktop
