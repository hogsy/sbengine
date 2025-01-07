if $%postbuild_p3%==$ goto l0
echo Copying to %postbuild_p3%\MXR.dll...
copy MXR.dll %postbuild_p3%
echo Copying to %postbuild_p3%\SBZ1\Models\MXR.dll...
copy MXR.dll %postbuild_p3%\SBZ1\Models\

:l0

if $"%postbuild_ed%"==$"" goto l1
echo Copying to %postbuild_ed%\MXR.dll...
copy MXR.dll "%postbuild_ed%"

:l1
