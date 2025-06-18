#include "view.h"
#include "lvgl.h"

#include "music_list.h"
#include "music_main.h"

#include "player.h"
#include "sound.h"

#include "mln_list.h"
#include "mln_utils.h"

static lv_obj_t *ctrl;
static lv_obj_t *list;

static lv_timer_t *view_update_timer;

static bool update_info;
static bool update_img;
static bool update_state;
static bool update_list;
static bool update_list_index;
static bool init_list;

static void view_timer_tick(lv_timer_t *timer)
{
    view_set_all_time(time_all);
    view_set_now_time(time_now);

    if (update_img)
    {
        if (image.data)
        {
            view_set_image(image.data, image.size);
        }
        else
        {
            view_set_image(NULL, 0);
        }

        update_img = false;
    }

    if (update_info)
    {
        if (title.data)
        {
            view_set_title(title.data);
        }
        else
        {
            view_set_title("");
        }

        if (album.data)
        {
            view_set_album(album.data);
        }
        else
        {
            view_set_album("");
        }

        if (auther.data)
        {
            view_set_auther(auther.data);
        }
        else
        {
            view_set_auther("");
        }

        view_set_sound_info(pcm_now_format, pcm_now_rate, pcm_now_channels);

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

        update_state = false;
    }

    if (init_list)
    {
        view_list_clear();
        play_item *t;
        for (t = mln_container_of(mln_list_head(&play_list), play_item, node);
             t != NULL;
             t = mln_container_of(mln_list_next(&t->node), play_item, node))
        {
            view_add_list_button(t);
        }
        init_list = false;
    }

    if (update_list)
    {
        play_item *t;
        for (t = mln_container_of(mln_list_head(&play_list), play_item, node);
             t != NULL;
             t = mln_container_of(mln_list_next(&t->node), play_item, node))
        {
            view_list_button_reload(t);
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

void view_init()
{
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x343247), 0);

    list = lv_music_list_create(lv_screen_active());
    ctrl = lv_music_main_create(lv_screen_active());

    view_update_timer = lv_timer_create(view_timer_tick, 500, NULL);
}

void view_tick()
{
    if (get_play_state() == MUSIC_STATE_PLAY)
    {
        view_fft_load();
    }
}
