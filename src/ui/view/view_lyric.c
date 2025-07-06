#include "view_lyric.h"

#include "../font.h"
#include "../view.h"

#include "lvgl.h"

#include <stdint.h>

static lv_obj_t *lyric_obj;
static lv_obj_t *lyric_tr_obj;

// 滚动结束锁定
static void scroll_end_handler(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target_obj(e);
    if (lv_event_get_code(e) == LV_EVENT_SCROLL_END)
    {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_state(obj, LV_STATE_SCROLLED); // 进入滚动结束状态
    }
}

lv_obj_t *lv_lyric_create(lv_obj_t *parent)
{
    uint32_t wid = lv_obj_get_width(parent);

    lv_obj_t *pan = lv_obj_create(parent);
    lv_obj_remove_style_all(pan);
    lv_obj_set_size(pan, wid, 110);

    lv_obj_t *pan1 = lv_obj_create(pan);
    lv_obj_remove_style_all(pan1);
    lv_obj_set_size(pan1, wid, LV_SIZE_CONTENT);
    lv_obj_set_align(pan1, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(pan1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(pan1, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lyric_obj = lv_label_create(pan1);
    lv_obj_set_style_text_font(lyric_obj, font_22, 0);
    lv_obj_set_style_text_color(lyric_obj, lv_color_hex(0x8199f7), 0);
    lv_obj_set_width(lyric_obj, wid - LV_DEMO_MUSIC_HANDLE_SIZE);
    lv_obj_set_style_text_align(lyric_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(lyric_obj, "");

    lyric_tr_obj = lv_label_create(pan1);
    lv_obj_set_style_text_font(lyric_tr_obj, font_18, 0);
    lv_obj_set_style_text_color(lyric_tr_obj, lv_color_hex(0x8199f7), 0);
    lv_obj_set_width(lyric_tr_obj, wid - LV_DEMO_MUSIC_HANDLE_SIZE);
    lv_obj_set_style_text_align(lyric_tr_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(lyric_tr_obj, "");

    return pan;
}

void lv_lyric_set_text(const char *text)
{
    lv_label_set_text(lyric_obj, text);
}

void lv_lyric_tr_set_text(const char *text)
{
    lv_label_set_text(lyric_tr_obj, text);
}
