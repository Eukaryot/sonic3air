@echo on

call ../get_msbuild_path.bat


:: Build ImGui

@echo.
@echo.
@echo === Building SDL2 ===

pushd _vstudio
%msbuildPath% imgui.sln /target:imgui /property:Configuration=Debug   /property:Platform=x86 -verbosity:minimal
%msbuildPath% imgui.sln /target:imgui /property:Configuration=Release /property:Platform=x86 -verbosity:minimal
%msbuildPath% imgui.sln /target:imgui /property:Configuration=Debug   /property:Platform=x64 -verbosity:minimal
%msbuildPath% imgui.sln /target:imgui /property:Configuration=Release /property:Platform=x64 -verbosity:minimal
popd

call copy_imgui.bat no_pause


:: Done
echo.
if "%1"=="" pause
