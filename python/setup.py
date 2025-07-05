import os
import sys
import re
import shutil
import pathlib
import platform
import glob
import setuptools

from distutils import log
from distutils import sysconfig
from setuptools import Extension
from setuptools.command.build_ext import build_ext
from setuptools.command.install_scripts import install_scripts


project_name = "yup"
root_dir = os.path.dirname(os.path.abspath(__file__))


def get_environment_option(typeClass, name, default=None):
    try:
        return typeClass(os.environ.get(name, default))
    except ValueError:
        return default


def glob_python_library(path):
    for extension in [".a", ".lib", ".so", ".dylib", ".dll", ".pyd"]:
        for m in glob.iglob(f"{path}/**/*python*{extension}", recursive=True):
            if "site-packages" not in m:
                return m
    return None


def get_python_path():
    vars = sysconfig.get_config_vars()

    if 'LIBPL' in vars and 'LIBRARY' in vars:
        path = os.path.join(vars['LIBPL'], vars['LIBRARY'])
        if os.path.exists(path):
            return path

    if 'SCRIPTDIR' in vars:
        srcdir = vars['SCRIPTDIR']
    elif 'srcdir' in vars:
        srcdir = vars['srcdir']
    else:
        srcdir = None

    if srcdir:
        path = glob_python_library(srcdir)
        if path and os.path.exists(path):
            return path

    if 'LIBDEST' in vars:
        path = vars['LIBDEST']
        if sys.platform in ["win32", "cygwin"]:
            path = os.path.split(os.path.split(path)[0])[0]

        path = glob_python_library(path)
        if path and os.path.exists(path):
            return path

    log.error("cannot find static library to be linked")
    exit(-1)


def get_python_includes_path():
    include_dir = sysconfig.get_config_var('CONFINCLUDEPY')
    if include_dir and os.path.exists(include_dir):
        return include_dir
    return sysconfig.get_config_var('INCLUDEPY')


def get_python_lib_path():
    return os.path.dirname(get_python_path())


class CMakeExtension(Extension):
    def __init__(self, name):
        super().__init__(name, sources=[])


class CMakeBuildExtension(build_ext):
    build_for_coverage = get_environment_option(int, "YUP_ENABLE_COVERAGE", 0)
    build_for_distribution = get_environment_option(int, "YUP_ENABLE_DISTRIBUTION", 0)
    build_with_lto = get_environment_option(int, "YUP_ENABLE_LTO", 0)
    build_osx_architectures = get_environment_option(str, "YUP_OSX_ARCHITECTURES", "arm64")
    build_osx_deployment_target = get_environment_option(str, "YUP_OSX_DEPLOYMENT_TARGET", "11.0")

    def build_extension(self, ext):
        log.info("building with cmake")

        cwd = pathlib.Path().absolute()

        build_temp = pathlib.Path(self.build_temp)
        build_temp.mkdir(parents=True, exist_ok=True)

        extdir = pathlib.Path(self.get_ext_fullpath(ext.name)).absolute()
        extdir.mkdir(parents=True, exist_ok=True)

        output_path = extdir.parent
        output_path.mkdir(parents=True, exist_ok=True)

        config = "Debug" if self.debug or self.build_for_coverage else "Release"
        cmake_args = [
            f"-DYUP_BUILD_WHEEL:BOOL=ON",
            f"-DYUP_EXPORT_MODULES:BOOL=OFF",
            f"-DYUP_BUILD_EXAMPLES:BOOL=OFF",
            f"-DYUP_BUILD_TESTS:BOOL=OFF",
            f"-DCMAKE_BUILD_TYPE:STRING={config}",
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH={output_path}",
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{config.upper()}:PATH={output_path}",
            f"-DPython_ROOT_DIR:PATH={sys.exec_prefix}",
            f"-DPython_INCLUDE_DIRS:PATH={get_python_includes_path()}",
            f"-DPython_LIBRARY_DIRS:PATH={get_python_lib_path()}"
        ]

        if platform.system() == 'Darwin':
            cmake_args += [
                f"-DCMAKE_OSX_ARCHITECTURES:STRING={self.build_osx_architectures}",
                f"-DCMAKE_OSX_DEPLOYMENT_TARGET:STRING={self.build_osx_deployment_target}"
            ]

        if self.build_for_coverage:
            cmake_args += ["-DYUP_ENABLE_COVERAGE:BOOL=ON"]

        if self.build_for_distribution:
            cmake_args += ["-DYUP_ENABLE_DISTRIBUTION:BOOL=ON"]

        if self.build_with_lto:
            cmake_args += ["-DYUP_ENABLE_LTO:BOOL=ON"]

        try:
            os.chdir(str(build_temp))
            make_command = ["cmake", str(os.path.join(cwd, ".."))] + cmake_args
            self.spawn(make_command)

            if getattr(self, "dry_run"): return

            build_command = ["cmake", "--build", ".", "--config", config]
            if sys.platform not in ["win32", "cygwin"]:
                build_command += ["--", f"-j{os.cpu_count()}"]
            self.spawn(build_command)

            if self.build_for_coverage:
                self.generate_coverage(cwd)

        finally:
            os.chdir(str(cwd))

        self.generate_pyi(cwd)

    def generate_coverage(self, cwd):
        log.info("generating coverage files")

        self.spawn([sys.executable, "-m", "pytest", "-s", os.path.join(cwd, "tests")])
        self.spawn(["lcov", "--directory", cwd, "--capture", "--output-file", "coverage/coverage.info",
                    "--ignore-errors", "mismatch,gcov,source,negative,unused,empty,format,corrupt"])

        if not os.path.isdir("/host"): # We are not running in cibuildwheel container
            return

        for m in glob.iglob(f"{cwd}/**/coverage.info", recursive=True):
            log.info(f"found {m} coverage info file")

            self.spawn(["sed", "-i", "s:/project/::g", m])

            os.makedirs("/output", exist_ok=True)
            shutil.copyfile(m, f"/output/lcov.info")

            break

    def generate_pyi(self, cwd):
        log.info("generating pyi files")

        library = None
        for extension in [".so", ".pyd"]:
            for m in glob.iglob(f"{cwd}/**/{project_name}{extension}", recursive=True):
                library = m
                break

        if library is None:
            return

        library_dir = os.path.dirname(library)
        temp_pyi_dir = os.path.join(library_dir, project_name)
        final_pyi_dir = os.path.join(root_dir, project_name)
        final_pyi_file = os.path.join(root_dir, f"{project_name}.pyi")

        try:
            os.chdir(library_dir)

            shutil.rmtree(temp_pyi_dir, ignore_errors=True)
            shutil.rmtree(final_pyi_dir, ignore_errors=True)

            self.spawn(["stubgen", "--output", library_dir, "-p", project_name])

            if os.path.isdir(project_name):
                shutil.copytree(project_name, final_pyi_dir)

            if os.path.isfile(f"{project_name}.pyi"):
                shutil.copyfile(f"{project_name}.pyi", final_pyi_file)

        finally:
            os.chdir(str(cwd))


