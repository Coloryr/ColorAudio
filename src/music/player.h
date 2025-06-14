#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "mp3id3.h"

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
    MUSIC_STATE_PLAY = 0,
    MUSIC_STATE_PAUSE,
    MUSIC_STATE_STOP,
    MUSIC_STATE_UNKNOW = -1
} music_state;

typedef struct
{
    uint8_t *path;
    uint16_t len;
    mln_list_t node;
} play_item;

typedef struct
{
    uint8_t *data;
    uint32_t size;
} play_info_item;

#ifdef __cplusplus
extern "C"
{
#endif

    extern float time_all;
    extern float time_now;

    extern play_info_item title;
    extern play_info_item album;
    extern play_info_item auther;
    extern play_info_item image;

    extern bool is_play;
    extern bool is_pause;
    extern bool is_stop;

    extern int32_t *buf;
    extern int32_t *buf1;
    extern uint32_t last_pcm_size;
    extern uint8_t last_pcm_bps;

    void play_index(uint32_t index);
    void play_info_update_id3(mp3id3 *id3);
    void play_info_update_image(uint8_t *data, uint32_t size);
    void play_info_update_title(uint8_t *data, uint32_t size);
    void play_info_update_album(uint8_t *data, uint32_t size);
    void play_info_update_auther(uint8_t *data, uint32_t size);
    void fill_fft(uint16_t rate, uint16_t size, int32_t data[], uint32_t down);
    void play_init();
    void play_set_state(bool isplay);
    void play_file(char *path);

    bool play_resume();
    bool play_pause();
    void play_stop();
    void play_next();
    void play_last();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif