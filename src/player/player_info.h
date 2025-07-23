#ifndef _PLAYER_INFO_H_
#define _PLAYER_INFO_H_

#include <stdint.h>

typedef enum {
    MUSIC_RUN_LOCAL = 0,
    MUSIC_RUN_NET,
    MUSIC_RUN_UNKNOW = -1
} music_run_type;

typedef enum
{
    MUSIC_MODE_LOOP = 0,
    MUSIC_MODE_RND,
    MUSIC_MODE_UNKNOW = -1
} music_mode;

typedef enum
{
    MUSIC_TYPE_WAV = 0,
    MUSIC_TYPE_MP3,
    MUSIC_TYPE_FLAC,
    MUSIC_TYPE_NCM,
    MUSIC_TYPE_UNKNOW = -1
} music_type;

typedef enum
{
    MUSIC_COMMAND_PLAY = 0,
    MUSIC_COMMAND_PAUSE,
    MUSIC_COMMAND_STOP,
    MUSIC_COMMAND_NEXT,
    MUSIC_COMMAND_LAST,
    MUSIC_COMMAND_CHANGE_MODE,
    MUSIC_COMMAND_JUMP_TIME,
    MUSIC_COMMAND_UNKNOW = -1
} music_command;

typedef enum
{
    MUSIC_STATE_PLAY = 0,
    MUSIC_STATE_PAUSE,
    MUSIC_STATE_STOP,
    MUSIC_STATE_UNKNOW = -1
} music_state;

typedef enum
{
    MUSIC_INFO_TITLE = 0,
    MUSIC_INFO_AUTHER,
    MUSIC_INFO_ALBUM,
    MUSIC_INFO_IMAGE,
    MUSIC_INFO_UNKNOW = -1
} music_info_type;

extern music_type play_music_type;
extern music_state play_state;
extern music_mode play_music_mode;

extern uint32_t play_music_bps;
extern uint32_t play_now_index;
extern uint32_t play_list_count;

extern float time_all;
extern float time_now;
extern float target_time;

#endif