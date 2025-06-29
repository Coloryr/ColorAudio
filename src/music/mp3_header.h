#ifndef _PLAYER_MP3_HEADER_H_
#define _PLAYER_MP3_HEADER_H_

#include "stream.h"

float mp3_seek_to_time(stream_t *st, float time);
float mp3_get_time_len(stream_t *st);

#endif