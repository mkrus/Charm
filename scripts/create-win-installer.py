#!/usr/bin/env python3

import os
import subprocess
import sys
import shutil
import argparse
import glob

VCREDIST2013="none"

class DeployHelper(object):
    def __init__(self, args):
        self.args = args
        self.gitDir = os.path.realpath(os.path.join(os.path.dirname(__file__), ".."))

        self.deployImage = os.path.realpath("./deployImage")
        if not os.path.exists(self.args.buildDir):
            self._die("Build dir %s does not exist" % self.args.buildDir)


    def _die(self, message):
        print(message, file=sys.stderr)
        exit(1)

    def _logExec(self, command):
        print(command)
        sys.stdout.flush()
        print(subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT).decode("UTF-8"))

    def _copyToImage(self, src, destDir = None, deploy = True):
        if destDir is None:
            destDir = self.deployImage
        print("Copy %s to %s" % (src, destDir))
        try:
            shutil.copy(src, destDir)
        except shutil.SameFileError:
            print("Same file: %s and %s -- ignoring" % (src, destDir))
            pass

        if deploy:
            try:
                self._logExec("windeployqt --%s --dir \"%s\" --qmldir \"%s\"  \"%s\"" % (self.args.buildType, self.deployImage, self.gitDir, src))
            except subprocess.CalledProcessError as e:
                if not b"does not seem to be a Qt executable." in e.output:
                    raise e
                else:
                    print(e.output.decode("UTF-8"))

    def _sign(self, fileName):
        if self.args.sign:
            self._logExec("signtool.exe sign  -t http://timestamp.globalsign.com/scripts/timestamp.dll  -fd SHA256 -v \"%s\"" % fileName)

    def cleanImage(self):
        if os.path.exists(self.deployImage):
            shutil.rmtree(self.deployImage)
        os.makedirs(self.deployImage)

    def _locateDll(self, dllName, extraSearchDirs=None):
        print(extraSearchDirs)
        pathext = os.environ.get("PATHEXT")
        os.environ["PATHEXT"] = ".dll"
        found = shutil.which(dllName, path=os.pathsep.join([self.deployImage, os.environ.get("PATH")] + (extraSearchDirs if extraSearchDirs else [])))
        os.environ["PATHEXT"] = pathext
        if not found:
            self._die("Unable to locate %s" % dllName)
        return found

    def deploy(self):
        global VCREDIST2013
        if self.args.deployDlls:
            for dll in self.args.deployDlls.split(";"):
                self._copyToImage(self._locateDll(dll, self.args.extraSearchDirs.split(";")))

        app = os.path.join(self.args.buildDir, self.args.applicationFileName)
        shutil.copy(app, self.deployImage)
        self._logExec("windeployqt --%s --compiler-runtime --dir \"%s\" --qmldir \"%s\" \"%s\"" % (self.args.buildType, self.deployImage, self.gitDir, app))

        for folder in self.args.pluginFolders.split(";"):
            for f in glob.glob(os.path.join(self.args.buildDir, folder, "**/*.dll"), recursive=True):
                self._copyToImage(f)

        for folder in self.args.extraPluginFolders.split(";"):
            dest = os.path.join(self.deployImage, os.path.basename(folder))
            if not os.path.exists(dest):
                os.makedirs(dest)
            for f in glob.glob(os.path.join(self.args.buildDir, folder, "**/*.dll"), recursive=True):
                self._copyToImage(f, dest)

        if self.args.deployOpenSSL1:
            self._copyToImage(os.path.join(self.args.deployOpenSSL1, "libeay32.dll" ), deploy=False)
            self._copyToImage(os.path.join(self.args.deployOpenSSL1, "libssl32.dll" ), deploy=False)
            self._copyToImage(os.path.join(self.args.deployOpenSSL1, "ssleay32.dll" ), deploy=False)

        if self.args.deployOpenSSL1_1:
            self._copyToImage(os.path.join(self.args.deployOpenSSL1_1, "bin/libcrypto-1_1-x64.dll" ), deploy=False)
            self._copyToImage(os.path.join(self.args.deployOpenSSL1_1, "bin/libssl-1_1-x64.dll" ), deploy=False)

        if self.args.deployVCRedist:
            #find vcredists for various MSVC versions
            destdir = self.deployImage

            #2013
            for f in glob.glob(os.path.join(self.args.deployVCRedist, "vcredist_x64-2013*.exe"), recursive=False):
                VCREDIST2013 = os.path.basename(f)
                self._copyToImage(os.path.join(self.args.deployVCRedist, VCREDIST2013), destDir=destdir, deploy=False)
                break  #once is enough
            if not VCREDIST2013:
                print("Error: Cannot find vcredist2013 installer in " + self.args.deployVCRedist)
                exit(1)

        if self.args.sign:
            for f in glob.glob(os.path.join(self.deployImage, "**/*.exe"), recursive=True):
                self._sign(f)
            for f in glob.glob(os.path.join(self.deployImage, "**/*.dll"), recursive=True):
                self._sign(f)

    def makeInstaller(self):
        global VCREDIST2013
        if self.args.architecture == "x64":
            programDir = "$PROGRAMFILES64"
        else:
            programDir = "$PROGRAMFILES"

        defines = {}
        defines["productName"] = self.args.productName
        defines["companyName"] = self.args.companyName
        defines["productVersion"] = self.args.productVersion
        defines["setupname"] = self.args.installerName
        defines["applicationFileName"] = os.path.basename(self.args.applicationFileName)
        defines["applicationIcon"] = self.args.applicationIcon
        defines["programFilesDir"] = programDir or ""
        defines["deployDir"] = self.deployImage
        defines["productLicence"] = "" if not self.args.productLicence else "!insertmacro MUI_PAGE_LICENSE \"%s\"" % self.args.productLicence
        defines["vcredist2013"] = VCREDIST2013

        definestring = ""
        for key in defines:
            definestring += " /D%s=\"%s\"" % (key, defines[key])

        command = "makensis.exe /NOCD %s %s" %\
                  (definestring, os.path.join(os.path.dirname(__file__), "NullsoftInstaller.nsi"))
        self._logExec(command)
        installer = os.path.realpath(self.args.installerName)
        self._sign(installer)
        print("""Generated package file: %s """ % installer)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--architecture", action = "store", default="x64" )
    parser.add_argument("--buildType", action = "store", default="release" )
    parser.add_argument("--installerName", action = "store", default="setup.exe" )
    parser.add_argument("--applicationFileName", action = "store" )
    parser.add_argument("--applicationIcon", action = "store" )
    parser.add_argument("--buildDir", action = "store" )
    parser.add_argument("--pluginFolders", action = "store", default="" )
    parser.add_argument("--extraPluginFolders", action = "store", default="" )
    parser.add_argument("--productName", action = "store", default="My Product" )
    parser.add_argument("--companyName", action = "store", default="My Company" )
    parser.add_argument("--productVersion", action = "store", default="0.1" )
    parser.add_argument("--productLicence", action = "store" )
    parser.add_argument("--extraSearchDirs", action = "store" )
    parser.add_argument("--deployDlls", action = "store" )
    parser.add_argument("--deployOpenSSL1", action = "store" )
    parser.add_argument("--deployOpenSSL1_1", action = "store" )
    parser.add_argument("--deployVCRedist", action = "store" )

    parser.add_argument("--sign", action = "store_true", default=False)


    args = parser.parse_args()
    try:
        helper = DeployHelper(args)
        helper.cleanImage()
        helper.deploy()
        helper.makeInstaller()
    except subprocess.CalledProcessError as e:
        print( "Error: Process %s\n"
               "\texited with status %s" % (e.cmd, e.returncode))
        print(e.output.decode("UTF-8"))
        exit(1)
