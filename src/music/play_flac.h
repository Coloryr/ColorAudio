#ifndef _PLAY_FLAC_H_
#define _PLAY_FLAC_H_

#include "stream.h"

void flac_decode_init(stream *st);
int flac_decode_start(stream *st);
void flac_decode_close();

#endif