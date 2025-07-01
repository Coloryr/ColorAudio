#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "player_info.h"

#include "../common/data_item.h"
#include "../music/music.h"
#include "../stream/stream.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <map>

extern std::string title;
extern std::string album;
extern std::string auther;
extern DataItem* image;

extern std::map<uint32_t, play_item *> play_list;

extern pthread_mutex_t play_mutex;
extern pthread_cond_t play_start; 

extern ColorAudio::Stream *play_st;

music_type play_test_music_type(ColorAudio::Stream *st);

void play_update_text(std::string text, music_info_type type);
void play_update_image(DataItem* data, music_info_type type);

void play_jump_index(uint32_t index);
void play_jump_time(float time);
void play_jump_end();

void play_set_volume(uint16_t value);

void play_init();

bool play_set_command(music_command command);
music_command play_get_command();

#endif