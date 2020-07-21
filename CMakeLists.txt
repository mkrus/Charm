CMAKE_MINIMUM_REQUIRED( VERSION 3.1.0 )
PROJECT( Charm CXX )

# Enable C++17
SET(CMAKE_CXX_STANDARD 17)

include(ExternalProject)
include(FeatureSummary)

set(ECM_MODULE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ECM/modules/")
set(CMAKE_MODULE_PATH ${ECM_MODULE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ECM/kde-modules" )

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)
include(ECMInstallIcons)

add_definitions( -DQT_DEPRECATED_WARNINGS )

IF( NOT Charm_VERSION )
    FIND_PACKAGE( Git QUIET )
    IF( EXISTS ${GIT_EXECUTABLE} )
        EXECUTE_PROCESS( COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=1
            RESULT_VARIABLE GIT_RESULT
            OUTPUT_VARIABLE Charm_VERSION
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_STRIP_TRAILING_WHITESPACE )
        IF( NOT GIT_RESULT EQUAL 0 )
            MESSAGE( FATAL_ERROR "Cannot get 'git describe' version!" )
        ENDIF()
    ENDIF()
ENDIF()

STRING( REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.(.*)$" Charm_VERSION "${Charm_VERSION}")
SET( Charm_VERSION_MAJOR "${CMAKE_MATCH_1}" )
SET( Charm_VERSION_MINOR "${CMAKE_MATCH_2}" )
SET( Charm_VERSION_PATCH "${CMAKE_MATCH_3}" )
SET( Charm_VERSION_COUNT 3 )

IF( NOT ( DEFINED Charm_VERSION_MAJOR AND DEFINED Charm_VERSION_MINOR AND DEFINED Charm_VERSION_PATCH ) )
    MESSAGE( FATAL_ERROR "No Git executable or valid Charm version argument found.\n"
    "Please pass a version to CMake e.g. cmake -DCharm_VERSION=1.0.0" )
ENDIF()

MESSAGE( STATUS "Building Charm ${Charm_VERSION} in ${CMAKE_BUILD_TYPE} mode" )

set(QT_REQUIRED_VERSION "5.10")

find_package(Qt5Core ${QT_REQUIRED_VERSION} REQUIRED)
find_package(Qt5Widgets ${QT_REQUIRED_VERSION} REQUIRED)
find_package(Qt5Xml ${QT_REQUIRED_VERSION} REQUIRED)
find_package(Qt5Network ${QT_REQUIRED_VERSION} REQUIRED)
find_package(Qt5Sql ${QT_REQUIRED_VERSION} REQUIRED)
find_package(Qt5Test ${QT_REQUIRED_VERSION} REQUIRED)
find_package(Qt5Bluetooth ${QT_REQUIRED_VERSION} REQUIRED)
find_package(Qt5PrintSupport ${QT_REQUIRED_VERSION})

if(WIN32)
    find_package(Qt5WinExtras ${QT_REQUIRED_VERSION} REQUIRED)
endif()

IF(APPLE)
    find_package(Qt5MacExtras ${QT_REQUIRED_VERSION} REQUIRED)
ENDIF()

IF(UNIX AND NOT APPLE)
    find_package(Qt5DBus ${QT_REQUIRED_VERSION} QUIET)
ENDIF()

option(BUILD_INTERNAL_QTKEYCHAIN "Enable build of external project qtkeychain" OFF)
add_feature_info("Internal build of qtkeychain" BUILD_INTERNAL_QTKEYCHAIN "enable with BUILD_INTERNAL_QTKEYCHAIN=ON")
if(BUILD_INTERNAL_QTKEYCHAIN)
    include(cmake/QtKeychainProject.cmake)
else()
    find_package(Qt5Keychain REQUIRED)
    set_package_properties(Qt5Keychain PROPERTIES
                           DESCRIPTION "Provides support for secure credentials storage"
                           URL "https://github.com/frankosterfeld/qtkeychain"
                           TYPE REQUIRED)
endif()

SET(CHARM_MAC_HIGHRES_SUPPORT_ENABLED ON)


ENABLE_TESTING()

IF( UNIX AND NOT APPLE )
    set( Charm_EXECUTABLE charmtimetracker )
ELSE()
    set( Charm_EXECUTABLE Charm )
ENDIF()

SET( SYSCONF_INSTALL_DIR etc)
SET( BIN_INSTALL_DIR bin )
SET( DOC_INSTALL_DIR ${CMAKE_INSTALL_DOCBUNDLEDIR}/${Charm_EXECUTABLE} )
SET( ICONS_DIR "${CMAKE_SOURCE_DIR}/Charm/Icons" )

IF( CHARM_PREPARE_DEPLOY AND WIN32 OR APPLE)
    SET( BIN_INSTALL_DIR . )
    SET( DOC_INSTALL_DIR . )
ENDIF()

IF( APPLE )
    option(PACKAGE_DO_NOTARIZE "Notarize the application bundle and DMG when packaging Charm" OFF)
    option(PACKAGE_DEVELOPER_ACCOUNT "The Apple developer account to use for the notarization process" "")
ENDIF()

OPTION( CHARM_IDLE_DETECTION "Build the Charm idle detector" ON )
set( CHARM_IDLE_TIME "360" CACHE STRING "Set the idle timeout (in seconds, default 360)" )

OPTION(CHARM_PREPARE_DEPLOY "Deploy dependencies with install target(Windows, Apple)" ON)

OPTION (CHARM_DEVELOPER_MODE "Build in developer mode. Use a separate database and configuration" OFF)
IF (CHARM_DEVELOPER_MODE)
    ADD_DEFINITIONS(-DCHARM_DEVELOPER_MODE)
ENDIF (CHARM_DEVELOPER_MODE)

ADD_SUBDIRECTORY( Core )
ADD_SUBDIRECTORY( Charm )
ADD_SUBDIRECTORY( AutoTests )

CONFIGURE_FILE( CharmCMake.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/CharmCMake.h )

SET( LICENSE_FILE "License.txt" )
SET( README_FILE "ReadMe.md" )
IF( NOT APPLE )
    INSTALL( FILES "${LICENSE_FILE}" "${README_FILE}" DESTINATION ${DOC_INSTALL_DIR} )
    FIND_PACKAGE( OpenSSL REQUIRED)
    set_package_properties(OpenSSL PROPERTIES
                                   PURPOSE "Deployment of openssl libraries."
                                   TYPE REQUIRED)
ENDIF()

IF (WIN32)
    FIND_PACKAGE( PythonInterp 3.5 QUIET)
    set_package_properties(PythonInterp PROPERTIES
                                        PURPOSE "Used to create Windows installer"
                                        TYPE OPTIONAL)

    IF(PYTHONINTERP_FOUND)
        OPTION(CHARM_SIGN_INSTALLER "Sign the installer and the contained files" ON)

        SET(EXTRA_PACKAGE_COMMANDS )

        IF (OPENSSL_FOUND)
            SET(EXTRA_PACKAGE_COMMANDS ${EXTRA_PACKAGE_COMMANDS} --deployOpenSSL1_1 "${OPENSSL_INCLUDE_DIR}/../")
        ENDIF()
        IF( CMAKE_BUILD_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])" )
            SET(EXTRA_PACKAGE_COMMANDS ${EXTRA_PACKAGE_COMMANDS} --buildType debug)
        ENDIF()

        IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(EXTRA_PACKAGE_COMMANDS ${EXTRA_PACKAGE_COMMANDS} --architecture x64)
        ELSE()
            set(EXTRA_PACKAGE_COMMANDS ${EXTRA_PACKAGE_COMMANDS} --architecture x86)
        ENDIF()

        IF(CHARM_SIGN_INSTALLER)
            set(EXTRA_PACKAGE_COMMANDS ${EXTRA_PACKAGE_COMMANDS} --sign)
        ENDIF()


        ADD_CUSTOM_TARGET(package
            COMMAND
                ${PYTHON_EXECUTABLE}
                "${CMAKE_CURRENT_SOURCE_DIR}/scripts/create-win-installer.py"
                --installerName "Charm-${Charm_VERSION}.exe"
                --applicationFileName "bin/Charm.exe"
                --buildDir "${CMAKE_CURRENT_BINARY_DIR}"
                --productName Charm
                --productVersion "${Charm_VERSION}"
                --companyName KDAB
                --applicationIcon "${CMAKE_CURRENT_SOURCE_DIR}/Charm/Icons/Charm.ico"
                --productLicence "${CMAKE_CURRENT_SOURCE_DIR}/License.txt"
                ${EXTRA_PACKAGE_COMMANDS}
            DEPENDS
                ${Charm_EXECUTABLE}
            VERBATIM)
    ENDIF()
ELSEIF( APPLE )
    set(QT5_BIN_DIR "${Qt5Core_DIR}/../../../bin")

    # use absolute file paths for dependent libraries so they're found properly while packaging
    set(APP_DIR ${CMAKE_BINARY_DIR}/Charm/${PROJECT_NAME}.app)

    set(NOTARIZE_ARGS "")
    if(PACKAGE_DO_NOTARIZE)
        if ("${PACKAGE_DEVELOPER_ACCOUNT}" STREQUAL "")
            message(FATAL_ERROR "PACKAGE_DO_NOTARIZE is on, but no PACKAGE_DEVELOPER_ACCOUNT email address has been provided.")
        else()
            set(NOTARIZE_ARGS -notarize-ac=${PACKAGE_DEVELOPER_ACCOUNT})
        endif()
    endif()

    add_custom_target(createInstaller
        COMMAND ${QT5_BIN_DIR}/macdeployqt ${APP_DIR} -always-overwrite -verbose=2
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deploy/fix-rpaths.sh -app-dir=${APP_DIR} -app-name=${PROJECT_NAME}
        COMMAND ${CMAKE_COMMAND} -E remove ${PROJECT_NAME}.dmg
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deploy/notarize.sh -qt5-bin-dir=${QT5_BIN_DIR} ${NOTARIZE_ARGS} -entitlements=${CMAKE_CURRENT_SOURCE_DIR}/deploy/charm.entitlements
        COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME}.dmg ${PROJECT_NAME}-${Charm_VERSION}.dmg # buildbot tries to always copy Charm.dmg too for currently unknown reasons, so we copy instead of rename
        COMMAND ${CMAKE_COMMAND} -E echo "Generated package file: Charm/${PROJECT_NAME}-${Charm_VERSION}.dmg" # needed so buildbot picks up the right filename to move the package to the download site
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Charm
    )
ENDIF()

install( FILES charm.categories DESTINATION ${SYSCONF_INSTALL_DIR}/xdg )

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES)
