#include "ui.h"

#include "music_view.h"
#include "info_view.h"
#include "main_view.h"
#include "input_view.h"

#include "../music/player_info.h"
#include "../music/music_player.h"
#include "../sound/sound.h"

#include "lvgl.h"

#include <stdint.h>
#include <pthread.h>
#include <stack>
#include <string>

static view_type now_type = VIEW_MAIN;

void view_init()
{
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x343247), 0);

    view_main_create(lv_screen_active());
    view_music_create(lv_screen_active());

    view_input_create(lv_screen_active());
    view_top_info_create(lv_screen_active());

    view_music_set_display(false);
}

void view_jump(view_type type)
{
    if(now_type == type)
    {
        return;
    }

    if(now_type == VIEW_MAIN)
    {
        view_main_set_display(false);
    }
    else if(now_type == VIEW_MUSIC)
    {
        view_music_set_display(false);
    }

    if(now_type == VIEW_MAIN)
    {
        view_main_set_display(true);
    }
    else if(type == VIEW_MUSIC)
    {
        view_music_set_display(true);
    }

    now_type = type;
}

void view_tick()
{
    view_music_tick();
}
