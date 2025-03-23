cmake_minimum_required(VERSION 3.13)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()


project(Sonic3AIR)


# Build options
option(BUILD_OXYGEN_ENGINEAPP "Build the Oxygen App executable" ON)
option(BUILD_OXYGEN_SERVER "Build the Oxygen server executable" OFF)
option(BUILD_SDL_STATIC "Build SDL as a static library instead of a shared / dynamic one" ON)
option(USE_GLES "Use OpenGLESv2" OFF)
option(USE_DISCORD "Use Discord API" ON)
option(USE_IMGUI "Use ImGui for scripts development" ON)


if (USE_GLES)
	add_definitions(-DRMX_LINUX_ENFORCE_GLES2)
endif()


# Determine whether to support discord integration
if (USE_DISCORD)
	set(USE_DISCORD ON)
endif()
if (UNIX AND CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
	link_directories(/opt/vc/lib)	# Needed when building on Raspberry Pi
	set(USE_DISCORD OFF)
endif()



message(STATUS "BUILD_OXYGEN_ENGINEAPP = ${BUILD_OXYGEN_ENGINEAPP}")
message(STATUS "BUILD_OXYGEN_SERVER = ${BUILD_OXYGEN_SERVER}")
message(STATUS "BUILD_SDL_STATIC = ${BUILD_SDL_STATIC}")
message(STATUS "USE_GLES = ${USE_GLES}")
message(STATUS "USE_DISCORD = ${USE_DISCORD}")
message(STATUS "USE_IMGUI = ${USE_IMGUI}")



set(CMAKE_CXX_STANDARD 17)

set(WORKSPACE_DIR ../../../..)

#add_definitions(-DDEBUG)


if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

	# GCC only
	#set(CMAKE_CXX_FLAGS "-Wall")
	set(CMAKE_CXX_FLAGS_DEBUG "-g2")		# Include debug information
	set(CMAKE_CXX_FLAGS_RELEASE "-g0")		# Leave out debug information
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")		# Full optimization

	add_compile_options(-Wno-unused-function)	# Silence a warning in SDL2 build (unused function "xinput2_remove_device_info")
	add_compile_options(-Wno-stringop-overflow)	# Silence a warning in rmxbase String class usage
	add_compile_options(-Wno-psabi)

	# Needed under Linux to avoid link error from minizip: "relocation r_x86_64_pc32 against symbol `_length_code' can not be used when making a shared object; recompile with -fpic"
	if (UNIX)
		set(CMAKE_POSITION_INDEPENDENT_CODE ON)
	endif()

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

	# Clang only
	#set(CMAKE_C_COMPILER   "/usr/bin/clang")
	#set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
	#set(CMAKE_AR      "/usr/bin/llvm-ar" CACHE PATH "archive")
	#set(CMAKE_LINKER  "/usr/bin/llvm-ld" CACHE PATH "linker")
	#set(CMAKE_NM      "/usr/bin/llvm-nm" CACHE PATH "nm")
	#set(CMAKE_OBJDUMP "/usr/bin/llvm-objdump" CACHE PATH "objdump")
	#set(CMAKE_RANLIB  "/usr/bin/llvm-ranlib" CACHE PATH "ranlib")

endif()


find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)


# SDL2 sources as virtual subdirectory "SDL"
if (BUILD_SDL_STATIC)
	set(SDL_STATIC ON)
	set(SDL_SHARED OFF)
	include_directories(SDL/include)
