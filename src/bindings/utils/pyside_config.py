#############################################################################
##
## Copyright (C) 2019 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing/
##
## This file is part of the Qt for Python examples of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:BSD$
## You may use this file under the terms of the BSD license as follows:
##
## "Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are
## met:
##   * Redistributions of source code must retain the above copyright
##     notice, this list of conditions and the following disclaimer.
##   * Redistributions in binary form must reproduce the above copyright
##     notice, this list of conditions and the following disclaimer in
##     the documentation and/or other materials provided with the
##     distribution.
##   * Neither the name of The Qt Company Ltd nor the names of its
##     contributors may be used to endorse or promote products derived
##     from this software without specific prior written permission.
##
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
## A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
## OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
## SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
## LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
## DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
## THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
##
## $QT_END_LICENSE$
##
#############################################################################

from distutils import sysconfig
from enum import Enum
import glob
import os
import re
import sys


PYSIDE = 'pyside6'
PYSIDE_MODULE = 'PySide6'
SHIBOKEN = 'shiboken6'


class Package(Enum):
    SHIBOKEN_MODULE = 1
    SHIBOKEN_GENERATOR = 2
    PYSIDE_MODULE = 3


generic_error = ('Did you forget to activate your virtualenv? Or perhaps'
                 f' you forgot to build / install {PYSIDE_MODULE} into your currently active Python'
                 ' environment?')
pyside_error = f'Unable to locate {PYSIDE_MODULE}. {generic_error}'
shiboken_module_error = f'Unable to locate {SHIBOKEN}-module. {generic_error}'
shiboken_generator_error = f'Unable to locate shiboken-generator. {generic_error}'
pyside_libs_error = f'Unable to locate the PySide shared libraries.  {generic_error}'
python_link_error = 'Unable to locate the Python library for linking.'
python_include_error = 'Unable to locate the Python include headers directory.'

options = []

# option, function, error, description
options.append(("--shiboken-module-path",
                lambda: find_shiboken_module(),
                shiboken_module_error,
                "Print shiboken module location"))
options.append(("--shiboken-generator-path",
                lambda: find_shiboken_generator(),
                shiboken_generator_error,
                "Print shiboken generator location"))
options.append(("--pyside-path", lambda: find_pyside(), pyside_error,
                f"Print {PYSIDE_MODULE} location"))

options.append(("--python-include-path",
                lambda: get_python_include_path(),
                python_include_error,
                "Print Python include path"))
options.append(("--shiboken-generator-include-path",
                lambda: get_package_include_path(Package.SHIBOKEN_GENERATOR),
                pyside_error,
                "Print shiboken generator include paths"))
options.append(("--pyside-include-path",
                lambda: get_package_include_path(Package.PYSIDE_MODULE),
                pyside_error,
                "Print PySide6 include paths"))

options.append(("--python-link-flags-qmake", lambda: python_link_flags_qmake(), python_link_error,
                "Print python link flags for qmake"))
options.append(("--python-link-flags-cmake", lambda: python_link_flags_cmake(), python_link_error,
                "Print python link flags for cmake"))

options.append(("--shiboken-module-qmake-lflags",
                lambda: get_package_qmake_lflags(Package.SHIBOKEN_MODULE), pyside_error,
                "Print shiboken6 shared library link flags for qmake"))
options.append(("--pyside-qmake-lflags",
                lambda: get_package_qmake_lflags(Package.PYSIDE_MODULE), pyside_error,
                "Print PySide6 shared library link flags for qmake"))

options.append(("--shiboken-module-shared-libraries-qmake",
                lambda: get_shared_libraries_qmake(Package.SHIBOKEN_MODULE), pyside_libs_error,
                "Print paths of shiboken shared libraries (.so's, .dylib's, .dll's) for qmake"))
options.append(("--shiboken-module-shared-libraries-cmake",
                lambda: get_shared_libraries_cmake(Package.SHIBOKEN_MODULE), pyside_libs_error,
                "Print paths of shiboken shared libraries (.so's, .dylib's, .dll's) for cmake"))

options.append(("--pyside-shared-libraries-qmake",
                lambda: get_shared_libraries_qmake(Package.PYSIDE_MODULE), pyside_libs_error,
                "Print paths of f{PYSIDE_MODULE} shared libraries (.so's, .dylib's, .dll's) for qmake"))
options.append(("--pyside-shared-libraries-cmake",
                lambda: get_shared_libraries_cmake(Package.PYSIDE_MODULE), pyside_libs_error,
                f"Print paths of {PYSIDE_MODULE} shared libraries (.so's, .dylib's, .dll's) for cmake"))

options_usage = ''
for i, (flag, _, _, description) in enumerate(options):
    options_usage += f'    {flag:<45} {description}'
    if i < len(options) - 1:
        options_usage += '\n'

usage = f"""
Utility to determine include/link options of shiboken/PySide and Python for qmake/CMake projects
that would like to embed or build custom shiboken/PySide bindings.

Usage: pyside_config.py [option]
Options:
{options_usage}
    -a                                            Print all options and their values
    --help/-h                                     Print this help
"""

option = sys.argv[1] if len(sys.argv) == 2 else '-a'
if option == '-h' or option == '--help':
    print(usage)
    sys.exit(0)


def clean_path(path):
    return path if sys.platform != 'win32' else path.replace('\\', '/')


def shared_library_suffix():
    if sys.platform == 'win32':
        return 'lib'
    elif sys.platform == 'darwin':
        return 'dylib'
    # Linux
    else:
        return 'so.*'


def import_suffixes():
    import importlib.machinery
    return importlib.machinery.EXTENSION_SUFFIXES


def is_debug():
    debug_suffix = '_d.pyd' if sys.platform == 'win32' else '_d.so'
    return any([s.endswith(debug_suffix) for s in import_suffixes()])


