@echo off
if "%1"=="" goto fail
pkzip c:\backup\mcc\%1 -pr -ex *.cpp *.mpp *.dsp *.dsw *.c *.h *.hpp *.asm *.mdp *.dsw *.bat *.rc *.ico *.cur *.scr *.txt %2 %3 %4
goto :slut
:fail
echo No destination parameter specified.
:slut
