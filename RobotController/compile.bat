@echo off
set "PATH=%onedrive%\Documents\2play\apps\codeblocks\codeblocks-17.12mingw-nosetup\MinGW\bin\;%PATH%"
gcc "%cd%\robotcontroller.c" "%cd%\controllerfunctions.c" -o "%cd%\robotcontroller.exe" -lSDL2 -lSDL2_net
pause
