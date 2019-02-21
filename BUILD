= Requirements =

Charm supports Qt 5.9 or higher.

The Qt modules required on all platforms are: QtWidgets, QtXml, QtSql with SQLite3 plugin, QtNetwork, QtTest, QtScript (or equivalent Qt 4 features)
Optional: QtPrintSupport

Charm code uses C++11, so using a recent enough compiler is advised (see below for details).

== Linux ==

 * g++ >= 4.8
 * CMake 3.x
 * Qt 5.9+
 * Extra Qt modules/features required: QtDBus

[1] https://build.opensuse.org/package/repositories/isv:KDAB/charmtimetracker

== OS X ==

 * A new enough clang to support the C++11 features we use. We recommend to use the latest XCode if possible. (7.0.2 right now)
 * Qt 5.9+
 * Extra Qt 5 modules required: QtMacExtras
 * CMake 3.x

== Windows ==

On Windows we require:
 * MSVC 2015 or MinGW (gcc >= 4.8; lower versions untested)
 * Qt 5.9+
 * Extra Qt 5 modules required: QtWinExtras
 * Install OpenSSL and make sure the libraries are in the PATH (TODO: add more details like suggested downloads)
 * CMake 3.x


= Build Instructions =

== Building from the Terminal ==

The build steps for a Debug build when building from the Terminal are:

cd Charm
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make # Windows: nmake, mingw32-make or jom, see below

Windows-specific hints:

 * Ensure that your compiler's environment is sourced (Call vcvarsall.bat for MSVC2013).
 * For MinGW: pass -G "MinGW Makefiles". "make" becomes "mingw32-make"
 * When using MSVC: call "nmake" instead of make.
 * You can use jom.exe (parallel builds using multiple cores) which is shipped as part of Qt Creator for both MSVC and MinGW builds

== Building in Qt Creator ==

 * Choose File -> Open File or Project
 * Select the top-level CMakeLists.txt
