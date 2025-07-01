#include "view_main.h"

#include "lvgl.h"

static void create_wave_images(lv_obj_t *parent)
{
    LV_IMAGE_DECLARE(img_lv_bg1);
    LV_IMAGE_DECLARE(img_lv_bg2);
    lv_obj_t *wave_top = lv_image_create(parent);
    lv_image_set_src(wave_top, &img_lv_bg2);
    lv_image_set_inner_align(wave_top, LV_IMAGE_ALIGN_TILE);
    lv_obj_set_width(wave_top, LV_HOR_RES);
    lv_obj_align(wave_top, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_flag(wave_top, LV_OBJ_FLAG_IGNORE_LAYOUT);

    lv_obj_t *wave_bottom = lv_image_create(parent);
    lv_image_set_src(wave_bottom, &img_lv_bg1);
    lv_image_set_inner_align(wave_bottom, LV_IMAGE_ALIGN_TILE);
    lv_obj_set_width(wave_bottom, LV_HOR_RES);
    lv_obj_align(wave_bottom, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(wave_bottom, LV_OBJ_FLAG_IGNORE_LAYOUT);
}

static void local_music_event_cb(lv_event_t *e)
{
    //view_go_music();
}

static void net_music_event_cb(lv_event_t *e)
{
    //view_go_music();
}

lv_obj_t *lv_main_create(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(obj, 255, 0);
    lv_obj_set_size(obj, LV_HOR_RES, LV_HOR_RES);

    lv_obj_t *cout = lv_obj_create(obj);
    lv_obj_remove_style_all(cout);
    lv_obj_align(cout, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(cout, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    static const int32_t grid_cols[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    static const int32_t grid_rows[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

    lv_obj_t *button = lv_obj_create(cout);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, "本地音乐");
    lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(button, local_music_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_grid_cell(button, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    button = lv_obj_create(cout);
    label = lv_label_create(button);
    lv_label_set_text(label, "网络音乐");
    lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(button, local_music_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_grid_cell(button, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
}