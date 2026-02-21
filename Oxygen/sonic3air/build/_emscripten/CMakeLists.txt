cmake_minimum_required(VERSION 3.10)

project(Sonic3AIR)


# Build options
option(USE_IMGUI "Use ImGui, required for dev mode and system windows" ON)

message(STATUS "USE_IMGUI = ${USE_IMGUI}")



set(CMAKE_CXX_STANDARD 17)

set(WORKSPACE_DIR ../../../..)

#add_definitions(-DDEBUG)
add_definitions(-D__EMSCRIPTEN__)
add_definitions(-DRMX_USE_GLES2)
add_compile_definitions(PLATFORM_ENDUSER)
if (USE_IMGUI)
	add_definitions(-DUSE_IMGUI)
endif()


if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

	# GCC only
	set(CMAKE_CXX_FLAGS_DEBUG "-g2")		# Include debug information
	set(CMAKE_CXX_FLAGS_RELEASE "-g0")		# Leave out debug information
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")		# Full optimization
	add_compile_options(-Wno-psabi)

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

	# Clang only
	#SET(CMAKE_C_COMPILER   "/usr/bin/clang")
	#SET(CMAKE_CXX_COMPILER "/usr/bin/clang++")
	#SET(CMAKE_AR      "/usr/bin/llvm-ar" CACHE PATH "archive")
	#SET(CMAKE_LINKER  "/usr/bin/llvm-ld" CACHE PATH "linker")
	#SET(CMAKE_NM      "/usr/bin/llvm-nm" CACHE PATH "nm")
	#SET(CMAKE_OBJDUMP "/usr/bin/llvm-objdump" CACHE PATH "objdump")
	#SET(CMAKE_RANLIB  "/usr/bin/llvm-ranlib" CACHE PATH "ranlib")

endif()

set (CMAKE_CXX_FLAGS "-s USE_VORBIS=1 -s USE_SDL=2 -s USE_ZLIB=1")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s FETCH=1 -lidbfs.js -lwebsocket.js")

include_directories(${WORKSPACE_DIR}/framework/external/sdl/SDL2/include)
include_directories(${WORKSPACE_DIR}/framework/external/zlib/zlib/contrib/minizip)
if (USE_IMGUI)
	include_directories(${WORKSPACE_DIR}/framework/external/imgui/imgui)
endif()
include_directories(${WORKSPACE_DIR}/librmx/source)
include_directories(${WORKSPACE_DIR}/Oxygen/lemonscript/source)
include_directories(${WORKSPACE_DIR}/Oxygen/oxygenengine/source)
include_directories(${WORKSPACE_DIR}/Oxygen/sonic3air/source)
include_directories(${WORKSPACE_DIR}/Oxygen/sonic3air/source/external)


# imgui
if (USE_IMGUI)
	file(GLOB IMGUI_SOURCES ${WORKSPACE_DIR}/framework/external/imgui/imgui/*.cpp
							${WORKSPACE_DIR}/framework/external/imgui/imgui/backends/imgui_impl_opengl3.cpp
							${WORKSPACE_DIR}/framework/external/imgui/imgui/backends/imgui_impl_sdl2.cpp)

	add_library(imgui ${IMGUI_SOURCES})
endif()


# rmxbase

file(GLOB_RECURSE RMXBASE_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxbase/*.cpp)

add_library(rmxbase ${RMXBASE_SOURCES})

#target_precompile_headers(rmxbase PRIVATE rmxbase.h)	# Requires CMake 3.16

#target_link_libraries(rmxbase stdc++fs)
#target_link_libraries(rmxbase zlibstatic)


# rmxmedia

file(GLOB_RECURSE RMXMEDIA_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxmedia/*.cpp)

add_library(rmxmedia ${RMXMEDIA_SOURCES})

target_link_libraries(rmxmedia rmxbase)
#target_link_libraries(rmxmedia SDL2)
#target_link_libraries(rmxmedia ${OPENGL_gl_LIBRARY})



# rmxext_oggvorbis

file(GLOB RMXEXT_OGGVORBIS_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxext_oggvorbis/*.cpp)

add_library(rmxext_oggvorbis ${RMXEXT_OGGVORBIS_SOURCES})

target_link_libraries(rmxext_oggvorbis rmxmedia)


# lemonscript

file(GLOB_RECURSE LEMONSCRIPT_SOURCES ${WORKSPACE_DIR}/Oxygen/lemonscript/source/lemon/*.cpp)

add_library(lemonscript ${LEMONSCRIPT_SOURCES})

target_link_libraries(lemonscript rmxbase)



# oxygen_netcore

file(GLOB_RECURSE OXYGEN_NETCORE_SOURCES ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen_netcore/*.cpp)

add_library(oxygen_netcore ${OXYGEN_NETCORE_SOURCES})

if (NOT CMAKE_VERSION VERSION_LESS "3.16.0")
	target_precompile_headers(oxygen_netcore PRIVATE ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen_netcore/pch.h)
endif()

target_link_libraries(oxygen_netcore rmxbase)



# oxygenengine

file(GLOB_RECURSE OXYGEN_SOURCES ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen/*.cpp
	${WORKSPACE_DIR}/framework/external/zlib/zlib/contrib/minizip/ioapi.c
	${WORKSPACE_DIR}/framework/external/zlib/zlib/contrib/minizip/unzip.c)

add_library(oxygen_enduser ${OXYGEN_SOURCES})

target_compile_definitions(oxygen_enduser PUBLIC ENDUSER)

target_link_libraries(oxygen_enduser rmxmedia)
target_link_libraries(oxygen_enduser rmxext_oggvorbis)
target_link_libraries(oxygen_enduser lemonscript)
if (USE_IMGUI)
	target_link_libraries(oxygen_enduser imgui)
endif()
target_link_libraries(oxygen_enduser oxygen_netcore)



# OxygenApp

#file(GLOB_RECURSE ENGINEAPP_SOURCES ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/engineapp/*.cpp)

# TODO: CMake seems to use a different working directory for this, so "Oxygen" is intentionally missing here
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${WORKSPACE_DIR}/oxygenengine")

#add_executable(OxygenApp ${ENGINEAPP_SOURCES})

#target_link_libraries(OxygenApp oxygen_development)

#set_target_properties(OxygenApp PROPERTIES OUTPUT_NAME "oxygenapp_linux.html")




# Sonic3AIR

file(GLOB_RECURSE SONIC3AIR_SOURCES ${WORKSPACE_DIR}/Oxygen/sonic3air/source/sonic3air/*.cpp)

# TODO: CMake seems to use a different working directory for this, so "Oxygen" is intentionally missing here
SET(CMAKE_EXECUTABLE_SUFFIX ".js")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${WORKSPACE_DIR}/sonic3air/build/_emscripten/dist")

add_executable(Sonic3AIR ${SONIC3AIR_SOURCES})

target_compile_definitions(Sonic3AIR PUBLIC ENDUSER)

#target_link_libraries(Sonic3AIR SDL2)
target_link_libraries(Sonic3AIR oxygen_enduser)
#target_link_libraries(Sonic3AIR discord_game_sdk_source)

set_target_properties(Sonic3AIR PROPERTIES OUTPUT_NAME "sonic3air_web")

set_target_properties(Sonic3AIR PROPERTIES LINK_FLAGS "-s FULL_ES2=1 -sFORCE_FILESYSTEM -sSTACK_SIZE=2MB -s ALLOW_MEMORY_GROWTH=1 -s EXIT_RUNTIME=1 -s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE=\"['\$ERRNO_CODES']\"")

