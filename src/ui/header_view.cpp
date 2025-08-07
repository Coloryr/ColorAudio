#include "header_view.h"
#include "info_view.h"
#include "ui.h"

#include "view/view_header.h"

#include "../main.h"

#include "lvgl.h"

static void back_dialog(bool stop)
{
    if (stop)
    {
        change_mode(MAIN_MODE_NONE);
    }

    view_jump(VIEW_MAIN);

    view_dialog_close();
}

static void back_button(lv_event_t *event)
{
    view_dialog_show(back_dialog, "是否要同时关闭播放");
}

void view_header_back_display(bool display)
{
    
}

void view_header_create(lv_obj_t *parent)
{
    lv_header_create(parent, back_button);
}
