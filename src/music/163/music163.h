#ifndef __MUSIC163_H__
#define __MUSIC163_H__

#include <stdint.h>
#include <string>

#include "music/lyric.h"

std::string music_163_dep(std::string &input);
bool music_lyric_163(uint64_t id, coloraudio::lyric::LyricParser **ldata, coloraudio::lyric::LyricParser **trdata);
bool music_lyric_163(std::string &comment, coloraudio::lyric::LyricParser **ldata, coloraudio::lyric::LyricParser **trdata);

#endif // __MUSIC163_H__