def shared_library_glob_pattern():
    glob = '*.' + shared_library_suffix()
    return glob if sys.platform == 'win32' else 'lib' + glob


def filter_shared_libraries(libs_list):
    def predicate(lib_name):
        basename = os.path.basename(lib_name)
        if 'shiboken' in basename or 'pyside6' in basename:
            return True
        return False
    result = [lib for lib in libs_list if predicate(lib)]
    return result


# Return qmake link option for a library file name
def link_option(lib):
    # On Linux:
    # Since we cannot include symlinks with wheel packages
    # we are using an absolute path for the libpyside and libshiboken
    # libraries when compiling the project
    baseName = os.path.basename(lib)
    link = ' -l'
    if sys.platform in ['linux', 'linux2']: # Linux: 'libfoo.so' -> '/absolute/path/libfoo.so'
        link = lib
    elif sys.platform in ['darwin']: # Darwin: 'libfoo.so' -> '-lfoo'
        link += os.path.splitext(baseName[3:])[0]
    else: # Windows: 'libfoo.dll' -> 'libfoo.dll'
        link += os.path.splitext(baseName)[0]
    return link


# Locate PySide6 via sys.path package path.
def find_pyside():
    return find_package_path(PYSIDE_MODULE)


def find_shiboken_module():
    return find_package_path(SHIBOKEN)


def find_shiboken_generator():
    return find_package_path(f"{SHIBOKEN}_generator")


def find_package(which_package):
    if which_package == Package.SHIBOKEN_MODULE:
        return find_shiboken_module()
    if which_package == Package.SHIBOKEN_GENERATOR:
        return find_shiboken_generator()
    if which_package == Package.PYSIDE_MODULE:
        return find_pyside()
    return None


def find_package_path(dir_name):
    for p in sys.path:
        if 'site-' in p:
            package = os.path.join(p, dir_name)
            if os.path.exists(package):
                return clean_path(os.path.realpath(package))
    return None


# Return version as "3.6"
def python_version():
    return str(sys.version_info[0]) + '.' + str(sys.version_info[1])


def get_python_include_path():
    return sysconfig.get_python_inc()


def python_link_flags_qmake():
    flags = python_link_data()
    if sys.platform == 'win32':
        libdir = flags['libdir']
        # This will add the "~1" shortcut for directories that
        # contain white spaces
        # e.g.: "Program Files" to "Progra~1"
        for d in libdir.split("\\"):
            if " " in d:
                libdir = libdir.replace(d, d.split(" ")[0][:-1]+"~1")
        lib_flags = flags['lib']
        return f'-L{libdir} -l{lib_flags}'
    elif sys.platform == 'darwin':
        libdir = flags['libdir']
        lib_flags = flags['lib']
        return f'-L{libdir} -l{lib_flags}'
    else:
        # Linux and anything else
        libdir = flags['libdir']
        lib_flags = flags['lib']
        return f'-L{libdir} -l{lib_flags}'


def python_link_flags_cmake():
    flags = python_link_data()
    libdir = flags['libdir']
    lib = re.sub(r'.dll$', '.lib', flags['lib'])
    return f'{libdir};{lib}'


def python_link_data():
    # @TODO Fix to work with static builds of Python
    libdir = sysconfig.get_config_var('LIBDIR')
    if libdir is None:
        libdir = os.path.abspath(os.path.join(
            sysconfig.get_config_var('LIBDEST'), "..", "libs"))
    version = python_version()
    version_no_dots = version.replace('.', '')

    flags = {}
    flags['libdir'] = libdir
    if sys.platform == 'win32':
        suffix = '_d' if is_debug() else ''
        flags['lib'] = f'python{version_no_dots}{suffix}'

    elif sys.platform == 'darwin':
        flags['lib'] = f'python{version}'

    # Linux and anything else
    else:
        flags['lib'] = f'python{version}{sys.abiflags}'

    return flags


def get_package_include_path(which_package):
    package_path = find_package(which_package)
    if package_path is None:
        return None

    includes = f"{package_path}/include"

    return includes


def get_package_qmake_lflags(which_package):
    package_path = find_package(which_package)
    if package_path is None:
        return None

    link = f"-L{package_path}"
    glob_result = glob.glob(os.path.join(package_path, shared_library_glob_pattern()))
    for lib in filter_shared_libraries(glob_result):
        link += ' '
        link += link_option(lib)
    return link


def get_shared_libraries_data(which_package):
    package_path = find_package(which_package)
    if package_path is None:
        return None

    glob_result = glob.glob(os.path.join(package_path, shared_library_glob_pattern()))
    filtered_libs = filter_shared_libraries(glob_result)
    libs = []
    if sys.platform == 'win32':
        for lib in filtered_libs:
            libs.append(os.path.realpath(lib))
    else:
        for lib in filtered_libs:
            libs.append(lib)
    return libs


def get_shared_libraries_qmake(which_package):
    libs = get_shared_libraries_data(which_package)
    if libs is None:
        return None

    if sys.platform == 'win32':
        if not libs:
            return ''
        dlls = ''
        for lib in libs:
            dll = os.path.splitext(lib)[0] + '.dll'
            dlls += dll + ' '

        return dlls
    else:
        libs_string = ''
        for lib in libs:
            libs_string += lib + ' '
        return libs_string


def get_shared_libraries_cmake(which_package):
    libs = get_shared_libraries_data(which_package)
    result = ';'.join(libs)
    return result


print_all = option == "-a"
for argument, handler, error, _ in options:
    if option == argument or print_all:
        handler_result = handler()
        if handler_result is None:
            sys.exit(error)

        line = handler_result
        if print_all:
            line = f"{argument:<40}: {line}"
        print(line)
