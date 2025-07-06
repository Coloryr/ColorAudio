#include "ui.h"

#include "view.h"
#include "view/view_lyric.h"
#include "view/view_input.h"
#include "view/view_top_info.h"
#include "view/view_keyborad.h"
#include "view/view_music_main.h"
#include "view/view_music_list.h"
#include "music_list.h"
#include "music_main.h"
#include "info.h"

#include "../player/player_info.h"
#include "../player/player.h"
#include "../player/sound.h"

#include "lvgl.h"

#include <stdint.h>
#include <pthread.h>
#include <stack>

static void timer_tick(lv_timer_t *timer);

static lv_obj_t *ctrl_view;
static lv_obj_t *list_view;
static lv_obj_t *main_view;

static lyric_node_t *lyric_data = NULL;
static lyric_node_t *lyric_tr_data = NULL;

static uint32_t check_list_button;
static uint32_t uncheck_list_button;

static pthread_mutex_t lyric_mutex;

static lyric_state ly_state = LYRIC_UNKNOW;

static void lyric_tick(lv_timer_t *timer)
{
    pthread_mutex_lock(&lyric_mutex);

    bool find = false;
    if (lyric_data != NULL)
    {
        char *data = lyric_find(lyric_data, time_now * 1000);
        if (data)
        {
            find = true;
            lv_lyric_set_text(data);
        }
        else
        {
            lv_lyric_set_text("");
        }
    }
    if (lyric_tr_data != NULL)
    {
        if (!find)
        {
            lv_lyric_tr_set_text("");
        }
        else
        {
            char *data = lyric_find(lyric_tr_data, time_now * 1000);
            if (data)
            {
                lv_lyric_tr_set_text(data);
            }
            else
            {
                lv_lyric_tr_set_text("");
            }
        }
    }

    pthread_mutex_unlock(&lyric_mutex);
}

static void timer_tick(lv_timer_t *timer)
{
    lv_music_set_now_time(time_now);

    if (clear_info)
    {
        lv_music_fft_clear();
        lv_music_set_title("");
        clear_info = false;
    }

    if (update_img)
    {
        if (image)
        {
            lv_music_set_image(image->data, image->size);
        }
        else
        {
            lv_music_set_image(NULL, 0);
        }

        update_img = false;
    }

    if (update_info)
    {
        lv_music_set_all_time(time_all);
        lv_music_set_title(title.c_str());
        lv_music_set_album(album.c_str());
        lv_music_set_auther(auther.c_str());

        lv_music_set_sound_info(pcm_now_format, pcm_now_rate, pcm_now_channels, play_music_bps);

        update_info = false;
    }

    if (update_state)
    {
        if (play_state == MUSIC_STATE_PLAY)
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

        for (auto it = play_list.begin(); it != play_list.end(); ++it)
        {
            lv_list_add_item(it->second);
        }

        view_music_list_button_check(play_now_index, true);

        init_list = false;
    }

    if (update_list)
    {
        for (auto it = play_list.begin(); it != play_list.end(); ++it)
        {
            lv_muisc_list_item_reload(it->second);
        }
        update_list = false;
    }

    if (check_list_button != UINT32_MAX)
    {
        view_music_list_button_check(check_list_button, true);

        check_list_button = UINT32_MAX;
    }
    if (uncheck_list_button != UINT32_MAX)
    {
        view_music_list_button_check(uncheck_list_button, false);

        uncheck_list_button = UINT32_MAX;
    }

    if (update_top_info)
    {
        top_info_update();
        update_top_info = false;
    }

    if (ly_state == LYRIC_NONE)
    {
        view_set_lyric(nullptr, nullptr);
        lv_lyric_set_text("无歌词");
        lv_lyric_tr_set_text("");
        ly_state = LYRIC_UNKNOW;
    }
    else if (ly_state == LYRIC_FAIL)
    {
        view_set_lyric(nullptr, nullptr);
        lv_lyric_set_text("歌词获取失败");
        lv_lyric_tr_set_text("");
        ly_state = LYRIC_UNKNOW;
    }

    if (volume_down > 0)
    {
        volume_down--;
        if (volume_down <= 0)
        {
            lv_music_volume_display(false);
            volume_down = 0;
        }
    }
}

void view_set_lyric_state(lyric_state state)
{
    ly_state = state;
}

void view_set_check(uint32_t index, bool enable)
{
    if (enable)
    {
        check_list_button = index;
    }
    else
    {
        uncheck_list_button = index;
    }
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

    list_view = lv_music_list_create(lv_screen_active());
    ctrl_view = lv_music_main_create(lv_screen_active());

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
    lv_obj_add_flag(ctrl_view, LV_OBJ_FLAG_HIDDEN);

    lv_obj_remove_flag(list_view, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(ctrl_view, LV_OBJ_FLAG_HIDDEN);

    lv_music_fadein();
}

void view_go_main()
{
    lv_obj_add_flag(list_view, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ctrl_view, LV_OBJ_FLAG_HIDDEN);

    lv_obj_remove_flag(ctrl_view, LV_OBJ_FLAG_HIDDEN);
}

void view_tick()
{
    if (play_state == MUSIC_STATE_PLAY)
    {
        lv_music_fft_load();
    }
}
