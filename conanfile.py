import os
from conans import ConanFile, CMake, tools


class CharmConan(ConanFile):
    name = "charm"
    version = "1.13.0"
    license = "GPLv2"
    author = "KDAB <info@kdab.com>"
    url = "ssh://codereview.kdab.com:29418/Charm"
    description = "The Cross-Platform Time Tracker"
    topics = ("qt", "timetracker")
    settings = "os", "compiler"
    requires = ["qtkeychain/[>=0.10.0]@kdab/stable"]
    default_options = {"qtkeychain:shared": False}
    generators = "cmake"

    exports = ["*"]

    no_copy_source = True

    def build(self):
        cmake = CMake(self)
        cmake.configure(args=["-DCHARM_PREPARE_DEPLOY=ON",
                              "-DUPDATE_CHECK_URL=https://updates.kdab.com/charm/updates.xml"])
        cmake.build()
        cmake.install()
        cmake.test()
