cmake_minimum_required(VERSION 3.10)


project(Sonic3AIR)

set(CMAKE_CXX_STANDARD 17)

set(WORKSPACE_DIR ../../../../../..)

#add_definitions(-DDEBUG)
add_definitions(-DIMGUI_IMPL_OPENGL_ES2)


if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	add_compile_options(-Wno-switch)	# Silence warning "enumeration value ... not handled in switch"
endif()


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")


# SDL2 sources as virtual subdirectory "SDL"
add_subdirectory(${WORKSPACE_DIR}/framework/external/sdl/SDL2 SDL)

find_library(SDL2 SDL2)



include_directories(SDL/include)
include_directories(${WORKSPACE_DIR}/framework/external/ogg-vorbis/libogg/include)
include_directories(${WORKSPACE_DIR}/framework/external/ogg-vorbis/libvorbis/include)
include_directories(${WORKSPACE_DIR}/framework/external/ogg-vorbis/libvorbis/lib)
include_directories(${WORKSPACE_DIR}/framework/external/zlib/zlib/contrib/minizip)
include_directories(${WORKSPACE_DIR}/framework/external/imgui/imgui)
include_directories(${WORKSPACE_DIR}/librmx/source)
include_directories(${WORKSPACE_DIR}/Oxygen/lemonscript/source)
include_directories(${WORKSPACE_DIR}/Oxygen/oxygenengine/source)
include_directories(${WORKSPACE_DIR}/Oxygen/sonic3air/source)



# ogg vorbis

file(GLOB OGGVORBIS_SOURCES ${WORKSPACE_DIR}/framework/external/ogg-vorbis/libogg/src/*.c
							${WORKSPACE_DIR}/framework/external/ogg-vorbis/libvorbis/lib/*.c)

add_library(oggvorbis SHARED ${OGGVORBIS_SOURCES})



# minizip

file(GLOB MINIZIP_SOURCES ${WORKSPACE_DIR}/framework/external/zlib/zlib/contrib/minizip/ioapi.c
						  ${WORKSPACE_DIR}/framework/external/zlib/zlib/contrib/minizip/unzip.c
						  ${WORKSPACE_DIR}/framework/external/zlib/zlib/contrib/minizip/zip.c)

add_library(minizip SHARED ${MINIZIP_SOURCES})

target_link_libraries(minizip z)		# zlib



# imgui

file(GLOB IMGUI_SOURCES ${WORKSPACE_DIR}/framework/external/imgui/imgui/*.cpp
						${WORKSPACE_DIR}/framework/external/imgui/imgui/backends/imgui_impl_opengl3.cpp
						${WORKSPACE_DIR}/framework/external/imgui/imgui/backends/imgui_impl_sdl2.cpp)

add_library(imgui ${IMGUI_SOURCES})

target_link_libraries(imgui SDL2)
target_link_libraries(imgui GLESv2)



# rmxbase

file(GLOB_RECURSE RMXBASE_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxbase/*.cpp)

add_library(rmxbase SHARED ${RMXBASE_SOURCES})

target_link_libraries(rmxbase z)		# zlib
target_link_libraries(rmxbase android)
target_link_libraries(rmxbase log)



# rmxmedia

file(GLOB_RECURSE RMXMEDIA_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxmedia/*.cpp)

add_library(rmxmedia SHARED ${RMXMEDIA_SOURCES})

target_link_libraries(rmxmedia SDL2)
target_link_libraries(rmxmedia GLESv2)
target_link_libraries(rmxmedia rmxbase)



# rmxext_oggvorbis

file(GLOB RMXEXT_OGGVORBIS_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxext_oggvorbis/*.cpp)

add_library(rmxext_oggvorbis ${RMXEXT_OGGVORBIS_SOURCES})

target_link_libraries(rmxext_oggvorbis rmxmedia)
target_link_libraries(rmxext_oggvorbis oggvorbis)



# lemonscript

file(GLOB_RECURSE LEMONSCRIPT_SOURCES ${WORKSPACE_DIR}/Oxygen/lemonscript/source/lemon/*.cpp)

add_library(lemonscript ${LEMONSCRIPT_SOURCES})

target_link_libraries(lemonscript rmxbase)



# Oxygen Core Library

file(GLOB_RECURSE OXYGEN_NETCORE_SOURCES ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen_netcore/*.cpp)

add_library(oxygen_netcore ${OXYGEN_NETCORE_SOURCES})

target_link_libraries(oxygen_netcore rmxbase)



# Oxygen Engine Library

file(GLOB_RECURSE OXYGEN_SOURCES ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen/*.cpp)

add_library(oxygen ${OXYGEN_SOURCES})

target_link_libraries(oxygen minizip)
target_link_libraries(oxygen rmxmedia)
target_link_libraries(oxygen rmxext_oggvorbis)
target_link_libraries(oxygen lemonscript)
target_link_libraries(oxygen imgui)
target_link_libraries(oxygen oxygen_netcore)



# Sonic3AIR

file(GLOB_RECURSE SONIC3AIR_SOURCES ${WORKSPACE_DIR}/Oxygen/sonic3air/source/sonic3air/*.cpp)

add_library(main SHARED ${SONIC3AIR_SOURCES})

target_link_libraries(main oxygen)
