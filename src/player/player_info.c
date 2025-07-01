#include "player_info.h"

#include <stdint.h>

music_run_type music_run = MUSIC_RUN_UNKNOW;
music_type play_music_type = MUSIC_TYPE_UNKNOW;
music_state play_state = MUSIC_STATE_UNKNOW;
music_mode play_music_mode = MUSIC_MODE_LOOP;

uint32_t play_music_bps;
uint32_t play_now_index;
uint32_t play_list_count;

float target_time = 0;
float time_all = 0;
float time_now = 0;