class CustomInstallScripts(install_scripts):
    def run(self):
        install_scripts.run(self)

        log.info("cleaning up pyi files")

        final_pyi_dir = os.path.join(root_dir, project_name)
        if os.path.isdir(final_pyi_dir):
            shutil.rmtree(final_pyi_dir, ignore_errors=True)

        final_pyi_file = os.path.join(root_dir, f"{project_name}.pyi")
        if os.path.isfile(final_pyi_file):
            os.remove(final_pyi_file)


def load_description(version):
    with open("../README.md", mode="r", encoding="utf-8") as f:
        long_description = f.read()

    #long_description = re.sub(
    #    r"`([^`>]+)\s<((?!https)[^`>]+)>`_",
    #    fr"`\1 <https://github.com/kunitoki/yup/tree/v{version}/\2>`_",
    #    long_description)

    #long_description = re.sub(
    #    r"image:: ((?!https).*)",
    #    fr"image:: https://raw.githubusercontent.com/kunitoki/yup/v{version}/\1",
    #    long_description)

    #long_description = re.sub(
    #    r":target: ((?!https).*)",
    #    fr":target: https://github.com/kunitoki/yup/tree/v{version}/\1",
    #    long_description)

    return long_description


with open("../modules/yup_core/system/yup_StandardHeader.h", mode="r", encoding="utf-8") as f:
    content = f.read()
    version_major = re.findall(r"YUP_MAJOR_VERSION\s+(\d+)", content)[0]
    version_minor = re.findall(r"YUP_MINOR_VERSION\s+(\d+)", content)[0]
    version_build = re.findall(r"YUP_BUILDNUMBER\s+(\d+)", content)[0]
    version = f"{version_major}.{version_minor}.{version_build}"


if platform.system() == 'Darwin':
    build_osx_architectures = get_environment_option(str, "YUP_OSX_ARCHITECTURES", "arm64")
    build_osx_deployment_target = get_environment_option(str, "YUP_OSX_DEPLOYMENT_TARGET", "11.0")
    if "arm64" in build_osx_architectures and "x86_64" in build_osx_architectures:
        os.environ["_PYTHON_HOST_PLATFORM"] = f"macosx-{build_osx_deployment_target}-universal2"
    elif "arm64" in build_osx_architectures:
        os.environ["_PYTHON_HOST_PLATFORM"] = f"macosx-{build_osx_deployment_target}-arm64"
    elif "x86_64" in build_osx_architectures:
        os.environ["_PYTHON_HOST_PLATFORM"] = f"macosx-{build_osx_deployment_target}-x86_64"
    else:
        raise RuntimeError("Invalid configuration of YUP_OSX_ARCHITECTURES")


setuptools.setup(
    name=project_name,
    version=version,
    author="kunitoki",
    author_email="kunitoki@gmail.com",
    description=f"{project_name}: Python integration for YUP with pybind11.",
    long_description=load_description(version),
    long_description_content_type="text/markdown",
    url="https://github.com/kunitoki/yup",
    packages=setuptools.find_packages(".", exclude=["*cmake*", "*docs*", "*examples*", "*modules*", "*standalone*", "*tests*", "*thirdparty*", "*tools*"]),
    include_package_data=True,
    cmdclass={"build_ext": CMakeBuildExtension, "install_scripts": CustomInstallScripts},
    ext_modules=[CMakeExtension(project_name)],
    zip_safe=False,
    platforms=["macosx", "win32", "linux"],
    python_requires=">=3.11",
    license="ISC",
    classifiers=[
        "Intended Audience :: Developers",
        "Development Status :: 4 - Beta",
        "Topic :: Software Development :: Libraries :: Application Frameworks",
        "Programming Language :: C++",
        "Programming Language :: Python",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: Python :: 3.13",
        "Operating System :: MacOS :: MacOS X",
        "Operating System :: Microsoft :: Windows",
        "Operating System :: POSIX :: Linux",
        "Operating System :: Android",
        "Operating System :: iOS",
        "Operating System :: WebAssembly"
    ]
)
