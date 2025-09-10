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

#define DEFAULT_STR_MAP 1024 * 1024

extern std::string play_title;
extern std::string play_album;
extern std::string play_auther;
extern coloraudio::common::DataItem* play_image;

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

void play_list_close();
void play_list_add_item(std::string &path, std::string &title, std::string &auther, float time);
void play_list_sort_by_pinyin();
bool play_list_empty();
void play_list_read_done();
play_item* play_list_get_now();
play_item* play_list_get_item(uint32_t index);
uint32_t play_list_get_count();

#endif