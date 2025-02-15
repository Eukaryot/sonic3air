@echo on

:: You can replace this with "mastering_build_data_64bit.bat" if you like
call mastering_build_data_32bit.bat no_pause

pushd ..\..

set outputDir=build\_android\app\src\main\assets


rmdir "%outputDir%" /s /q
mkdir "%outputDir%"

robocopy "_master_image_template\data" "%outputDir%\data" /mir
copy "_master_image_template\config.json" "%outputDir%"

:: Leave out the remastered music, as the Android build can use an internal downloader
del "%outputDir%\data\audioremaster.bin"

popd


:: Done
pause
