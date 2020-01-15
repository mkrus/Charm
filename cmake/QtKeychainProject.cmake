# Copyright (c) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set(qtkeychain_root ${CMAKE_CURRENT_BINARY_DIR}/qtkeychain-install)
set(QtKeychainProject_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/QtKeychainProject-prefix/)
set(QtKeychainProject_BUILD_DIR ${qtkeychain_root}/src/QtKeychainProject-build)

set(QTKEYCHAIN_LIBRARY "${qtkeychain_root}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}qt5keychain${CMAKE_SHARED_LIBRARY_SUFFIX}")
list(APPEND QTKEYCHAIN_LIBRARIES ${QTKEYCHAIN_LIBRARY})
set(QTKEYCHAIN_INCLUDE_DIRS "${qtkeychain_root}/include")

message(STATUS "Building qtkeychain library: ${QTKEYCHAIN_LIBRARY}")

ExternalProject_Add(QtKeychainProject
    GIT_REPOSITORY https://github.com/frankosterfeld/qtkeychain
    GIT_TAG v0.9.1
    GIT_SHALLOW 1

    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${qtkeychain_root}
        -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        -DCMAKE_POSITION_INDEPENDENT_CODE=on
        -DCMAKE_OSX_DEPLOYMENT_TARGET=10.10
        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}

        # disable tests, not needed
        -DBUILD_TEST_APPLICATION=OFF
        # disable translations
        -DBUILD_TRANSLATIONS=OFF

    BUILD_BYPRODUCTS "${QTKEYCHAIN_LIBRARY}"
)

install(DIRECTORY ${qtkeychain_root}/ DESTINATION ${CMAKE_INSTALL_PREFIX})
