#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "stdint.h"

#include "mln_list.h"

typedef enum
{
    MUSIC_TYPE_WAV = 0,
    MUSIC_TYPE_MP3,
    MUSIC_TYPE_FLAC,
    MUSIC_TYPE_UNKNOW = -1
} music_type;

typedef struct
{
    uint8_t *path;
    uint16_t len;
    mln_list_t node;
} play_item;

#ifdef __cplusplus
extern "C"
{
#endif

    extern uint32_t time_all;
    extern uint32_t time_now;
    extern uint32_t prerate;

    void play_init();
    void play_file(char *path);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif