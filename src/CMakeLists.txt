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
    include_directories(./include/glib-2.0)
    include_directories(./include/dbus-1.0)
    include_directories(./include/opus)
else()
    find_package(SDL2 REQUIRED)
    include_directories(/usr/include/drm)
    include_directories(/usr/include/freetype2)
    include_directories(${Boost_INCLUDE_DIRS})
    include_directories(/usr/include/glib-2.0)
    include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include)
    include_directories(/usr/include/dbus-1.0)
    include_directories(/usr/lib/x86_64-linux-gnu/dbus-1.0/include)
    include_directories(/usr/include/opus)
endif()

# includes

include_directories(./lib_src)
include_directories(./lib_src/bluez-alsa/src)

#def
add_definitions(-DHAVE_CONFIG_H -DMINIMP4_IMPLEMENTATION)

#src

file(GLOB_RECURSE LIB_SOURCES
"./lib_src/bluez-alsa/src/*.*"
)

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
"./wireless/*.*"
"./config/*.*"
"./fft/*.*"
)

add_executable(coloraudio ${LIB_SOURCES} ${SOURCES} main.cpp)

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
    "${CMAKE_SOURCE_DIR}/libs/libopenh264.so"
    "${CMAKE_SOURCE_DIR}/libs/libwpa_client.so"
    "${CMAKE_SOURCE_DIR}/libs/libfdk-aac.so"
    "${CMAKE_SOURCE_DIR}/libs/libfreeaptx.so"
    "${CMAKE_SOURCE_DIR}/libs/libopus.so"
    "${CMAKE_SOURCE_DIR}/libs/libbluetooth.so"
    "${CMAKE_SOURCE_DIR}/libs/libdbus-1.so"
    "${CMAKE_SOURCE_DIR}/libs/libglib-2.0.so"
    "${CMAKE_SOURCE_DIR}/libs/libsbc.so"
    "${CMAKE_SOURCE_DIR}/libs/libgobject-2.0.so"
    "${CMAKE_SOURCE_DIR}/libs/libgio-2.0.so"

    #rime
    "${CMAKE_SOURCE_DIR}/libs/librime.a"
    "${CMAKE_SOURCE_DIR}/libs/libglog.a"
    "${CMAKE_SOURCE_DIR}/libs/libmarisa.a"
    "${CMAKE_SOURCE_DIR}/libs/libleveldb.a"
    "${CMAKE_SOURCE_DIR}/libs/libopencc.a"
    "${CMAKE_SOURCE_DIR}/libs/libyaml-cpp.a"

    "${CMAKE_SOURCE_DIR}/libs/liblc3.a"
    )
else()
    add_library(boost_beast INTERFACE)
    # target_include_directories(boost_beast SYSTEM PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../beast/include")
    target_link_libraries(coloraudio PRIVATE png evdev ${SDL2_LIBRARIES}
        drm asound mad freetype m turbojpeg FLAC++ FLAC rime dbus-1 opus
        fdk-aac sbc bluetooth glib-2.0 gio-2.0 gobject-2.0 freeaptx lc3
        sbc spandsp
        boost_system boost_url ssl crypto cryptopp openh264 wpa_client
    )
endif()