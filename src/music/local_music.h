#ifndef _LOCAL_MUSIC_H_
#define _LOCAL_MUSIC_H_

#include <stdint.h>

#ifdef BUILD_ARM
#define READ_DIR "/sdcard"
#else
// #define READ_DIR "/home/coloryr/test"
#define READ_DIR "/mnt/hgfs/music"
#endif

extern bool local_music_scan_now;

void local_music_init();
void local_music_run();

#endif