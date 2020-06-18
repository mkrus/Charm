#!/usr/bin/env bash

APP_NAME=Charm
APP_ID=com.kdab.charm

BUILDROOT=${PWD}
APP_APP=${APP_NAME}.app
APP_DMG=${APP_APP/app/dmg}

echo $BUILDROOT
echo $APP_APP
echo $APP_DMG


NOTARIZE_PASS=@keychain:NOTARY_PASSWORD     # xcrun altool --store-password-in-keychain-item NOTARY_PASSWORD --username ... --password ...
NOTARIZE_PROVIDER=KDABUKLtd             # xcrun altool --list-providers -u ... -p ...

for arg in ${@}; do
    if [[ ${arg} = -notarize-ac=* ]]; then
        NOTARIZE_ACC=${arg#*=}
    fi
    if [[ ${arg} = -notarize-pass=* ]]; then
        NOTARIZE_PASS=${arg#*=}
    fi
    if [[ ${arg} == -qt5-bin-dir=* ]]; then
        QT5_BIN_DIR=${arg#*=}
    fi
    if [[ ${arg} == -entitlements=* ]]; then
        ENTITLEMENTS_FILE=${arg#*=}
    fi
done

echo "Qt dir: ${QT5_BIN_DIR}"
echo "Entitlements file: ${ENTITLEMENTS_FILE}"

waiting_fixed() {
    local message=${1}
    local waitTime=${2}

    for i in $(seq ${waitTime}); do
        sleep 1
        printf -v dots '%*s' ${i}
        printf -v spaces '%*s' $((${waitTime} - $i))
        printf "\r%s [%s%s]" "${message}" "${dots// /.}" "${spaces}"
    done
    printf "\n"
}

notarize_build() {
    local NOT_SRC_FILE=${1}

    # zip it
    ditto -c -k --sequesterRsrc --keepParent "${NOT_SRC_FILE}" "${BUILDROOT}/tmp_notarize/${NOT_SRC_FILE}.zip"

    # trigger notarizing
    local response=$(xcrun altool --notarize-app --primary-bundle-id "${APP_ID}" --username "${NOTARIZE_ACC}" --password "${NOTARIZE_PASS}" -itc_provider "${NOTARIZE_PROVIDER}" --file "${BUILDROOT}/tmp_notarize/${NOT_SRC_FILE}.zip" 2>&1)
    local uuid="$(echo "${response}" | grep RequestUUID | awk '{ print $3 }')"

    if [[ -z "${uuid}" ]]; then
        echo "Failed to initiate notarization!"
        echo "${response}"
        exit 1
    fi

    echo "RequestUUID = ${uuid}" # Display identifier string

    waiting_fixed "Waiting to retrieve notarize status" 15

    # wait until done
    while true ; do
        response="$(xcrun altool --notarization-info "${uuid}" --username "${NOTARIZE_ACC}" --password "${NOTARIZE_PASS}" 2>&1)"
        notarize_status="$(echo "${response}" | grep 'Status\:' | awk '{ print $2 }')"
        if [[ "${notarize_status}" = "success" ]]; then
            xcrun stapler staple "${NOT_SRC_FILE}"   #staple the ticket
            xcrun stapler validate -v "${NOT_SRC_FILE}"
            echo "Notarization success!"
            break
        elif [[ "${notarize_status}" = "in" ]]; then
            waiting_fixed "Notarization still in progress, sleeping for 15 seconds and trying again" 15
        elif [[ "${response}" =~ "Apple Services operation failed" ]]; then
            echo "${response}"
            waiting_fixed "Apple Services error, sleeping for 15 seconds and trying again" 15
        else
            echo "${response}"
            echo "Notarization failed!"
            exit 1
        fi
    done
}

createDMG () {
    DMG_size=700
    DMG_title="${APP_NAME}"
    DMG_dir=tmp_dmg

    test ! -e ${DMG_dir} || /bin/rm -rf ${DMG_dir}
    mkdir ${DMG_dir}
    echo "Creating dmg: Pwd: $(pwd); ${APP_APP}; ${DMG_dir}"
    tar cBf - ./${APP_APP} | (cd ${DMG_dir}; tar xf -)

    ## Build dmg from folder

    # create dmg on local system
    # usage of -fsargs minimze gaps at front of filesystem (reduce size)
    hdiutil create -srcfolder "${DMG_dir}" -volname "${DMG_title}" -fs HFS+ \
        -fsargs "-c c=64,a=16,e=16" -format UDRW -size ${DMG_size}m ${APP_NAME}.temp.dmg

    # Next line is only useful if we have a dmg as a template!
    # previous hdiutil must be uncommented
    device=$(hdiutil attach -readwrite -noverify -noautoopen "${APP_NAME}.temp.dmg" | egrep '^/dev/' | sed 1q | awk '{ print $1 }')

    # Set style for dmg
    ln -s "/Applications" "/Volumes/${DMG_title}/Applications"

    chmod -Rf go-w "/Volumes/${DMG_title}"

    # ensure all writing operations to dmg are over
    sync

    hdiutil detach $device
    hdiutil convert "${APP_NAME}.temp.dmg" -format UDZO -imagekey -zlib-level=9 -o ${APP_NAME}-out.dmg

    mv ${APP_NAME}-out.dmg ${APP_NAME}.dmg
    rm ${APP_NAME}.temp.dmg
}

test ! -e ${APP_DMG} || /bin/rm -rf ${APP_DMG}

echo Code signing app Bundle
if ! codesign --options runtime --timestamp -fv --deep -s "Developer ID Application: KDAB (UK) Ltd. (72QK35344H)" --entitlements ${ENTITLEMENTS_FILE} ${APP_APP}; then
    echo "Code signing app bundle failed"
    exit 1
fi

if [[ -n ${NOTARIZE_ACC} ]]; then
    echo notarizing app
    notarize_build ${APP_APP}
fi
echo creating dmg
createDMG

echo Code signing dmg
if ! codesign --options runtime --timestamp -f -s "Developer ID Application: KDAB (UK) Ltd. (72QK35344H)" "${APP_DMG}"; then
    echo "Code signing DMG failed"
    exit 1
fi

if [[ -n ${NOTARIZE_ACC} ]]; then
    echo notarizing dmg
    notarize_build "${APP_DMG}"
fi
