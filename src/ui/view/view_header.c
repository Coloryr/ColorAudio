#include "view_header.h"

#include "../font.h"

#include "lvgl.h"

#include <stdbool.h>

static lv_obj_t *button_back_obj;
static lv_obj_t *headphone1_obj;

void lv_header_back_display(bool display)
{
    if (display)
    {
        lv_obj_remove_flag(button_back_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_fade_in(button_back_obj, 1000, 0);
    }
    else
    {
        lv_obj_add_flag(button_back_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t *lv_header_create(lv_obj_t *parent, lv_event_cb_t back)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, LV_HOR_RES, 40);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    LV_IMAGE_DECLARE(img_lv_back);

    button_back_obj = lv_image_create(obj);
    lv_image_set_src(button_back_obj, &img_lv_back);
    lv_obj_set_size(button_back_obj, 40, 40);
    lv_obj_add_event_cb(button_back_obj, back, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(button_back_obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_align(button_back_obj, LV_ALIGN_TOP_LEFT, 10, 0);

    lv_obj_add_flag(button_back_obj, LV_OBJ_FLAG_HIDDEN);

    LV_IMAGE_DECLARE(img_lv_headphone);

    headphone1_obj = lv_obj_create(obj);
    lv_obj_set_size(headphone1_obj, 60, 40);
    lv_obj_remove_style_all(headphone1_obj);
    lv_obj_align(headphone1_obj, LV_ALIGN_RIGHT_MID, 60, 0);

    lv_obj_t* icon = lv_image_create(headphone1_obj);
    lv_image_set_src(icon, &img_lv_headphone);
    lv_obj_set_size(icon, 40, 40);
    lv_obj_align(icon, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t* label = lv_label_create(headphone1_obj);
    lv_obj_set_style_text_font(label, font_18, 0);

    lv_obj_add_flag(headphone1_obj, LV_OBJ_FLAG_HIDDEN);

    return obj;
}