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

void music_test_run(music_run_type type);
void music_go_local();
void music_go_net();

#endif