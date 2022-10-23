@echo on

call ../get_msbuild_path.bat


:: Build libcurl
:: This is built with /MT flag, by setting the _CL_ environment variable, which enforces usage of non-DLL Release Runtime Library.
::  -> This way, the VS projects don't necessarily need that set manually in the vcxproj before building.

@echo.
@echo.
@echo === Building libcurl ===

pushd curl

pushd curl\projects\Windows\VC14.30
set _CL_=/MT
%msbuildPath22% curl-all.sln /target:libcurl /property:Configuration="LIB Release - DLL Windows SSPI" /property:Platform=Win32 -verbosity:minimal
%msbuildPath22% curl-all.sln /target:libcurl /property:Configuration="LIB Release - DLL Windows SSPI" /property:Platform=x64   -verbosity:minimal
set _CL_=
popd

call copy_curl.bat no_pause

popd


:: Done
echo.
if "%1"=="" pause
