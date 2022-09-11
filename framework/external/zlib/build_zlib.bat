@echo on

call ../get_msbuild_path.bat


:: Build zlib + minizip

@echo.
@echo.
@echo === Building zlib + minizip ===

pushd _vstudio
%msbuildPath% zlib.sln /target:zlib    /property:Configuration=Debug   /property:Platform=x86 -verbosity:minimal
%msbuildPath% zlib.sln /target:minizip /property:Configuration=Debug   /property:Platform=x86 -verbosity:minimal
%msbuildPath% zlib.sln /target:zlib    /property:Configuration=Release /property:Platform=x86 -verbosity:minimal
%msbuildPath% zlib.sln /target:minizip /property:Configuration=Release /property:Platform=x86 -verbosity:minimal
%msbuildPath% zlib.sln /target:zlib    /property:Configuration=Debug   /property:Platform=x64 -verbosity:minimal
%msbuildPath% zlib.sln /target:minizip /property:Configuration=Debug   /property:Platform=x64 -verbosity:minimal
%msbuildPath% zlib.sln /target:zlib    /property:Configuration=Release /property:Platform=x64 -verbosity:minimal
%msbuildPath% zlib.sln /target:minizip /property:Configuration=Release /property:Platform=x64 -verbosity:minimal
popd

call copy_zlib.bat no_pause


:: Done
echo.
if "%1"=="" pause
