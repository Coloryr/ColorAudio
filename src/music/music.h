#ifndef _MUSIC_H_
#define _MUSIC_H_

#include "player_info.h"

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

music_run_type get_music_run();

void music_start();
void music_end();
void music_next();
void music_go_local();
void music_close();

void music_init();
void music_run_loop();

#endif