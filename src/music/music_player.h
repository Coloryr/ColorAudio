#ifndef _MUSIC_PLAYER_H_
#define _MUSIC_PLAYER_H_

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <map>

#include <boost/container/vector.hpp>

#include "common/data_item.h"
#include "music/music.h"
#include "stream/stream.h"

extern std::string play_title;
extern std::string play_album;
extern std::string play_auther;
extern coloraudio::common::DataItem* play_image;

extern boost::container::vector<play_item *> play_list;

extern pthread_mutex_t play_mutex;
extern pthread_cond_t play_start; 

extern coloraudio::stream::BaseStream *play_st;

extern bool play_need_seek;

void play_update_text(std::string text, music_info_type type);
void play_update_image(coloraudio::common::DataItem* data, music_info_type type);

void play_jump_time(float time);
void play_jump_end();

void play_clear();
void play_init();

bool play_set_command(music_command command);
music_command play_get_command();

#endif