#include "view_lyric.h"

#include "../font.h"
#include "../../common/utils.h"

#include "lvgl.h"

#include <stdint.h>
#include <string.h>
#include <malloc.h>

static lv_obj_t *lyric_obj;
static lv_obj_t *lyric_tr_obj;
static lv_obj_t *pan;

static char temp_text[512];
static char temp_text1[512];
static char *text;

static char *text_lyric;
static char *text_lyric_k;
static char *text_lyric_k_now;
static char *text_lyric_tr;

static float text_lyric_kp;

static bool have_k;

#include "../lvgl/src/misc/lv_area_private.h"
#include "../lvgl/src/misc/lv_text_private.h"

typedef enum
{
    LYRIC_TEST1,
    LYRIC_TEST2,
    LYRIC_TEST3,
    LYRIC_TEST4,
    LYRIC_TEST5,
    LYRIC_TEST6,
    LYRIC_TEST7,
} view_lyric_test;

typedef struct
{
    uint32_t x;
    uint32_t y;
    uint32_t start;
    uint32_t len;
    lv_point_t size;
    bool have;
} view_lyric_item;

static void lv_lyric_render(lv_event_t *e)
{
    view_lyric_test test_time = LYRIC_TEST1;

    lv_obj_t *obj = lv_event_get_current_target(e);
    lv_layer_t *layer = lv_event_get_layer(e);

    lv_area_t txt_coords;
    lv_obj_get_content_coords(obj, &txt_coords);
    int32_t max_w = lv_area_get_width(&txt_coords);
    int32_t max_h = lv_area_get_height(&txt_coords);
    int32_t line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
    int32_t letter_space = lv_obj_get_style_text_letter_space(obj, LV_PART_MAIN);

    lv_area_t txt_clip;
    bool is_common = lv_area_intersect(&txt_clip, &txt_coords, &layer->_clip_area);
    if (!is_common)
    {
        return;
    }

    if (text_lyric == NULL)
    {
        return;
    }
    uint32_t len1 = strlen(text_lyric);
    uint32_t len2 = 0;
    uint32_t len3 = 0;
    if (text_lyric_k != NULL)
    {
        len2 = strlen(text_lyric_k);
    }
    if (text_lyric_k_now != NULL)
    {
        len3 = strlen(text_lyric_k_now);
    }
    if (text == NULL)
    {
        text = malloc(len1 + len2 + len3 + 1);
    }
    else
    {
        text = realloc(text, len1 + len2 + len3 + 1);
    }

    lv_point_t size_lyric;
    lv_point_t size_lyric_tr;
    if (text_lyric_k != NULL)
    {
        strcpy(text, text_lyric_k);
    }
    if (text_lyric_k_now != NULL)
    {
        strcpy(text + len2, text_lyric_k_now);
    }
    strcpy(text + len2 + len3, text_lyric);

    const lv_font_t *ly_font;
    const lv_font_t *ty_font;

test_next:
    if (test_time == LYRIC_TEST1)
    {
        ly_font = font_22;
        ty_font = font_18;
    }
    else if (test_time == LYRIC_TEST2)
    {
        ly_font = font_18;
        ty_font = font_18;
    }
    else if (test_time == LYRIC_TEST3)
    {
        ly_font = font_18;
        ty_font = font_16;
    }
    else if (test_time == LYRIC_TEST4)
    {
        ly_font = font_16;
        ty_font = font_16;
    }
    else if (test_time == LYRIC_TEST5)
    {
        ly_font = font_16;
        ty_font = font_12;
    }
    else if (test_time == LYRIC_TEST6)
    {
        ly_font = font_14;
        ty_font = font_14;
    }
    else if (test_time == LYRIC_TEST7)
    {
        ly_font = font_14;
        ty_font = font_12;
    }

    lv_text_get_size(&size_lyric_tr, text_lyric_tr, ty_font, letter_space, line_space, max_w, LV_TEXT_FLAG_NONE);
    view_lyric_item k_lyric_pos[2] = {0};

    {
        size_lyric.x = 0;
        size_lyric.y = 0;

        uint32_t line_start = 0;
        uint32_t new_line_start = 0;
        uint16_t letter_height = lv_font_get_line_height(ly_font);
        while (text[line_start] != '\0')
        {
            new_line_start += lv_text_get_next_line(&text[line_start], LV_TEXT_LEN_MAX, ly_font, letter_space, max_w, NULL, LV_TEXT_FLAG_NONE);

            if ((unsigned long)size_lyric.y + (unsigned long)letter_height + (unsigned long)line_space > LV_MAX_OF(int32_t))
            {
                LV_LOG_WARN("integer overflow while calculating text height");
                return;
            }
            else
            {
                size_lyric.y += letter_height;
                size_lyric.y += line_space;
            }

            int32_t act_line_length = lv_text_get_width_with_flags(&text[line_start], new_line_start - line_start, ly_font,
                                                                   letter_space, LV_TEXT_FLAG_NONE);

            size_lyric.x = LV_MAX(act_line_length, size_lyric.x);
            if (len3 > 0)
            {
                if (line_start <= len2 && (new_line_start >= len2 + len3 || new_line_start >= len2))
                {
                    uint32_t len = len2 - line_start;
                    k_lyric_pos[0].y = size_lyric.y - letter_height;
                    memcpy(temp_text, text + line_start, len);
                    temp_text[len] = 0;
                    int32_t left_width = lv_text_get_width_with_flags(temp_text, new_line_start - line_start, ly_font,
                                                                      letter_space, LV_TEXT_FLAG_NONE);
                    k_lyric_pos[0].x = ((max_w - act_line_length) / 2) + left_width;
                    k_lyric_pos[0].have = true;
                    k_lyric_pos[0].start = len2;
                    k_lyric_pos[0].len = min(len3, new_line_start - len2);

                    len = k_lyric_pos[0].len;
                    memcpy(temp_text, text + len2, len);
                    temp_text[len] = 0;

                    lv_text_get_size(&k_lyric_pos[0].size, temp_text, ly_font, letter_space, line_space, max_w, LV_TEXT_FLAG_NONE);
                }
                else if (line_start >= len2 + len3 && line_start <= len2 + len3 && new_line_start <= len2 + len3)
                {
                    k_lyric_pos[1].y = size_lyric.y - letter_height;
                    k_lyric_pos[1].start = line_start;
                    k_lyric_pos[1].len = len2 + len3 - line_start;
                    k_lyric_pos[1].x = (max_w - act_line_length) / 2;
                    k_lyric_pos[1].have = true;

                    uint32_t len = k_lyric_pos[1].len;
                    memcpy(temp_text, text + line_start, len);
                    temp_text[len] = 0;

                    lv_text_get_size(&k_lyric_pos[1].size, temp_text, ly_font, letter_space, line_space, max_w, LV_TEXT_FLAG_NONE);
                }
            }
            line_start = new_line_start;
        }

        /*Make the text one line taller if the last character is '\n' or '\r'*/
        if ((line_start != 0) && (text[line_start - 1] == '\n' || text[line_start - 1] == '\r'))
        {
            size_lyric.y += letter_height + line_space;
        }

        /*Correction with the last line space or set the height manually if the text is empty*/
        if (size_lyric.y == 0)
            size_lyric.y = letter_height;
        else
            size_lyric.y -= line_space;
    }

    if (test_time != LYRIC_TEST7 && size_lyric.y + size_lyric_tr.y > max_h)
    {
        test_time++;
        goto test_next;
    }

    uint32_t pos = (max_h - size_lyric.y - size_lyric_tr.y) / 2;

    int32_t s = lv_obj_get_scroll_top(obj);
    lv_area_move(&txt_coords, 0, -s);
    lv_area_t obj_coords;
    lv_obj_get_coords(obj, &obj_coords);
    txt_coords.y2 = obj_coords.y2;

    {
        lv_draw_label_dsc_t lyric_draw_dsc;
        lv_draw_label_dsc_init(&lyric_draw_dsc);
        lyric_draw_dsc.text = text;
        lyric_draw_dsc.ofs_y = pos;
        lyric_draw_dsc.text_size = size_lyric;
        lyric_draw_dsc.flag = LV_TEXT_FLAG_NONE;
        lyric_draw_dsc.base.layer = layer;

        if (text_lyric_k != NULL)
        {
            lyric_draw_dsc.sel_start = 0;
            lyric_draw_dsc.sel_end = utf8_strlen(text_lyric_k);
        }
        lv_obj_init_draw_label_dsc(obj, LV_PART_MAIN, &lyric_draw_dsc);
        lv_bidi_calculate_align(&lyric_draw_dsc.align, &lyric_draw_dsc.bidi_dir, text);

        lyric_draw_dsc.font = ly_font;
        lyric_draw_dsc.sel_color = lv_color_hex(0x5172f7);
        lyric_draw_dsc.sel_bg_color = lv_color_hex(0xffffff);
        lyric_draw_dsc.color = have_k ? lv_color_hex(0xaf93f6) : lv_color_hex(0x8199f7);

        lv_draw_label(layer, &lyric_draw_dsc, &txt_coords);
    }

    if (k_lyric_pos[0].have)
    {
        uint32_t len = k_lyric_pos[0].len;
        memcpy(temp_text, text + len2, len);
        temp_text[len] = 0;

        lv_draw_label_dsc_t klyric_draw_dsc;
        lv_draw_label_dsc_init(&klyric_draw_dsc);
        klyric_draw_dsc.text = temp_text;

        klyric_draw_dsc.align = LV_TEXT_ALIGN_LEFT;

        klyric_draw_dsc.font = ly_font;
        klyric_draw_dsc.color = lv_color_hex(0x5172f7);

        uint32_t temp = k_lyric_pos[0].size.x * text_lyric_kp;

        lv_area_t tex_cop = {0, 0, k_lyric_pos[0].size.x, k_lyric_pos[0].size.y};

        lv_layer_t *layer_in = lv_draw_layer_create(layer, LV_COLOR_FORMAT_ARGB8888, &tex_cop);

        lv_draw_fill_dsc_t fill_draw_dsc;
        lv_draw_fill_dsc_init(&fill_draw_dsc);
        fill_draw_dsc.color = lv_color_hex(0xffffff);
        lv_draw_fill(layer_in, &fill_draw_dsc, &tex_cop);
        lv_draw_label(layer_in, &klyric_draw_dsc, &tex_cop);

        lv_area_t cop = {.x1 = txt_coords.x1,
                         .x2 = txt_coords.x1 + temp,
                         .y1 = txt_coords.y1,
                         .y2 = txt_coords.y1 + k_lyric_pos[0].size.y};

        lv_area_move(&cop, k_lyric_pos[0].x, pos + k_lyric_pos[0].y);

        lv_draw_image_dsc_t image_draw_dsc;
        lv_draw_image_dsc_init(&image_draw_dsc);
        image_draw_dsc.src = layer_in; /* Source image is the new layer. */
        /* Draw new layer to parent layer. */
        lv_draw_layer(layer, &image_draw_dsc, &cop);
    }

    if (k_lyric_pos[1].have)
    {
        uint32_t len = k_lyric_pos[1].len;
        memcpy(temp_text1, text + len2, len);
        temp_text1[len] = 0;

        lv_draw_label_dsc_t klyric1_draw_dsc;
        lv_draw_label_dsc_init(&klyric1_draw_dsc);
        klyric1_draw_dsc.text = temp_text1;

        klyric1_draw_dsc.align = LV_TEXT_ALIGN_LEFT;

        klyric1_draw_dsc.font = ly_font;
        klyric1_draw_dsc.color = lv_color_hex(0x5172f7);

        uint32_t temp = k_lyric_pos[1].size.x * text_lyric_kp;

        lv_area_t tex_cop = {0, 0, k_lyric_pos[1].size.x, k_lyric_pos[1].size.y};

        lv_layer_t *layer_in = lv_draw_layer_create(layer, LV_COLOR_FORMAT_ARGB8888, &tex_cop);

        lv_draw_fill_dsc_t fill_draw_dsc_1;
        lv_draw_fill_dsc_init(&fill_draw_dsc_1);
        fill_draw_dsc_1.color = lv_color_hex(0xffffff);
        lv_draw_fill(layer_in, &fill_draw_dsc_1, &tex_cop);
        lv_draw_label(layer_in, &klyric1_draw_dsc, &tex_cop);

        lv_area_t cop = {.x1 = txt_coords.x1,
                         .x2 = txt_coords.x1 + temp,
                         .y1 = txt_coords.y1,
                         .y2 = txt_coords.y1 + k_lyric_pos[1].size.y};

        lv_area_move(&cop, k_lyric_pos[1].x, pos + k_lyric_pos[1].y);

        lv_draw_image_dsc_t image_draw_dsc_1;
        lv_draw_image_dsc_init(&image_draw_dsc_1);
        image_draw_dsc_1.src = layer_in; /* Source image is the new layer. */
        /* Draw new layer to parent layer. */
        lv_draw_layer(layer, &image_draw_dsc_1, &cop);
    }

    {
        lv_draw_label_dsc_t tlyric_draw_dsc;
        lv_draw_label_dsc_init(&tlyric_draw_dsc);
        tlyric_draw_dsc.text = text_lyric_tr;
        tlyric_draw_dsc.ofs_y = pos + size_lyric.y;
        tlyric_draw_dsc.text_size = size_lyric_tr;
        tlyric_draw_dsc.flag = LV_TEXT_FLAG_NONE;
        tlyric_draw_dsc.base.layer = layer;

        lv_obj_init_draw_label_dsc(obj, LV_PART_MAIN, &tlyric_draw_dsc);
        lv_bidi_calculate_align(&tlyric_draw_dsc.align, &tlyric_draw_dsc.bidi_dir, text_lyric_tr);

        tlyric_draw_dsc.font = ty_font;
        tlyric_draw_dsc.color = lv_color_hex(0x8199f7);

        lv_draw_label(layer, &tlyric_draw_dsc, &txt_coords);
    }

    lv_area_t clip_area_ori = layer->_clip_area;
    layer->_clip_area = txt_clip;

    layer->_clip_area = clip_area_ori;
}

