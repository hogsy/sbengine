SET CYGPATH=c:/cygwin/
set MAdditionalPS3Compile=-Wno-non-virtual-dtor -Wno-sign-compare -Wno-conversion -Wno-multichar -Wno-unused -Werror
rem -Wno-non-virtual-dtor -Wno-sign-compare -Wno-conversion -Wno-implicit-int
rem set MPS3BinPath=%VS80COMNTOOLS%\..\..\vc7\bin
start "%VS80COMNTOOLS%..\IDE\devenv.exe" .\P5_PS3.sln
