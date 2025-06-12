#ifndef _PLAYER_MP3_H_
#define _PLAYER_MP3_H_

#include "stdint.h"
#include "stream.h"

void mp3_decode_init(stream *st);
int mp3_decode_start(stream *st);
void mp3_decode_close();

#endif