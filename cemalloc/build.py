#!/usr/bin/env python3
# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

import argparse
import multiprocessing
import os
import shutil
import subprocess
import sys


class HmsdkVersion:
    Major = 1
    Minor = 1


class OptionUtil:
    mode_list = {"build": ["b"], "test": ["t"]}

    build_type = {"debug": ["d"], "release": ["r"], "clean": ["c"]}

    build_target = {
        "all": ["a"],
        "malloc": ["m"],
        "cemalloc": ["c"],
        "jemalloc": ["j"],
        "cemalloc_python": ["p"],
        "cemalloc_java": ["v"],
    }

    @staticmethod
    def expand_alias(value, aliases_dict):
        if value in aliases_dict:
            return value
        for key, aliases_list in aliases_dict.items():
            if value in aliases_list:
                return key
        raise Exception("No alias found for '{}' in '{}'".format(value, aliases_dict))


class Option:
    def __init__(self):
        self._build = False
        self._clean = False
        self._cemalloc = False
        self._jemalloc = False
        self._cemalloc_python = False
        self._cemalloc_java = False
        self._cmake_arguments = []
        self._make_arguments = []

        self._test = False
        self._test_list = []
        self._show_test = False
        self._all_test = False

    def set_build(self, build_type, target, log_level, je_prefix):
        if build_type == "clean":
            self._clean = True
            self._make_arguments.append("clean")
        else:
            self._build = True
            if build_type == "debug":
                self._cmake_arguments.append("-DCMAKE_BUILD_TYPE=debug")
                self._cmake_arguments.append("-DDISTRIBUTION=0")
            elif build_type == "release":
                self._cmake_arguments.append("-DCMAKE_BUILD_TYPE=release")
                self._cmake_arguments.append("-DDISTRIBUTION=1")
            else:
                raise Exception("Unknown mode: {}".format(build_type))

            if je_prefix:
                self._cmake_arguments.append(
                    "-DJEMALLOC_PREFIX={je_prefix}".format(je_prefix=je_prefix)
                )

            self._cmake_arguments.append("-DLOG_LEVEL={}".format(log_level))
            cpu_option = "-j {}".format(multiprocessing.cpu_count())
            self._make_arguments.append(cpu_option)

        if target == "cemalloc":
            self._cemalloc = True
        elif target == "jemalloc":
            self._jemalloc = True
        elif target == "malloc":
            self._jemalloc = True
            self._cemalloc = True
        elif target == "cemalloc_python":
            self._jemalloc = True
            self._cemalloc = True
            self._cemalloc_python = True
        elif target == "cemalloc_java":
            self._cemalloc = True
            self._jemalloc = True
            self._cemalloc_java = True
        else:  # "all"
            self._cemalloc = True
            self._jemalloc = True
            self._cemalloc_python = True
            self._cemalloc_java = True

    def is_clean(self):
        return self._clean

    def is_build(self):
        return self._build

    def target(self):
        return (
            self._jemalloc,
            self._cemalloc,
            self._cemalloc_python,
            self._cemalloc_java,
        )

    def cmake_arguments(self):
        return " ".join(self._cmake_arguments)

    def make_arguments(self):
        return " ".join(self._make_arguments)

    def set_test(self, test_list):
        if len(test_list) == 0:
            raise Exception("test_list is empty!")

        self._test = True
        if "list" in test_list:
            self._show_test = True
        elif "all" in test_list:
            self._all_test = True
        else:
            self._test_list = test_list.copy()

    def is_test(self):
        return self._test

    def is_all_test(self):
        return self._all_test

    def is_show_test(self):
        return self._show_test

    def test_list(self):
        return self._test_list


