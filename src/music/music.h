#ifndef _MUSIC_H_
#define _MUSIC_H_

#include <string>
#include <stdint.h>

#include "stream/stream.h"

typedef struct
{
    uint32_t index;
    char* path;
    char* title;
    char* auther;
    float time;
} play_item;

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
} music_mode_type;

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
extern music_mode_type play_music_mode;

extern uint32_t play_music_bps;
extern uint32_t play_now_index;
extern uint32_t play_list_count;

extern float time_all;
extern float time_now;
extern float target_time;

music_type music_test_type(coloraudio::stream::BaseStream *st);

void music_get_lyric(std::string &comment);

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