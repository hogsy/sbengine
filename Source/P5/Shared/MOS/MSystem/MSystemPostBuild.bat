if $%postbuild_p3%==$ goto l0
echo Copying to %postbuild_p3%\MSystem.dll...
copy MSystem.dll %postbuild_p3%
echo Copying to %postbuild_p3%\SBZ1\Models\MSystem.dll...
copy MSystem.dll %postbuild_p3%\SBZ1\Models

:l0
if $%postbuild_ogier%==$ goto l1
echo Copying to %postbuild_ogier%\MSystem.dll...
copy MSystem.dll %postbuild_ogier%

:l1

if $"%postbuild_ed%"==$"" goto l2
echo Copying to %postbuild_ed%\MSystem.dll...
copy MSystem.dll "%postbuild_ed%"

:l2
