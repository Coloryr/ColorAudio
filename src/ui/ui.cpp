#include "ui.h"

#include "music_view.h"
#include "info_view.h"
#include "view_state.h"
#include "view/view_main.h"
#include "view/view_input.h"
#include "view/view_top_info.h"
#include "view/view_keyborad.h"

#include "../player/player_info.h"
#include "../player/player.h"
#include "../player/sound.h"

#include "lvgl.h"

#include <stdint.h>
#include <pthread.h>
#include <stack>
#include <string>

static lv_obj_t *main_view;
static lv_obj_t *music_view;

static void timer_tick(lv_timer_t *timer)
{
    if (update_top_info)
    {
        top_info_update();
        update_top_info = false;
    }
}

void view_init()
{
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x343247), 0);

    // main_view = lv_main_create(lv_screen_active());

    music_view = lv_music_view_create(lv_screen_active());

    lv_input_create(lv_screen_active());
    lv_info_create(lv_screen_active());

    // lv_obj_add_flag(list_view, LV_OBJ_FLAG_HIDDEN);
    // lv_obj_add_flag(ctrl_view, LV_OBJ_FLAG_HIDDEN);

    lv_timer_create(timer_tick, 500, NULL);
}

void view_go_music()
{
    // lv_obj_add_flag(main_view, LV_OBJ_FLAG_HIDDEN);

    //
    //
}

void view_go_main()
{
    //
    //

    // lv_obj_remove_flag(main_view, LV_OBJ_FLAG_HIDDEN);
}

void view_tick()
{
    lv_music_view_tick();
}
