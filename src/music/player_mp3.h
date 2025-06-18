#ifndef _PLAYER_MP3_H_
#define _PLAYER_MP3_H_

#include "stream.h"

#include <stdbool.h>

bool mp3_decode_init(stream *st);
bool mp3_decode_start(stream *st);
bool mp3_decode_close();

#endif