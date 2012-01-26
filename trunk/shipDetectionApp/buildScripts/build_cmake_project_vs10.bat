@echo off

set OLDPATH=%PATH%

call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"

set PATH="C:\Program Files (x86)\CMake 2.8\bin";%PATH%

set ossim_trunk_dir="D:/libraries/ossim/ossim_trunk"
set ossim_third_party_dir="D:/libraries/ossim/3rd-party"
set ossim_lib_dir="D:/libraries/ossim/ossim_build/Release"

set ossim_third_party_lib=%ossim_third_party_dir%/lib/win32
set ossim_include_dir=%ossim_trunk_dir%/ossim/include

cmake -G "Visual Studio 10"^
 -DCMAKE_BUILD_TYPE=Release^
 -DCMAKE_INCLUDE_PATH=%ossim_third_party_dir%/include^
 -DOSSIM_INCLUDE_DIR=%ossim_include_dir%^
 -DOPENCV_INCLUDE_DIR=%ossim_third_party_dir%/include^
 -DSHAPELIB_INCLUDE_DIR=%ossim_third_party_dir%/include^
 -DCVBLOBS_INCLUDE_DIR=%ossim_third_party_dir%/include^
 -DOPENCV_LIBRARIES=%ossim_third_party_lib%/cv210.lib;%ossim_third_party_lib%/cvaux210.lib;%ossim_third_party_lib%/cxcore210.lib;%ossim_third_party_lib%/cxts210.lib;%ossim_third_party_lib%/ml210.lib;%ossim_third_party_lib%/highgui210.lib^
 -DOSSIM_LIBRARIES=%ossim_lib_dir%/ossim.lib^
 -DSHAPELIB_LIRARY=%ossim_third_party_lib%/shapelib.lib^
 -DCVBLOBS_LIRARY=%ossim_third_party_lib%/cvblobslib.lib^
 ../

set PATH=%OLDPATH%
set OLDPATH=