lv_obj_t *lv_lyric_create(lv_obj_t *parent)
{
    uint32_t wid = lv_obj_get_width(parent);

    pan = lv_obj_create(parent);
    lv_obj_remove_style_all(pan);
    lv_obj_set_size(pan, wid, 115);
    // lv_obj_set_style_bg_color(pan, lv_color_hex(0xFF0000), 0);
    // lv_obj_set_style_bg_opa(pan, 50, 0);
    lv_obj_set_style_pad_left(pan, 20, 0);
    lv_obj_set_style_pad_right(pan, 20, 0);
    lv_obj_add_event_cb(pan, lv_lyric_render, LV_EVENT_DRAW_MAIN, NULL);
    lv_obj_set_style_text_align(pan, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(pan, lv_color_hex(0x8199f7), 0);

    return pan;
}

void lv_lyric_set_text(const char *text)
{
    const size_t text_len = strlen(text) + 1;

    if (text_lyric == NULL)
    {
        text_lyric = malloc(text_len);
    }
    else
    {
        text_lyric = realloc(text_lyric, text_len);
    }

    strcpy(text_lyric, text);
}

void lv_lyric_k_set_text(const char *text)
{
    const size_t text_len = strlen(text) + 1;

    if (text_lyric_k == NULL)
    {
        text_lyric_k = malloc(text_len);
    }
    else
    {
        text_lyric_k = realloc(text_lyric_k, text_len);
    }

    strcpy(text_lyric_k, text);
}

void lv_lyric_k_now_set_text(const char *text)
{
    const size_t text_len = strlen(text) + 1;

    if (text_lyric_k_now == NULL)
    {
        text_lyric_k_now = malloc(text_len);
    }
    else
    {
        text_lyric_k_now = realloc(text_lyric_k_now, text_len);
    }

    strcpy(text_lyric_k_now, text);
}

void lv_lyric_kp_set_text(float kp)
{
    text_lyric_kp = kp;
}

void lv_lyric_tr_set_text(const char *text)
{
    const size_t text_len = strlen(text) + 1;

    if (text_lyric_tr == NULL)
    {
        text_lyric_tr = malloc(text_len);
    }
    else
    {
        text_lyric_tr = realloc(text_lyric_tr, text_len);
    }

    strcpy(text_lyric_tr, text);
}

void lv_lyric_set_have_k(bool have)
{
    have_k = have;
}

void lv_lyric_draw()
{
    lv_obj_invalidate(pan);
}
