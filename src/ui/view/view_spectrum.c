#include "view_spectrum.h"

#include "../view_setting.h"

#include "lvgl.h"

#define BAR_COLOR1 lv_color_hex(0xe9dbfc)
#define BAR_COLOR2 lv_color_hex(0x6f8af6)
#define BAR_COLOR3 lv_color_hex(0xffffff)
#define BAR_COLOR1_STOP 5
#define BAR_COLOR2_STOP 20
#define BAR_REST_RADIUS 5
#define BAR_COLOR3_STOP (LV_MAX(LV_HOR_RES, LV_VER_RES) / 3)
#define BAR_CNT 20
#define BAR_MAX_VALUE 50

static void spectrum_draw_event_cb(lv_event_t *e)
{
    uint32_t *spectrum = lv_event_get_user_data(e);

    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_REFR_EXT_DRAW_SIZE)
    {
        lv_event_set_ext_draw_size(e, LV_HOR_RES);
    }
    else if (code == LV_EVENT_COVER_CHECK)
    {
        lv_event_set_cover_res(e, LV_COVER_RES_NOT_COVER);
    }
    else if (code == LV_EVENT_DRAW_MAIN_BEGIN)
    {
        lv_obj_t *obj = lv_event_get_target(e);
        lv_layer_t *layer = lv_event_get_layer(e);

        lv_opa_t opa = lv_obj_get_style_opa_recursive(obj, LV_PART_MAIN);
        if (opa <= LV_OPA_MIN)
            return;

        lv_draw_rect_dsc_t draw_dsc;
        lv_draw_rect_dsc_init(&draw_dsc);
        draw_dsc.bg_opa = LV_OPA_COVER;

        lv_point_t center;
        lv_area_t obj_coords;
        lv_obj_get_coords(obj, &obj_coords);
        center.x = obj_coords.x1;
        center.y = obj_coords.y1;

        uint32_t i;

        int32_t width = lv_obj_get_width(obj);
        int32_t height = lv_obj_get_height(obj);

        int32_t item = width / BAR_CNT;
        int32_t pad = 1;
        int32_t count = 0;
        for (i = 0; i < BAR_CNT; i++)
        {
            uint32_t v;
            v = BAR_REST_RADIUS + spectrum[i];

            if (v < BAR_COLOR1_STOP)
                draw_dsc.bg_color = BAR_COLOR1;
            else if (v > (uint32_t)BAR_COLOR3_STOP)
                draw_dsc.bg_color = BAR_COLOR3;
            else if (v > BAR_COLOR2_STOP)
                draw_dsc.bg_color = lv_color_mix(BAR_COLOR3, BAR_COLOR2,
                                                 ((v - BAR_COLOR2_STOP) * 255) / (BAR_COLOR3_STOP - BAR_COLOR2_STOP));
            else
                draw_dsc.bg_color = lv_color_mix(BAR_COLOR2, BAR_COLOR1,
                                                 ((v - BAR_COLOR1_STOP) * 255) / (BAR_COLOR2_STOP - BAR_COLOR1_STOP));
            count += pad;
            int16_t temp = item - pad * 2;

            lv_area_t area = {.x1 = center.x + count,
                              .x2 = center.x + temp + count,
                              .y1 = center.y + height - v + 5,
                              .y2 = center.y + height + 5};
            lv_draw_rect(layer, &draw_dsc, &area);
            count += pad + temp;
        }
    }
    else if (code == LV_EVENT_DELETE)
    {
    }
}

lv_obj_t *lv_spectrum_create(lv_obj_t *parent)
{
    /*Create the spectrum visualizer*/
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_height(obj, 20);

    uint32_t wid = lv_obj_get_width(parent);

    uint32_t *temp = lv_malloc(sizeof(uint32_t) * BAR_CNT);

    lv_obj_set_width(obj, wid - LV_MUSIC_HANDLE_SIZE - LV_MUSIC_HANDLE_SIZE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(obj, spectrum_draw_event_cb, LV_EVENT_ALL, temp);
    lv_obj_refresh_ext_draw_size(obj);

    lv_obj_set_user_data(obj, temp);

    return obj;
}

void lv_spectrum_set_value(lv_obj_t *spectrum, uint16_t index, uint16_t value)
{
    uint32_t *temp = lv_obj_get_user_data(spectrum);

    if (value > BAR_MAX_VALUE)
    {
        value = BAR_MAX_VALUE;
    }
    temp[index] = value;
}

void lv_spectrum_clear_value(lv_obj_t *spectrum)
{
    uint32_t *temp = lv_obj_get_user_data(spectrum);
    for (int i = 0; i < BAR_CNT; i++)
    {
        temp[i] = 0;
    }
}