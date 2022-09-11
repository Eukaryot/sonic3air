@echo on

call mastering_build_data.bat no_pause

pushd ..\..

set outputDir=build\_android\app\src\main\assets


rmdir "%outputDir%" /s /q
mkdir "%outputDir%"

robocopy "_master_image_template\data" "%outputDir%/data" /mir
copy "_master_image_template\config.json" "%outputDir%"

popd


:: Done
pause
