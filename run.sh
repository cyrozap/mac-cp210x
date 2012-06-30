#!/bin/sh

TARGET=$1

if [ -z "${TARGET}" ]; then
    echo "Specify a remote target or 'localhost'"
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

xcodebuild -configuration Debug -target CP210x &&
    remote sudo kextunload CP210x.kext &&
    remote sudo rm -rf CP210x.kext &&
    rsync -avz build/Debug/CP210x.kext ${RSYNC_TARGET} &&
    remote sudo chown -R root:wheel CP210x.kext &&
    remote sudo kextload CP210x.kext

