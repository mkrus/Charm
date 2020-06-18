#!/usr/bin/env bash

# Exit if any undefined variable is used.
set -u

# Exit this script if it any subprocess exits non-zero.
set -e

# If any process in a pipeline fails, the return value is a failure.
# set -o pipefail

if [ $# -ne 2 ];
then
    echo "Usage: $(basename "$0") -app-dir=<app bundle directory> -app-name=<app name>"
    exit 1
fi

for arg in ${@}; do
    if [[ ${arg} = -app-dir=* ]]; then
        APP_DIR=${arg#*=}
    fi
    if [[ ${arg} = -app-name=* ]]; then
        APP_NAME=${arg#*=}
    fi
done

echo App dir: ${APP_DIR}
echo App name: ${APP_NAME}

binaries=$(find ${APP_DIR} -type f \( -name "*.dylib" -o -name "*.so" -o -name ${APP_NAME} \))

for binary in ${binaries}; do
    echo Checking: ${binary}

    #        otool ...          | Absolute path | <search>  ,<regex capture path      >,<capture group>,<print capture group>
    rpaths=$(otool -l ${binary} | grep "path /" | sed -ne "s,.*path\([^@].*\) (offset.*,\1,p")
    for rpath in ${rpaths}; do
        echo - deleting: ${rpath}
        install_name_tool -delete_rpath ${rpath} ${binary}
    done
done

DYLD_BIND_AT_LAUNCH=1 ${APP_DIR}/Contents/MacOS/${APP_NAME} --version
