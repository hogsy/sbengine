if $%postbuild_p3%==$ goto l0
echo Copying to %postbuild_p3%\Sbz1\GameClasses.dll...
copy GameClasses.dll %postbuild_p3%\Sbz1

:l0
if $%postbuild_ogier%==$ goto l1
echo Copying to %postbuild_ogier%\GameClasses.dll...
copy GameClasses.dll %postbuild_ogier%

:l1
