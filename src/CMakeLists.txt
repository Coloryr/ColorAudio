cmake_minimum_required(VERSION 3.10.0)
project(coloraudio VERSION 0.1.0 LANGUAGES C)

# build arm32
# set(BUILD_ARM ON)

# Lvgl
set(LV_CONF_PATH
        lv_conf.h
        CACHE STRING "" FORCE)

include_directories(lvgl/src)
include_directories(lvgl)

if (DEFINED BUILD_ARM)
    set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
    set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
    add_definitions(-DBUILD_ARM -DLV_DRM_USE_RGA)
    include_directories(./include)
    include_directories(./include/drm)
else()
    find_package(SDL2 REQUIRED)
    include_directories(/usr/include/drm)
endif()

# includes

include_directories(./display)
include_directories(./sys)
include_directories(./common)
include_directories(./view)
include_directories(./music)
include_directories(./melon/include)
include_directories(./)

#def
add_definitions(-DMLN_ROOT="/usr/local/melon")
add_definitions(-DMLN_NULL="/dev/null")
add_definitions(-DMLN_LANG_LIB="/usr/local/lib/melang")
add_definitions(-DMLN_LANG_DYLIB="/usr/local/lib/melang_dynamic")
add_definitions(-DMLN_EPOLL -DMLN_SENDFILE -DMLN_WRITEV -DMLN_USE_UNIX98 -DMLN_MMAP -DMLN_CONSTRUCTOR)

#src

file(GLOB_RECURSE SOURCES 
"./melon/src/*.*" 
"./lvgl/src/*.*"
"./display/*.*" 
"./sys/*.*" 
"./common/*.*"
"./music/*.*"
"./view/*.*")

add_executable(coloraudio ${SOURCES} main.c)

if (DEFINED BUILD_ARM)
    target_link_libraries(coloraudio PRIVATE 
    "${CMAKE_SOURCE_DIR}/libs/libasound.so"
    "${CMAKE_SOURCE_DIR}/libs/libevdev.so"
    "${CMAKE_SOURCE_DIR}/libs/librga.so"
    "${CMAKE_SOURCE_DIR}/libs/libdrm.so"
    "${CMAKE_SOURCE_DIR}/libs/libid3tag.so"
    "${CMAKE_SOURCE_DIR}/libs/libmad.so"
    "${CMAKE_SOURCE_DIR}/libs/libpng.so"
    "${CMAKE_SOURCE_DIR}/libs/libSDL2.so")
else()
    target_link_libraries(coloraudio PRIVATE png evdev ${SDL2_LIBRARIES} drm asound mad)
endif()