#ifndef _MUSIC_H_
#define _MUSIC_H_

#include "../player/player_info.h"

#include <string>

typedef struct
{
    uint32_t index;
    std::string path;
    std::string title;
    std::string auther;
    float time;
} play_item;

extern uint32_t play_now_index;
extern uint32_t play_list_count;

void play_jump_index(uint32_t index);
void play_jump_index_clear();
uint32_t get_jump_index();
bool have_jump_index();

void music_lyric_163(uint64_t id);

void music_start();
void music_end();
void music_next();
void music_test_run(music_run_type type);
void music_go_local();
void music_go_net();

void music_init();

#endif