#ifndef _LOCAL_MUSIC_H_
#define _LOCAL_MUSIC_H_

#include <stdint.h>

#ifdef BUILD_ARM
#define READ_DIR "/sdcard"
#else
// #define READ_DIR "/home/coloryr/playlist"
// #define READ_DIR "/home/coloryr/playtest"
#define READ_DIR "/mnt/hgfs/music"
#endif

void local_music_init();

#endif