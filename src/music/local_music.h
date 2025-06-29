#ifndef _LOCAL_MUSIC_H_
#define _LOCAL_MUSIC_H_

#include "mln_list.h"

#include <stdint.h>

typedef struct
{
    mln_list_t node;
    uint32_t index;
    uint8_t *path;
    uint32_t len;
    uint8_t *title;
    uint32_t title_len;
    uint8_t *auther;
    uint32_t auther_len;
    float time;
} play_item;

extern mln_list_t local_play_list;

void local_music_init();

#endif