class OptionParser:
    def __init__(self):
        self.args = None
        self._option = Option()

    def run_parser(self):
        parser = argparse.ArgumentParser(description="cemalloc")
        parser.add_argument(
            "--mode",
            type=str,
            nargs="?",
            default="build",
            help="select mode [{}]".format(", ".join(OptionUtil.mode_list.keys())),
        )
        parser.add_argument(
            "--build_type",
            type=str,
            nargs="?",
            default="release",
            help="select build type [{}]".format(
                ", ".join(OptionUtil.build_type.keys())
            ),
        )
        parser.add_argument(
            "--build_target",
            type=str,
            nargs="?",
            default="malloc",
            help="select build target [{}]".format(
                ", ".join(OptionUtil.build_target.keys())
            ),
        )
        parser.add_argument(
            "--run_test",
            nargs="?",
            default="list",
            help="run compiled gtests, "
            "names of tests can be specified ('all' to run all)",
        )
        parser.add_argument(
            "--je_prefix", type=str, nargs="?", default="ce", help="set jemalloc prefix"
        )
        parser.add_argument(
            "--log_level",
            type=int,
            nargs="?",
            default=1,
            help="select log level "
            "(LOG_ERROR=0, LOG_WARN=1, LOG_INFO=2, LOG_DEBUG=3, "
            "LOG_VERBOSE=4)",
        )

        self.args = parser.parse_args()
        self._pre_processing()
        return self._option

    def _pre_processing(self):
        args = self.args
        mode_list = []
        for mode in args.mode.split(","):
            mode_list.append(
                OptionUtil.expand_alias(mode.lower(), OptionUtil.mode_list)
            )
        build_type = OptionUtil.expand_alias(
            args.build_type.lower(), OptionUtil.build_type
        )
        build_target = OptionUtil.expand_alias(
            args.build_target.lower(), OptionUtil.build_target
        )
        test_list = args.run_test.split(",")

        log_level = args.log_level

        je_prefix = args.je_prefix

        if "build" in mode_list:
            self._option.set_build(
                build_type, build_target, log_level, je_prefix
            )
        if "test" in mode_list:
            self._option.set_test(test_list)


class Shell:
    @staticmethod
    def run(cmd):
        """!@brief perform bash command"""
        print(cmd)
        process = subprocess.Popen(cmd, shell=True)
        process.wait()
        if process.returncode != 0:
            print(process.returncode)
            sys.exit(-1)


