@echo off
if exist "build" rmdir "build" /S /Q
if exist "libs" rmdir "libs" /S /Q
if exist "obj" rmdir "obj" /S /Q
make clean
pause