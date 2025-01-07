set MAdditionalPS3Compile=-Wno-non-virtual-dtor -Wno-sign-compare -Wno-conversion -Wno-multichar -Wno-unused -Werror
rem -Wno-non-virtual-dtor -Wno-sign-compare -Wno-conversion -Wno-implicit-int
set MPS3BinPath=%VS71COMNTOOLS%\..\..\vc7\bin
start .\XBench_PS3.sln
