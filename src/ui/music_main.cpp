#include "music_main.h"
#include "view.h"
#include "view/view_music_main.h"

#include "../player/player.h"

#include "lvgl.h"

#include <malloc.h>
#include <string.h>

static void mode_click_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    play_set_command(MUSIC_COMMAND_CHANGE_MODE);
}

static void prev_click_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    play_set_command(MUSIC_COMMAND_LAST);
}

static void next_click_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    play_set_command(MUSIC_COMMAND_NEXT);
}

static void time_change_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target_obj(e);
    if (!lv_slider_is_dragged(obj))
    {
        uint32_t value = lv_slider_get_value(obj);

        play_jump_time(value);
    }
}

static void play_event_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target_obj(e);
    if (lv_obj_has_state(obj, LV_STATE_CHECKED))
    {
        if (play_set_command(MUSIC_COMMAND_PLAY))
        {
            lv_music_set_play();
        }
    }
    else
    {
        if (play_set_command(MUSIC_COMMAND_PAUSE))
        {
            lv_music_set_pause();
        }
    }
}

static void volume_click_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target_obj(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        int sel = lv_slider_get_value(obj);
        play_set_volume(sel);
    }
    else if (code == LV_EVENT_PRESSED)
    {
        volume_down = INT32_MAX;
    }
    else if (code == LV_EVENT_RELEASED)
    {
        volume_down = LV_MUSIC_VOLUME_DISPLAY_TIME;
    }
}

static void mute_click_event_cb(lv_event_t *e)
{
    play_set_volume(0);
    lv_music_set_volume(0);
    volume_down = LV_MUSIC_VOLUME_DISPLAY_TIME;
}

lv_obj_t *lv_music_main_create(lv_obj_t *parent)
{
    return view_music_main_create(parent, time_change_event_cb,
                                  volume_click_event_cb, mode_click_event_cb,
                                  prev_click_event_cb, play_event_click_cb,
                                  next_click_event_cb, mute_click_event_cb);
}