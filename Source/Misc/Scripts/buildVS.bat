@echo off
set PATH=%CYGPATH%bin;%PATH%
pushd %1
perl %5/ParseSln.pl %2 %5
%5\mingw32-make.exe Config=%3 -j14 %4
popd
