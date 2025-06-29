#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "mp3id3.h"
#include "player_flac.h"
#include "data_item.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

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
    MUSIC_STATE_SEEK,
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

typedef struct
{
    bool (*decode_init)(stream_t *st);
    bool (*decode_start)(stream_t *st);
    bool (*decode_close)();
} music_decoder;

extern float time_all;
extern float time_now;
extern float target_time;

extern data_item_t title;
extern data_item_t album;
extern data_item_t auther;
extern data_item_t image;

extern music_mode play_music_mode;
extern music_type play_music_type;
extern uint32_t play_mp3_bps;
extern uint32_t play_now_index;
extern uint32_t play_list_count;

extern pthread_mutex_t play_mutex;
extern pthread_cond_t play_start; 

extern stream_t *play_st;

music_type play_test_music_type(stream_t *st);

void play_info_update_id3(mp3id3 *id3);
void play_info_update_flac(flac_metadata_t *data);
void play_info_update_raw(uint8_t* data, uint32_t size, music_info_type type);

void play_jump_index(uint32_t index);
void play_jump_time(float time);
void play_jump_end();

void play_set_volume(uint16_t value);

void play_init();

bool play_command(music_command command);

music_state get_play_state();
music_command get_play_command();

#endif