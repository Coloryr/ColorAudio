#include "view_volume.h"

#include "ui/anim.h"

#include "lvgl.h"

#include <stdbool.h>

static const int32_t grid_col[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
static const int32_t grid_row[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

static void volume_display(lv_obj_t *view, bool display)
{
    if (display)
    {
        lv_obj_remove_flag(view, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(view);

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)anim_opa_cb);
        lv_anim_set_values(&a, 0, 255);
        lv_anim_set_duration(&a, 200);
        lv_anim_set_var(&a, view);
        lv_anim_start(&a);
    }
    else
    {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)anim_opa_cb);
        lv_anim_set_values(&a, 255, 0);
        lv_anim_set_duration(&a, 200);
        lv_anim_set_var(&a, view);
        lv_anim_start(&a);
    }
}

static void slider_cb(lv_event_t *e)
{
    view_volume_t *obj = lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED)
    {
        obj->display_down = UINT8_MAX;
    }
    else if (code == LV_EVENT_RELEASED)
    {
        obj->display_down = LV_MUSIC_VOLUME_DISPLAY_TIME;
    }
}

static void volume_tick(lv_timer_t *timer)
{
    view_volume_t *data = lv_timer_get_user_data(timer);
    if (data->display_down > 0)
    {
        data->display_down--;
        if (data->display_down <= 0)
        {
            data->is_display = false;
            data->display_down = 0;
            volume_display(data->view, false);
        }
    }
}

void lv_volume_click(lv_obj_t *view)
{
    view_volume_t *data = lv_obj_get_user_data(view);
    if (data->is_display)
    {
        data->is_display = false;
        data->display_down = 0;
    }
    else
    {
        data->is_display = true;
        data->display_down = LV_MUSIC_VOLUME_DISPLAY_TIME;
    }

    volume_display(data->view, data->is_display);
}

void lv_volume_close(lv_obj_t *view)
{
    view_volume_t *data = lv_obj_get_user_data(view);
    if (data->is_display)
    {
        data->is_display = false;
        data->display_down = 0;
        volume_display(data->view, false);
    }
}

void lv_volume_set_value(lv_obj_t *view, int32_t value)
{
    view_volume_t *data = lv_obj_get_user_data(view);
    lv_slider_set_value(data->slider, (int32_t)value, LV_ANIM_OFF);
}

void lv_volume_timer_close(lv_obj_t *view)
{
    view_volume_t *data = lv_obj_get_user_data(view);
    if (data->timer)
    {
        lv_timer_delete(data->timer);
        data->timer = NULL;
    }
}

void lv_volume_set_dir_hor(lv_obj_t *view, int width)
{
    view_volume_t *data = lv_obj_get_user_data(view);
    lv_obj_set_style_bg_grad_dir(data->slider, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);

    lv_obj_set_height(data->slider, 12);
    lv_obj_set_width(data->slider, width);

    lv_obj_set_grid_dsc_array(view, grid_row, grid_col);

    lv_obj_set_style_margin_top(data->mute, 0, 0);
    lv_obj_set_style_margin_left(data->mute, 10, 0);

    lv_obj_set_grid_cell(data->slider, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_grid_cell(data->mute, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
}

void lv_volume_show(lv_obj_t *view)
{
    view_volume_t *data = lv_obj_get_user_data(view);
    data->is_display = true;
    data->display_down = 0;
    volume_display(view, true);
}

lv_obj_t *lv_volume_create(lv_obj_t *parent, lv_event_cb_t volume, lv_event_cb_t mute)
{
    view_volume_t *data = lv_malloc(sizeof(view_volume_t));

    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_grid_dsc_array(obj, grid_col, grid_row);

    lv_obj_t *slider = lv_slider_create(obj);
    lv_obj_set_style_anim_duration(slider, 100, 0);
    lv_obj_add_flag(slider, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(slider, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_grid_cell(slider, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    lv_obj_set_height(slider, 200);
    lv_obj_set_width(slider, 12);

    lv_slider_set_range(slider, 0, 100);
    lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, 10, LV_PART_KNOB);
    lv_obj_set_style_bg_grad_dir(slider, LV_GRAD_DIR_VER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x569af8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(slider, lv_color_hex(0xa666f1), LV_PART_INDICATOR);
    lv_obj_set_style_outline_width(slider, 0, 0);

    lv_obj_add_event_cb(slider, volume, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(slider, slider_cb, LV_EVENT_ALL, data);

    LV_IMAGE_DECLARE(lv_img_mute);
    lv_obj_t *icon = lv_image_create(obj);
    lv_image_set_src(icon, &lv_img_mute);
    lv_obj_set_style_margin_top(icon, 20, 0);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_add_event_cb(icon, mute, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(icon, slider_cb, LV_EVENT_ALL, data);

    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);

    data->timer = lv_timer_create(volume_tick, 500, data);

    data->mute = icon;
    data->display_down = 0;
    data->is_display = false;
    data->slider = slider;
    data->view = obj;

    lv_obj_set_user_data(obj, data);

    return obj;
}