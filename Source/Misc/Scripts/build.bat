@echo off
set PATH=%CYGPATH%bin;%PATH%
pushd s:\Source\P5
perl ../Misc/Scripts/ParseSln.pl P5_PS3.sln ../Misc/Scripts/distcchosts.txt
make Config=RTMD -j6
popd