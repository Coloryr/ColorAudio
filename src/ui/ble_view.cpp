#include "ble_view.h"
#include "info_view.h"

#include "ui.h"
#include "info_view.h"
#include "view/view_ble.h"
#include "../wireless/ble.h"
#include "../wireless/ble_info.h"

#include "lvgl.h"

#include <string>

static lv_obj_t *view;

static std::string title;

static bool isplay;
static bool update;
static bool is_par;

static void timer_tick(lv_timer_t *timer)
{
    if (update)
    {
        lv_ble_set_title(ble_title.c_str());
        lv_ble_set_artlist(ble_artist.c_str());
        lv_ble_set_album(ble_album.c_str());

        if (ble_now_state == BLE_STATE_CONNECTED)
        {
            lv_ble_connect(ble_device.c_str());
        }
        else
        {
            lv_ble_disconnect();
        }

        update = false;
    }
}

static void back_dialog(bool stop)
{
    if (stop)
    {
        // change_mode(MAIN_MODE_NONE);
    }

    view_jump(VIEW_MAIN);

    view_dialog_close();
}

static void back_click(lv_event_t *e)
{
    view_dialog_show(back_dialog, "是否要同时退出蓝牙音乐模式");
}

static void prev_click(lv_event_t *e)
{
    if (ble_now_state != BLE_STATE_CONNECTED)
    {
        return;
    }
}

static void next_click(lv_event_t *e)
{
    if (ble_now_state != BLE_STATE_CONNECTED)
    {
        return;
    }
}

static void volume_click(lv_event_t *e)
{
}

static void mute_click(lv_event_t *e)
{
}

static void par_click(lv_event_t *e)
{
    ble_set_discoverable(true);
    ble_set_pairable(true);
}

static void play_click(lv_event_t *e)
{
    if (ble_now_state != BLE_STATE_CONNECTED)
    {
        return;
    }
    if (isplay)
    {
    }
    else
    {
    }
}

void view_ble_update_info()
{
    update = true;
}

void view_ble_set_par(uint32_t key)
{
    is_par = true;
    char temp[256] = {0};
    sprintf(temp, "设备请求配对，配对码：%d", key);
    view_top_info_display(temp);
}

void view_ble_set_par_close()
{
    if (is_par)
    {
        view_top_info_close();
        is_par = false;
    }
}

void view_ble_par_disable()
{
    lv_ble_par_display(false);
}

void view_ble_par_enable()
{
    lv_ble_par_display(true);
}

void view_ble_set_display(bool display)
{
    if (display)
    {
        lv_obj_remove_flag(view, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(view, LV_OBJ_FLAG_HIDDEN);
    }
}

void view_ble_create(lv_obj_t *parent)
{
    view = lv_ble_create(parent, back_click, prev_click, play_click,
                         next_click, volume_click, mute_click, par_click);
    lv_timer_create(timer_tick, 500, NULL);
}

void view_ble_tick()
{
}