class BuildManager:
    def __init__(self, parser, option):
        self._option: Option = option
        self._parser: OptionParser = parser
        self._build = self._option.is_build()
        self._clean = self._option.is_clean()

        if self._build or self._clean:
            (
                self._jemalloc,
                self._cemalloc,
                self._cemalloc_python,
                self._cemalloc_java,
            ) = self._option.target()
            self.root_path = os.path.dirname(os.path.realpath(__file__))
            self.build_path = os.path.join(self.root_path, "out")
            self.jemalloc_path = "externals/jemalloc"
            self.prefix = self._parser.args.je_prefix
            self.jemalloc_prefix = "{prefix}_je_".format(prefix=self.prefix)
            self.jemalloc_library = "lib/lib{prefix}malloc_pic.a".format(
                prefix=self.prefix
            )
            self.jemalloc_include = "include/jemalloc/jemalloc.h"
            self.external_jemalloc = os.path.join(self.root_path, "externals/jemalloc")
            self.external_include = os.path.join(
                self.root_path, "externals/include/jemalloc.h"
            )
            os.makedirs(self.build_path, exist_ok=True)

    def _jemalloc_build(self):
        os.makedirs(os.path.join(self.build_path, "lib"), exist_ok=True)
        os.chdir(os.path.join(self.root_path, self.jemalloc_path))

        if not os.path.exists("Makefile"):
            # add je_ prefix and override mmap of jemalloc to cxl_mmap.
            cmd = "./autogen.sh CFLAGS=-Dmmap=cxl_mmap \
            --with-jemalloc-prefix={jemalloc_prefix}".format(
                jemalloc_prefix=self.jemalloc_prefix
            )
            Shell.run(cmd)

        # replace library name from libjemalloc to libcemalloc.
        cmd = "make LIBJEMALLOC=libcemalloc -j {core_count} {library}".format(
            core_count=multiprocessing.cpu_count(), library=self.jemalloc_library
        )
        Shell.run(cmd)

        if os.path.exists(self.jemalloc_library):
            shutil.copy(self.jemalloc_library, os.path.join(self.build_path, "lib"))
        else:
            sys.exit(
                "lib{prefix}malloc_pic.a has not been created. \
                    Possible jemalloc build fail".format(
                    prefix=self.prefix
                )
            )

    def _cemalloc_build(self):
        os.chdir(self.build_path)
        cmake_cmd = "cmake .. {}".format(self._option.cmake_arguments())
        Shell.run(cmake_cmd)
        make_cmd = "make {}".format(self._option.make_arguments())
        Shell.run(make_cmd)

    def _cemalloc_python_build(self):
        dist_dir = os.path.join(self.build_path, "cemalloc_python")
        os.makedirs(dist_dir, exist_ok=True)
        setup_dir = os.path.join(self.root_path, "cemalloc/python")
        os.chdir(setup_dir)
        cmd = (
            "python3 setup.py "
            "build --build-base {} "
            "egg_info --egg-base {} "
            "bdist_wheel --dist-dir {}".format(dist_dir, dist_dir, dist_dir)
        )
        Shell.run(cmd)

    def _cemalloc_java_build(self):
        java_build_dir = os.path.join(self.build_path, "cemalloc_java/")
        java_dir = os.path.join(self.root_path, "cemalloc/java/")
        os.makedirs(java_build_dir, exist_ok=True)
        # Make jar file
        os.chdir(java_dir)
        make_clean_cmd = "make clean -C {}".format(java_dir + "src/")
        Shell.run(make_clean_cmd)
        make_cmd = "make -C {} -j".format(java_dir + "src/")
        Shell.run(make_cmd)
        jar_cmd = "jar cvf {} cemalloc".format(java_build_dir + "cemalloc.jar")
        Shell.run(jar_cmd)

        # Create jni header file
        cmd = "unset CLASSPATH"
        Shell.run(cmd)
        cmd = "cp src/Cemalloc.java cemalloc/"
        Shell.run(cmd)
        javac_cmd = "javac -h . cemalloc/Cemalloc.java"
        Shell.run(javac_cmd)
        cmd = "rm cemalloc/Cemalloc.java"
        Shell.run(cmd)
        cmd = "mv cemalloc_Cemalloc.h jni/"
        Shell.run(cmd)

        # Build jni library
        os.chdir(java_build_dir)
        cmake_cmd = "cmake {}".format(java_dir + "jni/")
        Shell.run(cmake_cmd)
        make_cmd = "make -j"
        Shell.run(make_cmd)
        # Make jar file
        os.chdir(java_dir)
        make_cmd = "make -C {} -j".format(java_dir + "src/")
        Shell.run(make_cmd)
        jar_cmd = "jar cvf {} cemalloc".format(java_build_dir + "cemalloc.jar")
        Shell.run(jar_cmd)

    def _run_build(self):
        if self._jemalloc:
            self._jemalloc_build()
        if self._cemalloc:
            self._cemalloc_build()
        if self._cemalloc_python:
            self._cemalloc_python_build()
        if self._cemalloc_java:
            self._cemalloc_java_build()

    def _jemalloc_clean(self):
        if os.path.exists(self.external_include):
            os.remove(self.external_include)

        jemalloc_lib_dir = os.path.join(self.build_path, "lib")
        if os.path.exists(jemalloc_lib_dir):
            shutil.rmtree(jemalloc_lib_dir)

        os.chdir(self.external_jemalloc)
        cmd = "make clean"
        Shell.run(cmd)

    def _cemalloc_clean(self):
        os.chdir(self.build_path)
        make_cmd = "make clean"
        Shell.run(make_cmd)

    def _run_clean(self):
        if self._jemalloc:
            self._jemalloc_clean()
        if self._cemalloc:
            self._cemalloc_clean()

    def run(self):
        if self._build:
            self._run_build()
            self.generate_package()
        elif self._clean:
            self._run_clean()

    def _generate_cemalloc_package(self, package_root):
        cemalloc_build_path = self.build_path + "/cemalloc/core/"
        cemalloc_name = "libcemalloc.so.{}.{}".format(
            HmsdkVersion.Major, HmsdkVersion.Minor
        )
        cemalloc_link_name = "libcemalloc.so"
        shutil.copy(os.path.join(cemalloc_build_path, cemalloc_name), package_root)
        lib_path = package_root + "/" + cemalloc_link_name
        lib_path = os.path.normpath(lib_path)
        if os.path.exists(lib_path) and os.path.islink(lib_path):
            pass
        else:
            os.symlink(cemalloc_name, lib_path)

    def _generate_python_package(self, package_root):
        whl_path = self.build_path + "/cemalloc_python/*.whl"
        cmd = "cp -f {} {}".format(whl_path, package_root)
        Shell.run(cmd)

    def _generate_java_package(self, package_root):
        lib_path = self.build_path + "/cemalloc_java/libcemallocjava.so"
        cmd = "cp -f {} {}".format(lib_path, package_root)
        Shell.run(cmd)
        jar_path = self.build_path + "/cemalloc_java/cemalloc.jar"
        cmd = "cp -f {} {}".format(jar_path, package_root)
        Shell.run(cmd)

    def _generate_header_package(self, package_root):
        header_path = self.root_path + "/cemalloc/include/*.h"
        package_header_path = package_root + "/include"
        os.makedirs(package_header_path, exist_ok=True)
        cmd = "cp -f {} {}".format(header_path, package_header_path)
        Shell.run(cmd)

    def generate_package(self):
        package_root = os.path.join(self.root_path, "cemalloc_package")
        if os.path.exists(package_root) == False:
            os.makedirs(package_root, exist_ok=True)

        if self._cemalloc or self._cemalloc_python or self._cemalloc_java:
            self._generate_cemalloc_package(package_root)
            self._generate_header_package(package_root)
        if self._cemalloc_python:
            self._generate_python_package(package_root)
        if self._cemalloc_java:
            self._generate_java_package(package_root)


