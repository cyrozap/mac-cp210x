#!/bin/sh

CONFIG=$1
TARGET=$2
BUNDLE="coop.plausible.CP210x"

if [ -z "${CONFIG}" ] || [ -z "${TARGET}" ]; then
    echo "Usage: $0 <build config> <target>"
    echo "Target may either be 'localhost', or a remote system to which you have ssh and sudo access"
    exit 1
fi

if [ "${TARGET}" = "localhost" ]; then
    LOCAL="1"
    RSYNC_TARGET="./"
else
    RSYNC_TARGET="${TARGET}:"
fi

function remote () {
    if [ ! -z "${LOCAL}" ]; then
        $*
    else
        ssh "${TARGET}" $*
    fi 
}

function unload_kext () {
    # Check if loaded
    remote sudo kextstat -b "${BUNDLE}" | grep "${BUNDLE}" >/dev/null
    if [ $? != 0 ]; then
        return;
    fi

    remote sudo kextunload CP210x.kext
    if [ $? != 0 ]; then
        echo "Failed to unload, trying again"
        unload_kext
    fi
}

xcodebuild -configuration "${CONFIG}" -target CP210x
if [ "$?" != 0 ]; then
    exit 1
fi

# If the kext is loaded, try to unload it
unload_kext

remote sudo rm -rf CP210x.kext
rsync -avz "build/${CONFIG}/CP210x.kext" ${RSYNC_TARGET} &&
    remote sudo chown -R root:wheel CP210x.kext &&
    remote sudo kextload CP210x.kext &&
    echo "kext loaded"
