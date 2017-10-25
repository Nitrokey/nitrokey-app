#!/usr/bin/env python
# Build the project on AppVeyor.

import os
from subprocess import check_call

build = os.environ['BUILD']
config = os.environ['CONFIGURATION']
platform = os.environ.get('PLATFORM')
path = os.environ['PATH']
cmake_command = ['cmake', '-DFMT_PEDANTIC=ON', '-DCMAKE_BUILD_TYPE=' + config]
        
if build == 'mingw':
    cmake_command.append('-GMinGW Makefiles')
    cmake_command.append(r'-DCMAKE_USER_MOD_PATH=C:/Qt/5.9.1/mingw53_32/lib/cmake')
    build_command = ['mingw32-make', '-j2']
    test_command = ['mingw32-make', 'test']
    # Remove the path to Git bin directory from $PATH because it breaks
    # MinGW config.
    path = path.replace(r'C:/Program Files (x86)/Git/bin', '')
    #os.environ['PATH'] = r'C:/MinGW/bin;' + path
elif build == 'qmake':
    cmake_command = ['qmake', '.']
    build_command = ['mingw32-make', '-j2']
    test_command = ['true']
else:
    # Add MSBuild 14.0 to PATH as described in
    # http://help.appveyor.com/discussions/problems/2229-v140-not-found-on-vs2105rc.
    os.environ['PATH'] = r'C:/Program Files (x86)/MSBuild/14.0/Bin;' + path
    generator = 'Visual Studio 14 2015'
    if platform == 'x64':
        generator += ' Win64'
    cmake_command.append('-G' + generator)
    cmake_command.append(r'-DCMAKE_USER_MOD_PATH=C:/Qt/5.9.1/msvc2015/lib/cmake')
    build_command = ['cmake', '--build', '.', '--config', config, '--', '/m:2']
    test_command = ['ctest', '-C', config]

check_call(cmake_command)
check_call(build_command)
check_call(test_command)
