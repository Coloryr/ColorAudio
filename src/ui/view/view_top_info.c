#include "view_top_info.h"

#include "../font.h"
#include "../anim.h"

#include "lvgl.h"

#include <stdbool.h>

static lv_obj_t *info_obj;
static lv_obj_t *text_obj;
static lv_obj_t *bar_obj;

static lv_anim_t anim;

static void scroll_event_cb(lv_event_t *e)
{
    lv_obj_t *cont = lv_event_get_target(e);
    if (lv_obj_get_scroll_x(cont) >= 200)
    {
        lv_obj_scroll_to_x(cont, 0, LV_ANIM_OFF);
    }
}

// 动画回调函数
static void anim_cb(void *bar, int32_t value)
{
    int32_t min = lv_bar_get_min_value(bar);
    int32_t max = lv_bar_get_max_value(bar);

    int32_t indicator_len = (max - min) * 0.2;
    int32_t start_pos = value % (max - indicator_len);

    lv_bar_set_start_value(bar, start_pos, LV_ANIM_OFF);
    lv_bar_set_value(bar, start_pos + indicator_len, LV_ANIM_OFF);
}

static lv_obj_t *create_bar(lv_obj_t *parent)
{
    lv_obj_t *bar = lv_bar_create(parent);

    lv_obj_set_style_pad_all(bar, 10, LV_PART_KNOB);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, 200, LV_PART_MAIN);

    lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x569af8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(bar, lv_color_hex(0xa666f1), LV_PART_INDICATOR);
    lv_obj_set_style_outline_width(bar, 0, 0);

    lv_obj_set_size(bar, 260, 20);
    lv_obj_center(bar);

    lv_bar_set_mode(bar, LV_BAR_MODE_RANGE);

    lv_anim_init(&anim);
    lv_anim_set_var(&anim, bar);
    lv_anim_set_values(&anim, 0, 79);
    lv_anim_set_exec_cb(&anim, anim_cb);
    lv_anim_set_duration(&anim, 1000);
    lv_anim_set_reverse_duration(&anim, 1000);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);

    lv_anim_start(&anim);

    return bar;
}

lv_obj_t *lv_info_create(lv_obj_t *parent)
{
    info_obj = lv_obj_create(parent);
    lv_obj_remove_style_all(info_obj);
    lv_obj_set_size(info_obj, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_opa(info_obj, 150, 0);
    lv_obj_set_style_bg_color(info_obj, lv_color_hex(0x000000), 0);

    text_obj = lv_label_create(info_obj);
    lv_obj_align(text_obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(text_obj, font_22, 0);
    lv_obj_set_style_text_color(text_obj, lv_color_hex(0xffffff), 0);

    bar_obj = create_bar(info_obj);

    lv_info_scr_display(false);

    lv_obj_align_to(bar_obj, text_obj, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    lv_obj_add_flag(info_obj, LV_OBJ_FLAG_HIDDEN);

    return info_obj;
}

void lv_info_scr_display(bool display)
{
    if (display)
    {
        lv_anim_resume(&anim);
        lv_obj_remove_flag(bar_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_anim_pause(&anim);
        lv_obj_add_flag(bar_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void lv_info_display(bool display)
{
    anim_set_display(info_obj, display);
}

void lv_info_set_text(const char *text)
{
    lv_label_set_text(text_obj, text);
}