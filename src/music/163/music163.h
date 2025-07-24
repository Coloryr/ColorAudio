#ifndef __MUSIC163_H__
#define __MUSIC163_H__

#include "../lyric.h"

#include <stdint.h>
#include <string>

bool music_lyric_163(uint64_t id, LyricParser **ldata, LyricParser **trdata);
bool music_lyric_163(std::string &comment, LyricParser **ldata, LyricParser **trdata);

#endif // __MUSIC163_H__