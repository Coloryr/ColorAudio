#include "view_wave.h"

void lv_wave_images_create(lv_obj_t *parent, uint8_t type)
{
    if (type == 0)
    {
        LV_IMAGE_DECLARE(img_lv_demo_music_wave_top);
        LV_IMAGE_DECLARE(img_lv_demo_music_wave_bottom);
        lv_obj_t *wave_top = lv_image_create(parent);
        lv_image_set_src(wave_top, &img_lv_demo_music_wave_top);
        lv_image_set_inner_align(wave_top, LV_IMAGE_ALIGN_TILE);
        lv_obj_set_width(wave_top, LV_HOR_RES);
        lv_obj_align(wave_top, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_add_flag(wave_top, LV_OBJ_FLAG_IGNORE_LAYOUT);

        lv_obj_t *wave_bottom = lv_image_create(parent);
        lv_image_set_src(wave_bottom, &img_lv_demo_music_wave_bottom);
        lv_image_set_inner_align(wave_bottom, LV_IMAGE_ALIGN_TILE);
        lv_obj_set_width(wave_bottom, LV_HOR_RES);
        lv_obj_align(wave_bottom, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_flag(wave_bottom, LV_OBJ_FLAG_IGNORE_LAYOUT);
    }
    else if (type == 1)
    {
        LV_IMAGE_DECLARE(img_lv_demo_music_corner_left);
        LV_IMAGE_DECLARE(img_lv_demo_music_corner_right);
        lv_obj_t *wave_corner = lv_image_create(parent);
        lv_image_set_src(wave_corner, &img_lv_demo_music_corner_left);
        lv_obj_align(wave_corner, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        lv_obj_add_flag(wave_corner, LV_OBJ_FLAG_IGNORE_LAYOUT);

        wave_corner = lv_image_create(parent);
        lv_image_set_src(wave_corner, &img_lv_demo_music_corner_right);
        lv_obj_align(wave_corner, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
        lv_obj_add_flag(wave_corner, LV_OBJ_FLAG_IGNORE_LAYOUT);
    }
    else if (type == 2)
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
}