else()
	set(SDL_STATIC OFF)
	set(SDL_SHARED ON)
	if (UNIX AND CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
		include(FindPkgConfig)
		pkg_check_modules(SDL2 sdl2 REQUIRED)
	endif()
endif()

add_subdirectory(${WORKSPACE_DIR}/framework/external/sdl/SDL2 SDL)


# zlib sources as virtual subdirectory "zlib"
include_directories(${WORKSPACE_DIR}/framework/external/zlib/zlib)		# Included before add_subdirectory so that the examples will build
add_subdirectory(${WORKSPACE_DIR}/framework/external/zlib/zlib zlib)



include_directories(${WORKSPACE_DIR}/framework/external/ogg-vorbis/libogg/include)
include_directories(${WORKSPACE_DIR}/framework/external/ogg-vorbis/libvorbis/include)
include_directories(${WORKSPACE_DIR}/framework/external/ogg-vorbis/libvorbis/lib)
include_directories(build/zlib)		# Needed for zconf.h
include_directories(${WORKSPACE_DIR}/framework/external/zlib/zlib/contrib/minizip)
if (USE_IMGUI)
	include_directories(${WORKSPACE_DIR}/framework/external/imgui/imgui)
endif()
include_directories(${WORKSPACE_DIR}/librmx/source)
include_directories(${WORKSPACE_DIR}/librmx/source/rmxmedia/_glew)
include_directories(${WORKSPACE_DIR}/Oxygen/lemonscript/source)
include_directories(${WORKSPACE_DIR}/Oxygen/oxygenengine/source)
include_directories(${WORKSPACE_DIR}/Oxygen/oxygenserver/source)
include_directories(${WORKSPACE_DIR}/Oxygen/sonic3air/source)
include_directories(${WORKSPACE_DIR}/Oxygen/sonic3air/source/external)



# ogg vorbis

# TODO: I tried creating config_types.h by calling "./configure" by a add_custom_command,
#       but that did not work; instead the solution is to simply add the generated file to versioning

file(GLOB OGGVORBIS_SOURCES ${WORKSPACE_DIR}/framework/external/ogg-vorbis/libogg/src/*.c
							${WORKSPACE_DIR}/framework/external/ogg-vorbis/libvorbis/lib/*.c)

add_library(oggvorbis ${OGGVORBIS_SOURCES})



# minizip

file(GLOB MINIZIP_SOURCES ${WORKSPACE_DIR}/framework/external/zlib/zlib/contrib/minizip/ioapi.c
						  ${WORKSPACE_DIR}/framework/external/zlib/zlib/contrib/minizip/unzip.c
						  ${WORKSPACE_DIR}/framework/external/zlib/zlib/contrib/minizip/zip.c)

add_library(minizip ${MINIZIP_SOURCES})

target_link_libraries(minizip zlibstatic)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(minizip PRIVATE -Wno-implicit-function-declaration -Wno-int-conversion)	# Silence these warnings in minizip build
endif()



# imgui
if (USE_IMGUI)
	file(GLOB_RECURSE IMGUI_SOURCES ${WORKSPACE_DIR}/framework/external/imgui/imgui/*.cpp)

	add_library(imgui ${IMGUI_SOURCES})

	if (BUILD_SDL_STATIC)
		target_link_libraries(imgui SDL2-static)
	else()
		target_link_libraries(imgui SDL2)
		if (UNIX AND CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
			target_link_libraries(imgui pthread)
			target_link_libraries(imgui dl)
		endif()
	endif()
	if (USE_GLES)
		target_link_libraries(imgui GLESv2)
	else()
		target_link_libraries(imgui OpenGL::GL)
	endif()
endif()



# rmxbase

file(GLOB_RECURSE RMXBASE_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxbase/*.cpp)

add_library(rmxbase ${RMXBASE_SOURCES})

if (NOT CMAKE_VERSION VERSION_LESS "3.16.0")
	target_precompile_headers(rmxbase PRIVATE ${WORKSPACE_DIR}/librmx/source/rmxbase.h)
endif()

target_link_libraries(rmxbase stdc++fs)
target_link_libraries(rmxbase zlibstatic)



# rmxmedia

file(GLOB_RECURSE RMXMEDIA_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxmedia/*.cpp
								   ${WORKSPACE_DIR}/librmx/source/rmxmedia/_glew/*.c)

add_library(rmxmedia ${RMXMEDIA_SOURCES})

target_link_libraries(rmxmedia rmxbase)
if (BUILD_SDL_STATIC)
	target_link_libraries(rmxmedia SDL2-static)
	target_link_libraries(rmxmedia ${SDL2_STATIC_LIBRARIES})
	target_link_options(rmxmedia PRIVATE ${SDL2_STATIC_LDLIBS_OTHER})
	target_compile_options(rmxmedia PRIVATE ${SDL2_STATIC_CFLAGS})
else()
	target_link_libraries(rmxmedia SDL2)
endif()
if (USE_GLES)
	target_link_libraries(rmxmedia GLESv2)
else()
	target_link_libraries(rmxmedia OpenGL::GL)
endif()



# rmxext_oggvorbis

file(GLOB RMXEXT_OGGVORBIS_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxext_oggvorbis/*.cpp)

add_library(rmxext_oggvorbis ${RMXEXT_OGGVORBIS_SOURCES})

target_link_libraries(rmxext_oggvorbis rmxmedia)
target_link_libraries(rmxext_oggvorbis oggvorbis)



# lemonscript

file(GLOB_RECURSE LEMONSCRIPT_SOURCES ${WORKSPACE_DIR}/Oxygen/lemonscript/source/lemon/*.cpp)

add_library(lemonscript ${LEMONSCRIPT_SOURCES})

if (NOT CMAKE_VERSION VERSION_LESS "3.16.0")
	target_precompile_headers(lemonscript PRIVATE ${WORKSPACE_DIR}/Oxygen/lemonscript/source/lemon/pch.h)
endif()

target_link_libraries(lemonscript rmxbase)



# oxygen_netcore

file(GLOB_RECURSE OXYGEN_NETCORE_SOURCES ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen_netcore/*.cpp)

add_library(oxygen_netcore ${OXYGEN_NETCORE_SOURCES})

if (NOT CMAKE_VERSION VERSION_LESS "3.16.0")
	target_precompile_headers(oxygen_netcore PRIVATE ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen_netcore/pch.h)
endif()

target_link_libraries(oxygen_netcore rmxbase)



# oxygenengine

file(GLOB_RECURSE OXYGEN_SOURCES ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen/*.cpp)

add_library(oxygen ${OXYGEN_SOURCES})

if (NOT CMAKE_VERSION VERSION_LESS "3.16.0")
	# TODO: Precompiled headers lead to build errors with ImGui, there's a conflict between GLEW and ImGui's OpenGL headers
	#  -> A way around this might be to not have <rmxmedia.h> in oxygen's PCH, at least for CMake builds
	#target_precompile_headers(oxygen PRIVATE ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen/pch.h)
endif()

target_link_libraries(oxygen minizip)
target_link_libraries(oxygen rmxmedia)
target_link_libraries(oxygen rmxext_oggvorbis)
target_link_libraries(oxygen lemonscript)
if (USE_IMGUI)
	target_link_libraries(oxygen imgui)
endif()
target_link_libraries(oxygen oxygen_netcore)

if (UNIX AND CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
	target_link_libraries(oxygen atomic)	# Needed when building on Raspberry Pi
endif()

if (UNIX)
	find_package(CURL REQUIRED)
	target_link_libraries(oxygen CURL::libcurl)
endif()



# OxygenApp
if (BUILD_OXYGEN_ENGINEAPP)

	file(GLOB_RECURSE ENGINEAPP_SOURCES ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/engineapp/*.cpp)

	# TODO: CMake seems to use a different working directory for this, so "Oxygen" is intentionally missing here
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${WORKSPACE_DIR}/oxygenengine")

	add_executable(OxygenApp ${ENGINEAPP_SOURCES})

	if (UNIX AND NOT APPLE)
		# Different executable name on Linux
		set_target_properties(OxygenApp PROPERTIES OUTPUT_NAME "oxygenapp_linux")
	endif ()

	if (NOT CMAKE_VERSION VERSION_LESS "3.16.0")
		target_precompile_headers(OxygenApp PRIVATE ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/engineapp/pch.h)
	endif()

	target_link_libraries(OxygenApp Threads::Threads)
	target_link_libraries(OxygenApp oxygen)

endif()



# Oxygen Server
if (BUILD_OXYGEN_SERVER)

	file(GLOB_RECURSE OXYGENSERVER_SOURCES ${WORKSPACE_DIR}/Oxygen/oxygenserver/source/oxygenserver/*.cpp)

	# TODO: CMake seems to use a different working directory for this, so "Oxygen" is intentionally missing here
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${WORKSPACE_DIR}/oxygenserver")

	add_executable(OxygenServer ${OXYGENSERVER_SOURCES})
	set_target_properties(OxygenServer PROPERTIES OUTPUT_NAME "oxygenserver")

	if (NOT CMAKE_VERSION VERSION_LESS "3.16.0")
		target_precompile_headers(OxygenServer PRIVATE ${WORKSPACE_DIR}/Oxygen/oxygenserver/source/oxygenserver/pch.h)
	endif()

	target_link_libraries(OxygenServer oxygen_netcore)

endif()



# discord_game_sdk

if (USE_DISCORD)
	file(GLOB_RECURSE DISCORD_SOURCES ${WORKSPACE_DIR}/Oxygen/sonic3air/source/external/discord_game_sdk/cpp/*.cpp)

	add_library(discord_game_sdk_source ${DISCORD_SOURCES})

	link_directories("${WORKSPACE_DIR}/Oxygen/sonic3air/source/external/discord_game_sdk/lib/x86_64")

	target_link_libraries(discord_game_sdk_source discord_game_sdk)

	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
endif()



# Sonic3AIR

file(GLOB_RECURSE SONIC3AIR_SOURCES ${WORKSPACE_DIR}/Oxygen/sonic3air/source/sonic3air/*.cpp)

# TODO: CMake seems to use a different working directory for this, so "Oxygen" is intentionally missing here
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${WORKSPACE_DIR}/sonic3air")

add_executable(Sonic3AIR ${SONIC3AIR_SOURCES})

if (UNIX AND NOT APPLE)
	# Different executable name on Linux
	set_target_properties(Sonic3AIR PROPERTIES OUTPUT_NAME "sonic3air_linux")
endif()

if (NOT CMAKE_VERSION VERSION_LESS "3.16.0")
	target_precompile_headers(Sonic3AIR PRIVATE ${WORKSPACE_DIR}/Oxygen/sonic3air/source/sonic3air/pch.h)
endif()

target_link_libraries(Sonic3AIR Threads::Threads)
target_link_libraries(Sonic3AIR oxygen)
if (USE_DISCORD)
	target_link_libraries(Sonic3AIR discord_game_sdk_source)
endif()

