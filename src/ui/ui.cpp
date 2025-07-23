#include "ui.h"

#include "music_view.h"
#include "info_view.h"
#include "main_view.h"
#include "input_view.h"

#include "../player/player_info.h"
#include "../player/player.h"
#include "../player/sound.h"

#include "lvgl.h"

#include <stdint.h>
#include <pthread.h>
#include <stack>
#include <string>

void view_init()
{
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x343247), 0);

    // view_main_create(lv_screen_active());
    view_music_create(lv_screen_active());

    view_input_create(lv_screen_active());
    view_top_info_create(lv_screen_active());

    // view_music_set_display(false);
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
    view_music_tick();
}
