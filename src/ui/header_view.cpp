#include "lvgl.h"

#include "info_view.h"
#include "ui.h"
#include "lang.h"
#include "view/view_header.h"
#include "io/event.h"
#include "main.h"

#include "header_view.h"

static lv_obj_t *header;

static bool update;
static bool back_none;

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
    if (back_none)
    {
        view_jump(VIEW_MAIN);
        return;
    }
    view_dialog_show(back_dialog, now_lang->main_text5);
}

static void header_timer(lv_timer_t *timer)
{
    if (update)
    {
        update = false;
#ifdef BUILD_ARM
        view_header_headphone1(headphone_1_in);
        view_header_headphone2(headphone_2_in);
#endif
    }
}

void view_header_wifi(bool off, wifi_rf_state state)
{
    if (off)
    {
        lv_header_wifi_display(false);
        return;
    }

    lv_header_wifi_display(true);
    lv_header_wifi_set_state(state);
}

void view_header_headphone1(bool in)
{
    lv_header_headphone_display(0, in);
}

void view_header_headphone2(bool in)
{
    lv_header_headphone_display(1, in);
}

void view_header_move(lv_obj_t *parent)
{
    lv_obj_set_parent(header, parent);
    lv_obj_move_foreground(header);
}

void view_header_back_display(bool display, bool none)
{
    lv_header_back_display(display);
    back_none = none;
}

void view_header_update()
{
    update = true;
}

void view_header_create(lv_obj_t *parent)
{
    header = lv_header_create(parent, back_button);
    lv_timer_create(header_timer, 500, NULL);
    view_header_update();
}
