@echo on

call mastering_build_data.bat no_pause

pushd ..\..

set destDir=..\_MASTER
set outputDir=%destDir%\sonic3air_game
call ../../../../framework/external/get_msbuild_path.bat



:: Preparations
rmdir "%destDir%" /s /q
mkdir "%outputDir%"


:: Make sure the needed binaries are all up-to-date

%msbuildPath% ..\oxygenengine\build\_vstudio\oxygenengine.sln /property:Configuration=Release /property:Platform=Win32 -verbosity:minimal
%msbuildPath% build\_vstudio\sonic3air.sln /property:Configuration=Release-Enduser /property:Platform=Win32 -verbosity:minimal



:: Build the master installation

robocopy "_master_image_template" "%outputDir%" /e

:: Leave out the remastered music, as the Windows build can use an internal downloader
del "%outputDir%\data\audioremaster.bin"

copy "bin\Release-Enduser_x86\*.exe" "%outputDir%"
copy "source\external\discord_game_sdk\lib\x86\discord_game_sdk.dll" "%outputDir%"


:: Add Oxygen engine

robocopy "..\oxygenengine\_master_image_template" "%outputDir%\bonus\oxygenengine" /e
copy "..\oxygenengine\bin\Release_x86\OxygenApp.exe" "%outputDir%\bonus\oxygenengine\"
mkdir "%outputDir%\data"
robocopy "..\oxygenengine\data" "%outputDir%\bonus\oxygenengine\data" /e


:: Complete S3AIR dev

robocopy "scripts" "%outputDir%\bonus\sonic3air_dev\scripts" /e



:: Pack everything as a ZIP

pushd "%destDir%"
	"C:\Program Files\7-Zip\7z.exe" a -tzip -r sonic3air_game.zip %outputDir%
popd


:: Archive artifacts
mkdir "%destDir%\artifacts\bin"
copy "bin\Release-Enduser_x86\*.exe" "%destDir%\artifacts\bin"
copy "bin\Release-Enduser_x86\*.pdb" "%destDir%\artifacts\bin"

pushd "%destDir%"
	"C:\Program Files\7-Zip\7z.exe" a -t7z -mx1 -r artifacts.7z artifacts
popd

rmdir "%destDir%\artifacts" /s /q

popd


:: Done
pause
