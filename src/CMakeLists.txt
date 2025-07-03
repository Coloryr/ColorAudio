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
add_definitions(-DMLN_EPOLL -DMLN_SENDFILE -DMLN_WRITEV -DMLN_USE_UNIX98 -DMLN_MMAP -DMLN_CONSTRUCTOR)

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

    "${CMAKE_SOURCE_DIR}/libs/libboost_atomic.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_chrono.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_container.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_context.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_coroutine.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_date_time.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_exception.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_fiber.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_filesystem.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_graph.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_graph_parallel.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_iostreams.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_json.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_locale.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_log.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_log_setup.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_math_c99.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_math_c99f.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_math_tr1.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_math_tr1f.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_mpi.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_nowide.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_prg_exec_monitor.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_program_options.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_random.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_regex.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_serialization.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_stacktrace_addr2line.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_stacktrace_backtrace.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_stacktrace_basic.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_stacktrace_noop.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_system.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_thread.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_timer.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_type_erasure.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_unit_test_framework.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_url.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_wave.a"
    "${CMAKE_SOURCE_DIR}/libs/libboost_wserialization.a"

    "${CMAKE_SOURCE_DIR}/libs/libFLAC.a"
    "${CMAKE_SOURCE_DIR}/libs/libFLAC++.a"
    "${CMAKE_SOURCE_DIR}/libs/libogg.a"
    "${CMAKE_SOURCE_DIR}/libs/libssl.a"
    "${CMAKE_SOURCE_DIR}/libs/libcrypto.a"
    "${CMAKE_SOURCE_DIR}/libs/libmad.a"
    "${CMAKE_SOURCE_DIR}/libs/libpng16.a"
    "${CMAKE_SOURCE_DIR}/libs/libfreetype.a"
    "${CMAKE_SOURCE_DIR}/libs/libturbojpeg.a"
    "${CMAKE_SOURCE_DIR}/libs/librime.a"
    "${CMAKE_SOURCE_DIR}/libs/libglog.a"
    "${CMAKE_SOURCE_DIR}/libs/libmarisa.a"
    "${CMAKE_SOURCE_DIR}/libs/libleveldb.a"
    "${CMAKE_SOURCE_DIR}/libs/libbz2.a"
    "${CMAKE_SOURCE_DIR}/libs/libopencc.a"
    "${CMAKE_SOURCE_DIR}/libs/libyaml-cpp.a"
    "${CMAKE_SOURCE_DIR}/libs/libz.a"
    "${CMAKE_SOURCE_DIR}/libs/libm.a"
    "${CMAKE_SOURCE_DIR}/libs/libbrotlidec.a"
    "${CMAKE_SOURCE_DIR}/libs/libbrotlicommon.a"
    )
else()
    add_library(boost_beast INTERFACE)
    # target_include_directories(boost_beast SYSTEM PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../beast/include")
    target_link_libraries(coloraudio PRIVATE png evdev ${SDL2_LIBRARIES}
        drm asound mad freetype m turbojpeg FLAC++ FLAC rime 
        boost_system boost_url ssl crypto cryptopp
    )
endif()