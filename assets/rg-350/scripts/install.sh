#!/bin/sh

INTERNAL_PATH="/media/data/apps"
EXTERNAL_PATH="/media/sdcard/APPS"
OPK_NAME="gmenunx.opk"
RESOLVED_PATH=""
LAUNCHER_PATH="/usr/local/sbin/frontend_start"

if [ -f "${INTERNAL_PATH}/${OPK_NAME}" ]; then
    echo "Found our opk in the internal apps folder - ${INTERNAL_PATH}/${OPK_NAME}"
    RESOLVED_PATH=${INTERNAL_PATH}/${OPK_NAME}
elif [ -f "${EXTERNAL_PATH}/${OPK_NAME}" ]; then
    echo "Found our opk in the external apps folder - ${EXTERNAL_PATH}/${OPK_NAME}"
    RESOLVED_PATH=${EXTERNAL_PATH}/${OPK_NAME}
else
    echo "Couldn't find our opk, aborting install"
    exit 1
fi

if [ -e ${LAUNCHER_PATH} ]; then
    echo "Removing previous launcher script at : ${LAUNCHER_PATH}"
    rm -rf ${LAUNCHER_PATH}
fi

echo "installing launcher file"
cat > ${LAUNCHER_PATH} <<EOF
#!/bin/sh
if [ -f ${RESOLVED_PATH} ]; then
    /usr/bin/opkrun -m default.gcw0.desktop ${RESOLVED_PATH}
else
    /usr/bin/gmenu2x
fi
EOF

echo "making launcher file executable"
chmod +x ${LAUNCHER_PATH}

echo "installed launcher, starting..."

killall gmenu2x