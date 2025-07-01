#ifndef _LOCAL_MUSIC_H_
#define _LOCAL_MUSIC_H_

#include <stdint.h>

#ifdef BUILD_ARM
#define READ_DIR "/sdcard"
#else
#define READ_DIR "/home/coloryr/playlist"
// #define READ_DIR "/home/coloryr/playtest"
#endif

void local_music_init();

#endif