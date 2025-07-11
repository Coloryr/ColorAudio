cmake_minimum_required(VERSION 3.10.0)
project(coloraudio VERSION 0.1.0)

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
    include_directories(./include/freetype2)
else()
    find_package(SDL2 REQUIRED)
    include_directories(/usr/include/drm)
    include_directories(/usr/include/freetype2)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

# includes

# include_directories(./display)
# include_directories(./sys)
# include_directories(./common)
# include_directories(./view)
# include_directories(./music)
# include_directories(./input)
# include_directories(./net)
# include_directories(./stream)
# include_directories(./melon/include)
# include_directories(./lvgl)

include_directories(./lib_src)

#def
add_definitions(-DMLN_ROOT="/usr/local/melon")
add_definitions(-DMLN_NULL="/dev/null")
add_definitions(-DMLN_LANG_LIB="/usr/local/lib/melang")
add_definitions(-DMLN_LANG_DYLIB="/usr/local/lib/melang_dynamic")
add_definitions(-DMLN_EPOLL -DMLN_SENDFILE -DMLN_WRITEV -DMLN_USE_UNIX98 -DMLN_MMAP -DMLN_CONSTRUCTOR -DMINIMP4_IMPLEMENTATION)

#src

file(GLOB_RECURSE SOURCES 
"./lvgl/src/*.*"
"./display/*.*" 
"./sys/*.*" 
"./common/*.*"
"./music/*.*"
"./ui/*.*"
"./input/*.*"
"./stream/*.*"
"./player/*.*"
"./net/*.*"
"./config/*.*"
"./fft/*.*"
)

add_executable(coloraudio ${SOURCES} main.cpp)

if (DEFINED BUILD_ARM)
    target_link_libraries(coloraudio PRIVATE 
    "${CMAKE_SOURCE_DIR}/libs/libasound.so"
    "${CMAKE_SOURCE_DIR}/libs/libevdev.so"
    "${CMAKE_SOURCE_DIR}/libs/librga.so"
    "${CMAKE_SOURCE_DIR}/libs/libdrm.so"
    "${CMAKE_SOURCE_DIR}/libs/libSDL2.so"
    "${CMAKE_SOURCE_DIR}/libs/libboost_url.so"
    "${CMAKE_SOURCE_DIR}/libs/libboost_system.so"
    "${CMAKE_SOURCE_DIR}/libs/libFLAC.so"
    "${CMAKE_SOURCE_DIR}/libs/libFLAC++.so"
    "${CMAKE_SOURCE_DIR}/libs/libpng16.so"
    "${CMAKE_SOURCE_DIR}/libs/libturbojpeg.so"
    "${CMAKE_SOURCE_DIR}/libs/libfreetype.so"
    "${CMAKE_SOURCE_DIR}/libs/libcryptopp.so"
    "${CMAKE_SOURCE_DIR}/libs/libssl.so"
    "${CMAKE_SOURCE_DIR}/libs/libmad.so"
    "${CMAKE_SOURCE_DIR}/libs/libcrypto.so.3"

    "${CMAKE_SOURCE_DIR}/libs/librime.a"
    "${CMAKE_SOURCE_DIR}/libs/libglog.a"
    "${CMAKE_SOURCE_DIR}/libs/libmarisa.a"
    "${CMAKE_SOURCE_DIR}/libs/libleveldb.a"
    "${CMAKE_SOURCE_DIR}/libs/libopencc.a"
    "${CMAKE_SOURCE_DIR}/libs/libyaml-cpp.a"
    )
else()
    add_library(boost_beast INTERFACE)
    # target_include_directories(boost_beast SYSTEM PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../beast/include")
    target_link_libraries(coloraudio PRIVATE png evdev ${SDL2_LIBRARIES}
        drm asound mad freetype m turbojpeg FLAC++ FLAC rime 
        boost_system boost_url ssl crypto cryptopp openh264
    )
endif()