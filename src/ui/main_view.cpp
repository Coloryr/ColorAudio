#include "lvgl.h"

#include "header_view.h"
#include "lang.h"
#include "ui.h"
#include "view/view_main.h"
#include "main.h"
#include "music/music.h"
#include "music/local_music.h"
#include "music/music_player.h"
#include "usb/usb_audio.h"

#include "main_view.h"

static lv_obj_t *main_view;

static void button_event_cb(lv_event_t *e)
{
    main_button_type *arg = static_cast<main_button_type *>(lv_event_get_user_data(e));
    switch (*arg)
    {
    case MAIN_BUTTON_MUSIC:
        change_mode(MAIN_MODE_MUSIC);
        view_jump(VIEW_MUSIC);
        break;
    case MAIN_BUTTON_BLE:
        change_mode(MAIN_MODE_BLE);
        view_jump(VIEW_BLE);
        break;
    case MAIN_BUTTON_USB:
        change_mode(MAIN_MODE_USB);
        view_jump(VIEW_USB);
        break;
    case MAIN_BUTTON_SETTING:
        view_jump(VIEW_SETTING);
        break;
    default:
        break;
    }
}

static void reload_text()
{
    switch (get_mode())
    {
    case MAIN_MODE_MUSIC:
        if (get_music_run() == MUSIC_RUN_LOCAL)
        {
            if (local_music_scan_now)
            {
                lv_main_set_now(now_lang->main_text8);
            }
            else
            {
                if (play_title.empty())
                {
                    lv_main_set_now(now_lang->main_text6);
                }
                else
                {
                    char temp[256];
                    sprintf(temp, now_lang->main_text7, play_title.c_str());
                    lv_main_set_now(temp);
                }
            }
        }
        break;
    case MAIN_MODE_USB:
        if (usb_audio_is_connect())
        {
            lv_main_set_now(now_lang->music_text11);
        }
        else
        {
            lv_main_set_now(now_lang->music_text10);
        }
        break;
    default:
        lv_main_set_now(now_lang->title);
        break;
    }
}

static void main_tick(lv_timer_t *timer)
{
    if (get_view_mode() != VIEW_MAIN)
    {
        return;
    }

    reload_text();
}

void view_main_set_header()
{
    view_header_move(main_view);
    view_header_back_display(false, false);
}

void view_main_set_display(bool display)
{
    if (display)
    {
        lv_obj_remove_flag(main_view, LV_OBJ_FLAG_HIDDEN);
        reload_text();
    }
    else
    {
        lv_obj_add_flag(main_view, LV_OBJ_FLAG_HIDDEN);
    }
}

void view_main_create(lv_obj_t *parent)
{
    main_view = lv_main_create(parent, button_event_cb);

    lv_timer_create(main_tick, 500, NULL);
}
