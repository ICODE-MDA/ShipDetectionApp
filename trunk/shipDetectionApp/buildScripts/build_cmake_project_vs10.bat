@echo off

set OLDPATH=%PATH%

call "C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"

set PATH="C:\Program Files\CMake 2.8\bin";%PATH%

call cmake_arguments.bat

set PATH=%OLDPATH%
set OLDPATH=