class TestManager:
    def __init__(self, option):
        self._option: Option = option
        self._valid = self._option.is_test()

        if self._valid:
            root_path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
            build_path = os.path.join(root_path, "out")
            self._test_path = os.path.join(build_path, "tests")
            if not os.path.exists(self._test_path):
                raise Exception("out/tests directory does not exist.")

            self._all_test = self._option.is_all_test()
            self._show_test = self._option.is_show_test()
            self._test_list = self._option.test_list()

    def _run_show_test(self):
        print("<< TestCase List >>")
        for binary in os.listdir(self._test_path):
            if os.path.isfile(os.path.join(self._test_path, binary)) and os.access(
                os.path.join(self._test_path, binary), os.X_OK
            ):
                print("- {}".format(binary))

    def _run_test(self, test_list=[]):
        for binary in os.listdir(self._test_path):
            if os.path.isfile(os.path.join(self._test_path, binary)) and os.access(
                os.path.join(self._test_path, binary), os.X_OK
            ):
                if len(test_list) == 0 or (len(test_list) != 0 and binary in test_list):
                    cmd = os.path.join(self._test_path, binary)
                    Shell.run(cmd)

    def run(self):
        if not self._valid:
            return

        if self._show_test:
            self._run_show_test()
        elif self._all_test:
            self._run_test()
        else:
            self._run_test(self._test_list)


def main():
    parser = OptionParser()
    option = parser.run_parser()
    build_mgr = BuildManager(parser, option)
    build_mgr.run()
    test_mgr = TestManager(option)
    test_mgr.run()


if __name__ == "__main__":
    main()
