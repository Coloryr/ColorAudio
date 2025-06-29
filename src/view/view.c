#include "view.h"

#include "music_list.h"
#include "music_main.h"
#include "view_input.h"
#include "player.h"
#include "sound.h"
#include "local_music.h"
#include "lyric.h"
#include "view_lyric.h"
#include "view_top_info.h"
#include "view_main.h"

#include "lvgl.h"
#include "mln_list.h"
#include "mln_utils.h"

#include <stdint.h>
#include <pthread.h>

static void timer_tick(lv_timer_t *timer);

static lv_obj_t *ctrl;
static lv_obj_t *list;
static lv_obj_t *main;

static bool update_info;
static bool update_img;
static bool update_state;
static bool update_list;
static bool update_list_index;
static bool init_list;

static lyric_node_t *lyric_data = NULL;
static lyric_node_t *lyric_tr_data = NULL;

static pthread_mutex_t lyric_mutex;

static void lyric_tick(lv_timer_t *timer)
{
    pthread_mutex_lock(&lyric_mutex);

    if (lyric_data == NULL)
    {
        lv_lyric_set_text("");
    }
    else
    {
        uint8_t *data = lyric_find(lyric_data, time_now * 1000);
        if (data)
        {
            lv_lyric_set_text(data);
        }
        else
        {
            lv_lyric_set_text("");
        }
    }
    if (lyric_tr_data == NULL)
    {
        lv_lyric_tr_set_text("");
    }
    else
    {
        uint8_t *data = lyric_find(lyric_tr_data, time_now * 1000);
        if (data)
        {
            lv_lyric_tr_set_text(data);
        }
        else
        {
            lv_lyric_tr_set_text("");
        }
    }

    pthread_mutex_unlock(&lyric_mutex);
}

static void timer_tick(lv_timer_t *timer)
{
    lv_music_set_all_time(time_all);
    lv_music_set_now_time(time_now);

    if (update_img)
    {
        if (image.data)
        {
            lv_music_set_image(image.data, image.size);
        }
        else
        {
            lv_music_set_image(NULL, 0);
        }

        update_img = false;
    }

    if (update_info)
    {
        if (title.data)
        {
            lv_music_set_title(title.data);
        }
        else
        {
            lv_music_set_title("");
        }

        if (album.data)
        {
            lv_music_set_album(album.data);
        }
        else
        {
            lv_music_set_album("");
        }

        if (auther.data)
        {
            lv_music_set_auther(auther.data);
        }
        else
        {
            lv_music_set_auther("");
        }

        lv_music_set_sound_info(pcm_now_format, pcm_now_rate, pcm_now_channels);

        update_info = false;
    }

    if (update_state)
    {
        if (get_play_state() == MUSIC_STATE_PLAY)
        {
            lv_music_set_play();
        }
        else
        {
            lv_music_set_pause();
        }

        lv_music_set_play_mode();
        lv_music_set_volume(alsa_get_volume());

        update_state = false;
    }

    if (init_list)
    {
        lv_list_clear();
        play_item *t;
        for (t = mln_container_of(mln_list_head(&local_play_list), play_item, node);
             t != NULL;
             t = mln_container_of(mln_list_next(&t->node), play_item, node))
        {
            lv_list_add_item(t);
        }
        init_list = false;
    }

    if (update_list)
    {
        play_item *t;
        for (t = mln_container_of(mln_list_head(&local_play_list), play_item, node);
             t != NULL;
             t = mln_container_of(mln_list_next(&t->node), play_item, node))
        {
            lv_list_button_reload(t);
        }
        update_list = false;
    }

    if (update_list_index)
    {
        lv_music_list_button_check(play_now_index, true);
        if (play_list_count > 1)
        {
            if (play_now_index - 1 >= 0)
            {
                lv_music_list_button_check(play_now_index - 1, false);
            }
            if (play_now_index + 1 < play_list_count)
            {
                lv_music_list_button_check(play_now_index + 1, false);
            }
        }

        update_list_index = false;
    }
}

void view_update_list_index()
{
    update_list_index = true;
}
void view_init_list()
{
    init_list = true;
}
void view_update_list()
{
    update_list = true;
}
void view_update_info()
{
    update_info = true;
}
void view_update_img()
{
    update_img = true;
}
void view_update_state()
{
    update_state = true;
}

void view_set_lyric(lyric_node_t *lyric, lyric_node_t *tlyric)
{
    pthread_mutex_lock(&lyric_mutex);
    lyric_node_t *temp = lyric_data;
    lyric_node_t *temp1 = lyric_tr_data;
    lyric_data = lyric;
    lyric_tr_data = tlyric;
    pthread_mutex_unlock(&lyric_mutex);
    if (temp)
    {
        lyric_close(temp);
    }

    if (temp1)
    {
        lyric_close(temp1);
    }
}

void view_init()
{
    pthread_mutex_init(&lyric_mutex, NULL);

    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x343247), 0);

    list = lv_music_list_create(lv_screen_active());
    ctrl = lv_music_main_create(lv_screen_active());

    // main = lv_main_create(lv_screen_active());

    lv_input_create(lv_screen_active());
    lv_info_create(lv_screen_active());

    lv_timer_create(timer_tick, 500, NULL);
    lv_timer_create(lyric_tick, 50, NULL);

    // lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);
    // lv_obj_add_flag(ctrl, LV_OBJ_FLAG_HIDDEN);
}

void view_go_music()
{
    lv_obj_add_flag(ctrl, LV_OBJ_FLAG_HIDDEN);

    lv_obj_remove_flag(list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ctrl, LV_OBJ_FLAG_HIDDEN);

    lv_music_fadein();
}

void view_go_main()
{
    lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ctrl, LV_OBJ_FLAG_HIDDEN);

    lv_obj_remove_flag(ctrl, LV_OBJ_FLAG_HIDDEN);
}

void view_tick()
{
    if (get_play_state() == MUSIC_STATE_PLAY)
    {
        lv_music_fft_load();
    }
}
