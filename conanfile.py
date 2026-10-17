"""Conan 2 consumer recipe for local LabelStudioDesktop builds.

Qt is managed by Conan. This recipe does not package or deploy the application.
In CMakeLists.txt, replace the QT_TRANSLATION_DIR get_filename_component line
with: set(QT_TRANSLATION_DIR "" CACHE PATH "Qt translation catalog directory")
"""

import os

from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv


class LabelStudioDesktopConan(ConanFile):
    required_conan_version = ">=2.0"
    settings = "os", "arch", "compiler", "build_type"

    options = {"with_tests": [True, False]}
    default_options = {
        "with_tests": True,
        "qt/*:shared": True,
        "qt/*:gui": True,
        # Keep Widgets enabled for the Qt Tools build.
        "qt/*:widgets": True,
        # QML, Quick and QuickControls2 are provided by qtdeclarative in Qt 6.
        "qt/*:qtdeclarative": True,
        "qt/*:qtshadertools": True,
        "qt/*:qttools": True,
        "qt/*:qttranslations": True,
        "qt/*:openssl": True,
        # This application does not use Qt SQL database drivers.
        "qt/*:with_pq": False,
        "qt/*:with_odbc": False,
    }

    def requirements(self):
        # Available in the current ConanCenter recipe; satisfies Qt >= 6.6.3.
        self.requires("qt/6.8.3")

    def layout(self):
        cmake_layout(self)

    def validate(self):
        check_min_cppstd(self, "17")

    def generate(self):
        CMakeDeps(self).generate()

        toolchain = CMakeToolchain(self)
        toolchain.variables["BUILD_TESTING"] = bool(self.options.with_tests)
        toolchain.variables["QT_TRANSLATION_DIR"] = os.path.join(
            self.dependencies["qt"].package_folder, "translations"
        ).replace("\\", "/")
        toolchain.generate()

        # Qt DLLs/plugins and build tools are available in the activated envs.
        VirtualBuildEnv(self).generate()
        VirtualRunEnv(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
