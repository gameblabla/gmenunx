#!/bin/sh

echo "gmenunx installer script v1.6"

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

# Run gmenunx -install, bomb if we get a non zero return code
today=`date +"%Y-%m-%d"`
echo "logging install to : /tmp/gmenunx.install.${today}.log"

echo "running install as: "
echo "/usr/bin/opkrun -m default.gcw0.desktop ${RESOLVED_PATH} -i 2>&1 >> /tmp/gmenunx.install.${today}.log"
/usr/bin/opkrun -m default.gcw0.desktop ${RESOLVED_PATH} -i 2>&1 >> /tmp/gmenunx.install.${today}.log

rc=$?; if [[ $rc != 0 ]]; then 
    echo "Failed to install properly, not continuing shell script"
    exit $rc; 
fi
echo "install looks good!"

if [ -e ${LAUNCHER_PATH} ]; then
    echo "Removing previous launcher script at : ${LAUNCHER_PATH}"
    rm -rf ${LAUNCHER_PATH}
fi

echo "installing launcher file"
cat > ${LAUNCHER_PATH} <<EOF
#!/bin/sh

today=\`date +"%Y-%m-%d"\`
if [ -f ${RESOLVED_PATH} ]; then
    /usr/bin/opkrun -m default.gcw0.desktop ${RESOLVED_PATH} 2>&1 >> /tmp/gmenunx.run.\${today}.log
else
    /usr/bin/gmenu2x
fi
EOF

echo "making launcher file executable"
chmod +x ${LAUNCHER_PATH}

echo "installed launcher, starting gmenunx..."
killall gmenu2x