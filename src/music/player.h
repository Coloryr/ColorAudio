#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "mp3id3.h"
#include "player_flac.h"

#include "mln_list.h"

#include <stdbool.h>
#include <stdint.h>

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
    MUSIC_COMMAND_UNKNOW = -1
} music_command;

typedef enum
{
    MUSIC_STATE_PLAY = 0,
    MUSIC_STATE_PAUSE,
    MUSIC_STATE_STOP,
    MUSIC_STATE_UNKNOW = -1
} music_state;

typedef struct
{
    mln_list_t node;
    uint32_t index;
    uint8_t *path;
    uint32_t len;
    uint8_t* title;
    uint32_t title_len;
    uint8_t* auther;
    uint32_t auther_len;
    float time;
} play_item;

typedef struct
{
    uint8_t *data;
    uint32_t size;
} play_info_item;

typedef struct
{
    bool (* decode_init)(stream *st);
    bool (* decode_start)(stream *st);
    bool (* decode_close)();
} music_decoder;

extern float time_all;
extern float time_now;

extern play_info_item title;
extern play_info_item album;
extern play_info_item auther;
extern play_info_item image;

extern music_type play_music_type;
extern mln_list_t play_list;
extern uint32_t play_mp3_bps;
extern uint32_t play_now_index;
extern uint32_t play_list_count;

void play_info_update_id3(mp3id3 *id3);
void play_info_update_flac(flac_metadata* data);
void play_jump_index(uint32_t index);

void play_init();

bool play_command(music_command command);

music_state get_play_state();

#endif