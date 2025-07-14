#include "view_music_main.h"
#include "view_lyric.h"

#include "../view_state.h"
#include "../font.h"
#include "../anim.h"
#include "../common/utils.h"
#include "../player/player_info.h"

#include <malloc.h>

#define ACTIVE_TRACK_CNT 3
#define INTRO_TIME 0
#define BAR_COLOR1 lv_color_hex(0xe9dbfc)
#define BAR_COLOR2 lv_color_hex(0x6f8af6)
#define BAR_COLOR3 lv_color_hex(0xffffff)
#define BAR_COLOR1_STOP 5
#define BAR_COLOR2_STOP 20
#define BAR_REST_RADIUS 5
#define BAR_COLOR3_STOP (LV_MAX(LV_HOR_RES, LV_VER_RES) / 3)
#define BAR_CNT 20
#define BAR_MAX_VALUE 50
#define PIC_SIZE 300

static lv_obj_t *main_cont;
static lv_obj_t *spectrum_obj;
static lv_obj_t *title_label;
static lv_obj_t *artist_label;
static lv_obj_t *genre_label;
static lv_obj_t *time_all_obj;
static lv_obj_t *time_now_obj;
static lv_obj_t *album_image_obj;
static lv_obj_t *album_obj;
static lv_obj_t *slider_obj;
static lv_obj_t *sound_info_obj;
static lv_obj_t *volume_obj;
static lv_obj_t *play_obj;
static lv_obj_t *play_mode_obj;
static lv_obj_t *title_box;
static lv_obj_t *ctrl_box;
static lv_obj_t *handle_box;
static lv_obj_t *volume_slider_obj;
static lv_obj_t *volume_pan_obj;
static lv_obj_t *image_bg;
static lv_obj_t *image_bg1;

static bool playing;
static bool is_display_volume = false;
static uint32_t spectrum[20] = {0};
static lv_image_dsc_t img_dsc;

LV_IMAGE_DECLARE(img_lv_demo_music_btn_loop);
LV_IMAGE_DECLARE(img_lv_demo_music_btn_rnd);

static void spectrum_draw_event_cb(lv_event_t *e)
{
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

static void volume_display(bool display)
{
    if (display)
    {
        lv_obj_remove_flag(volume_pan_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(volume_pan_obj);

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)anim_opa_cb);
        lv_anim_set_values(&a, 0, 255);
        lv_anim_set_duration(&a, 200);
        lv_anim_set_var(&a, volume_pan_obj);
        lv_anim_start(&a);
    }
    else
    {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)anim_opa_cb);
        lv_anim_set_values(&a, 255, 0);
        lv_anim_set_duration(&a, 200);
        lv_anim_set_var(&a, volume_pan_obj);
        lv_anim_start(&a);
    }
}

static void speak_click_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    if (is_display_volume)
    {
        is_display_volume = false;
        volume_down = 0;
    }
    else
    {
        is_display_volume = true;
        volume_down = 8;
    }

    volume_display(is_display_volume);
}

static lv_obj_t *create_volume_slider(lv_obj_t *parent, lv_event_cb_t volume, lv_event_cb_t mute)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    static const int32_t grid_col[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    static const int32_t grid_row[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(obj, grid_col, grid_row);

    volume_slider_obj = lv_slider_create(obj);
    lv_obj_set_style_anim_duration(volume_slider_obj, 100, 0);
    lv_obj_add_flag(volume_slider_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(volume_slider_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_grid_cell(volume_slider_obj, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    lv_obj_set_height(volume_slider_obj, 200);
    lv_obj_set_width(volume_slider_obj, 12);

    lv_slider_set_range(volume_slider_obj, 0, 100);
    lv_obj_set_style_bg_opa(volume_slider_obj, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(volume_slider_obj, 10, LV_PART_KNOB);
    lv_obj_set_style_bg_grad_dir(volume_slider_obj, LV_GRAD_DIR_VER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(volume_slider_obj, lv_color_hex(0x569af8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(volume_slider_obj, lv_color_hex(0xa666f1), LV_PART_INDICATOR);
    lv_obj_set_style_outline_width(volume_slider_obj, 0, 0);

    lv_obj_add_event_cb(volume_slider_obj, volume, LV_EVENT_ALL, NULL);

    LV_IMAGE_DECLARE(lv_img_mute);
    lv_obj_t * icon = lv_image_create(obj);
    lv_image_set_src(icon, &lv_img_mute);
    lv_obj_set_style_margin_top(icon, 20, 0);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_add_event_cb(icon, mute, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);

    return obj;
}

static lv_obj_t *create_cont(lv_obj_t *parent)
{
    /*A transparent container in which the player section will be scrolled*/
    main_cont = lv_obj_create(parent);
    lv_obj_remove_flag(main_cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(main_cont, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_remove_style_all(main_cont); /*Make it transparent*/
    lv_obj_set_size(main_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_scroll_snap_y(main_cont, LV_SCROLL_SNAP_CENTER); /*Snap the children to the center*/

    /*Create a container for the player*/
    lv_obj_t *player = lv_obj_create(main_cont);
    lv_obj_set_y(player, -LV_DEMO_MUSIC_HANDLE_SIZE);
    lv_obj_set_size(player, LV_HOR_RES, LV_VER_RES + LV_DEMO_MUSIC_HANDLE_SIZE * 2);
    lv_obj_remove_flag(player, LV_OBJ_FLAG_SNAPPABLE);

    lv_obj_set_style_bg_color(player, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(player, 0, 0);
    lv_obj_set_style_pad_all(player, 0, 0);
    lv_obj_set_scroll_dir(player, LV_DIR_VER);

    /* Transparent placeholders below the player container
     * It is used only to snap it to center.*/
    lv_obj_t *placeholder1 = lv_obj_create(main_cont);
    lv_obj_remove_style_all(placeholder1);
    lv_obj_remove_flag(placeholder1, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *placeholder2 = lv_obj_create(main_cont);
    lv_obj_remove_style_all(placeholder2);
    lv_obj_remove_flag(placeholder2, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_size(placeholder1, lv_pct(100), LV_VER_RES);
    lv_obj_set_y(placeholder1, 0);

    lv_obj_set_size(placeholder2, lv_pct(100), LV_VER_RES - 2 * LV_DEMO_MUSIC_HANDLE_SIZE);
    lv_obj_set_y(placeholder2, LV_VER_RES + LV_DEMO_MUSIC_HANDLE_SIZE);

    lv_obj_update_layout(main_cont);

    return player;
}

static lv_obj_t *create_album_image_obj(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_image_create(parent);
    lv_obj_remove_style_all(obj);

    lv_obj_set_size(obj, PIC_SIZE, PIC_SIZE);

    lv_image_set_inner_align(obj, LV_IMAGE_ALIGN_STRETCH);
    lv_image_set_antialias(obj, true);

    return obj;
}

static void create_wave_images(lv_obj_t *parent)
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

static lv_obj_t *create_title_box(lv_obj_t *parent)
{
    static lv_anim_t animation_template;
    static lv_style_t label_style;

    lv_anim_init(&animation_template);
    lv_anim_set_delay(&animation_template, 3000);
    lv_anim_set_repeat_delay(&animation_template, 3000);
    lv_anim_set_reverse_delay(&animation_template, 3000);
    lv_anim_set_repeat_count(&animation_template, LV_ANIM_REPEAT_INFINITE);

    lv_style_init(&label_style);
    lv_style_set_anim(&label_style, &animation_template);

    /*Create the titles*/
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_height(cont, 130);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_refresh_ext_draw_size(cont);

    uint32_t wid = lv_obj_get_width(parent);

    title_label = lv_label_create(cont);
    lv_obj_set_style_text_font(title_label, font_32, 0);
    lv_obj_set_height(title_label, lv_font_get_line_height(font_32));
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x504d6d), 0);
    lv_obj_set_width(title_label, wid - LV_DEMO_MUSIC_HANDLE_SIZE);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_style(title_label, &label_style, LV_STATE_DEFAULT);
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_MODE_SCROLL);
    lv_label_set_text(title_label, "无音乐");

    lv_obj_set_style_margin_top(title_label, 15, 0);

    artist_label = lv_label_create(cont);
    lv_obj_set_style_text_font(artist_label, font_22, 0);
    lv_obj_set_height(artist_label, lv_font_get_line_height(font_22));
    lv_obj_set_style_text_color(artist_label, lv_color_hex(0x504d6d), 0);
    lv_obj_set_width(artist_label, wid - LV_DEMO_MUSIC_HANDLE_SIZE);
    lv_obj_set_style_text_align(artist_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(artist_label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_label_set_text(artist_label, "");

    genre_label = lv_label_create(cont);
    lv_obj_set_style_text_font(genre_label, font_22, 0);
    lv_obj_set_height(genre_label, lv_font_get_line_height(font_22));
    lv_obj_set_style_text_color(genre_label, lv_color_hex(0x8a86b8), 0);
    lv_obj_set_width(genre_label, wid - LV_DEMO_MUSIC_HANDLE_SIZE);
    lv_obj_set_style_text_align(genre_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_style(genre_label, &label_style, LV_STATE_DEFAULT);
    lv_label_set_long_mode(genre_label, LV_LABEL_LONG_MODE_DOTS);
    // lv_label_set_long_mode(genre_label, LV_LABEL_LONG_MODE_SCROLL);
    lv_label_set_text(genre_label, "");

    return cont;
}

static lv_obj_t *create_spectrum_obj(lv_obj_t *parent)
{
    /*Create the spectrum visualizer*/
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_height(obj, 20);

    uint32_t wid = lv_obj_get_width(parent);

    lv_obj_set_width(obj, wid - LV_DEMO_MUSIC_HANDLE_SIZE - LV_DEMO_MUSIC_HANDLE_SIZE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(obj, spectrum_draw_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_refresh_ext_draw_size(obj);

    return obj;
}

static lv_obj_t *create_timer_box(lv_obj_t *parent, lv_event_cb_t time)
{
    /*Create the control box*/
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_height(cont, LV_SIZE_CONTENT);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    static const int32_t grid_col[] = {LV_GRID_FR(2), LV_GRID_FR(3), LV_GRID_FR(15), LV_GRID_FR(3), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
    static const int32_t grid_row[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(cont, grid_col, grid_row);

    slider_obj = lv_slider_create(cont);
    lv_obj_set_style_anim_duration(slider_obj, 100, 0);
    lv_obj_add_flag(slider_obj, LV_OBJ_FLAG_CLICKABLE); /*No input from the slider*/
    lv_obj_remove_flag(slider_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    lv_obj_set_height(slider_obj, 6);
    lv_obj_set_grid_cell(slider_obj, LV_GRID_ALIGN_STRETCH, 1, 3, LV_GRID_ALIGN_CENTER, 1, 1);

    lv_obj_set_style_bg_opa(slider_obj, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider_obj, 10, LV_PART_KNOB);
    lv_obj_set_style_bg_grad_dir(slider_obj, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_obj, lv_color_hex(0x569af8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(slider_obj, lv_color_hex(0xa666f1), LV_PART_INDICATOR);
    lv_obj_set_style_outline_width(slider_obj, 0, 0);

    lv_obj_add_event_cb(slider_obj, time, LV_EVENT_RELEASED, NULL);

    time_all_obj = lv_label_create(cont);
    lv_obj_set_style_text_font(time_all_obj, font_22, 0);
    lv_obj_set_style_text_color(time_all_obj, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(time_all_obj, "/:/");
    lv_obj_set_grid_cell(time_all_obj, LV_GRID_ALIGN_END, 3, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    time_now_obj = lv_label_create(cont);
    lv_obj_set_style_text_font(time_now_obj, font_22, 0);
    lv_obj_set_style_text_color(time_now_obj, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(time_now_obj, "0:00");
    lv_obj_set_grid_cell(time_now_obj, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    sound_info_obj = lv_label_create(cont);
    lv_obj_set_style_text_font(sound_info_obj, font_16, 0);
    lv_obj_set_style_text_color(sound_info_obj, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(sound_info_obj, "N/A");
    lv_obj_set_grid_cell(sound_info_obj, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    return cont;
}

static lv_obj_t *create_ctrl_box(lv_obj_t *parent, lv_event_cb_t mode, lv_event_cb_t prev, lv_event_cb_t play, lv_event_cb_t next)
{
    /*Create the control box*/
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_height(cont, 220);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    static const int32_t grid_col[] = {LV_GRID_FR(2), LV_GRID_FR(3), LV_GRID_FR(5), LV_GRID_FR(5), LV_GRID_FR(5), LV_GRID_FR(3), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
    static const int32_t grid_row[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(cont, grid_col, grid_row);

    LV_IMAGE_DECLARE(img_lv_next);
    LV_IMAGE_DECLARE(img_lv_last);
    LV_IMAGE_DECLARE(img_lv_demo_music_btn_play);
    LV_IMAGE_DECLARE(img_lv_demo_music_btn_pause);
    LV_IMAGE_DECLARE(img_lv_music_speaker);

    play_mode_obj = lv_image_create(cont);
    lv_image_set_src(play_mode_obj, &img_lv_demo_music_btn_loop);
    lv_obj_set_grid_cell(play_mode_obj, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(play_mode_obj, mode, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(play_mode_obj, LV_OBJ_FLAG_CLICKABLE);

    volume_obj = lv_image_create(cont);
    lv_image_set_src(volume_obj, &img_lv_music_speaker);
    lv_obj_add_event_cb(volume_obj, speak_click_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(volume_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_grid_cell(volume_obj, LV_GRID_ALIGN_END, 5, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    lv_obj_t *icon;
    icon = lv_image_create(cont);
    lv_image_set_src(icon, &img_lv_last);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(icon, prev, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);

    play_obj = lv_imagebutton_create(cont);
    lv_imagebutton_set_src(play_obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_lv_demo_music_btn_play, NULL);
    lv_imagebutton_set_src(play_obj, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &img_lv_demo_music_btn_pause, NULL);
    lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_grid_cell(play_obj, LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    lv_obj_add_event_cb(play_obj, play, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(play_obj, img_lv_demo_music_btn_play.header.w);

    icon = lv_image_create(cont);
    lv_image_set_src(icon, &img_lv_next);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(icon, next, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);

    return cont;
}

static lv_obj_t *create_handle(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);

    lv_obj_set_size(cont, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(cont, 4, 0);

    /*A handle to scroll to the track list*/
    lv_obj_t *handle_label = lv_label_create(cont);
    lv_label_set_text(handle_label, "ALL TRACKS");
    lv_obj_set_style_text_font(handle_label, font_16, 0);
    lv_obj_set_style_text_color(handle_label, lv_color_hex(0x8a86b8), 0);

    lv_obj_t *handle_rect = lv_obj_create(cont);
    lv_obj_set_size(handle_rect, 40, 3);

    lv_obj_set_style_bg_color(handle_rect, lv_color_hex(0x8a86b8), 0);
    lv_obj_set_style_border_width(handle_rect, 0, 0);

    return cont;
}

lv_obj_t *view_music_main_create(lv_obj_t *parent, lv_event_cb_t time,
                                 lv_event_cb_t volume, lv_event_cb_t mode,
                                 lv_event_cb_t prev, lv_event_cb_t play, 
                                 lv_event_cb_t next, lv_event_cb_t mute)
{
    /*Create the content of the music player*/
    lv_obj_t *cont = create_cont(parent);

    create_wave_images(cont);
    title_box = create_title_box(cont);
    ctrl_box = create_ctrl_box(cont, mode, prev, play, next);
    spectrum_obj = create_spectrum_obj(cont);
    handle_box = create_handle(cont);
    lv_obj_t *time_box = create_timer_box(cont, time);

    image_bg = lv_obj_create(cont);
    lv_obj_remove_style_all(image_bg);
    lv_obj_set_size(image_bg, PIC_SIZE, PIC_SIZE);
    lv_obj_set_style_bg_color(image_bg, lv_color_hex(0x8199f7), 0);
    lv_obj_set_style_bg_opa(image_bg, 60, 0);
    lv_obj_set_style_bg_grad_color(image_bg, lv_color_hex(0x81f6e9), 0);
    lv_obj_set_style_bg_grad_dir(image_bg, LV_GRAD_DIR_HOR, 0);

    image_bg1 = lv_obj_create(cont);
    lv_obj_remove_style_all(image_bg1);
    lv_obj_set_size(image_bg1, PIC_SIZE, PIC_SIZE);
    lv_obj_set_style_bg_color(image_bg1, lv_color_hex(0x81d7f6), 0);
    lv_obj_set_style_bg_opa(image_bg1, 60, 0);
    lv_obj_set_style_bg_grad_color(image_bg1, lv_color_hex(0x81adf6), 0);
    lv_obj_set_style_bg_grad_dir(image_bg1, LV_GRAD_DIR_HOR, 0);

    album_image_obj = create_album_image_obj(cont);

    /*Arrange the content into a grid*/
    static const int32_t grid_cols[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static const int32_t grid_rows[] = {LV_DEMO_MUSIC_HANDLE_SIZE, /*Spacing*/
                                        LV_GRID_CONTENT,
                                        LV_GRID_CONTENT,
                                        LV_GRID_CONTENT,
                                        LV_GRID_CONTENT,
                                        LV_GRID_CONTENT,
                                        LV_GRID_FR(1),
                                        LV_DEMO_MUSIC_HANDLE_SIZE, /*Spacing*/
                                        LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(cont, grid_cols, grid_rows);
    lv_obj_set_style_grid_row_align(cont, LV_GRID_ALIGN_SPACE_BETWEEN, 0);

    lv_obj_set_grid_cell(title_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_grid_cell(album_image_obj, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);
    lv_obj_set_grid_cell(image_bg, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);
    lv_obj_set_grid_cell(image_bg1, LV_GRID_ALIGN_END, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);

    lv_obj_set_style_pad_top(ctrl_box, 70, 0);
    lv_obj_set_style_margin_top(ctrl_box, 35, 0);

    lv_obj_set_grid_cell(ctrl_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 3, 1);
    lv_obj_set_grid_cell(time_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 5, 1);

    lv_obj_set_grid_cell(spectrum_obj, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_START, 4, 1);

    lv_obj_set_grid_cell(handle_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 6, 1);

    lv_obj_set_style_margin_top(image_bg, 40, 0);
    lv_obj_set_style_margin_left(image_bg, 60, 0);

    lv_obj_set_style_margin_bottom(image_bg1, 40, 0);
    lv_obj_set_style_margin_right(image_bg1, 60, 0);

    lv_obj_update_layout(main_cont);

    volume_pan_obj = create_volume_slider(cont, volume, mute);
    lv_obj_align_to(volume_pan_obj, image_bg, LV_ALIGN_OUT_RIGHT_MID, 70, 0);

    lv_obj_t *lyric = lv_lyric_create(cont);
    lv_obj_set_grid_cell(lyric, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 3, 1);
    lv_obj_set_style_margin_top(lyric, 10, 0);

    return main_cont;
}

void lv_music_volume_close()
{
    volume_down = 0;
    is_display_volume = false;
    volume_display(false);
}

void lv_music_set_all_time(float time)
{
    if (!lv_slider_is_dragged(slider_obj) && play_state == MUSIC_STATE_PLAY)
    {
        lv_slider_set_range(slider_obj, 0, (uint32_t)time * 1000);
    }
    lv_label_set_text_fmt(time_all_obj, "%" LV_PRIu32 ":%02" LV_PRIu32, (uint32_t)(time / 60), ((uint32_t)time % 60));
}

void lv_music_set_now_time(float time)
{
    if (!lv_slider_is_dragged(slider_obj) && play_state == MUSIC_STATE_PLAY)
    {
        lv_slider_set_value(slider_obj, (uint32_t)time * 1000, LV_ANIM_ON);
    }
    lv_label_set_text_fmt(time_now_obj, "%" LV_PRIu32 ":%02" LV_PRIu32, (uint32_t)(time / 60), ((uint32_t)time % 60));
}

void lv_music_set_title(const char *data)
{
    lv_label_set_text(title_label, data);
}

void lv_music_set_album(const char *data)
{
    lv_label_set_text(genre_label, data);
}

void lv_music_set_auther(const char *data)
{
    lv_label_set_text(artist_label, data);
}

void lv_music_set_image(uint8_t *data, uint32_t size)
{
    if (img_dsc.data)
    {
        free((uint8_t *)img_dsc.data);
        img_dsc.data = NULL;
    }
    if (data == NULL)
    {
        lv_image_set_src(album_image_obj, NULL);
        return;
    }

    if (load_image(data, size, &img_dsc))
    {
        lv_image_set_src(album_image_obj, &img_dsc);
    }
    else
    {
        lv_image_set_src(album_image_obj, NULL);
    }
}

void lv_music_set_image_data(uint32_t width, uint32_t height, uint8_t *data)
{
    if (img_dsc.data)
    {
        free((uint8_t *)img_dsc.data);
    }
    img_dsc.header.w = width;
    img_dsc.header.h = height;
    img_dsc.data = data;
    img_dsc.data_size = width * height * 3;
    img_dsc.header.stride = width * 3;
    img_dsc.header.cf = LV_COLOR_FORMAT_RGB888;

    lv_image_set_src(album_image_obj, &img_dsc);
}

void lv_music_set_fft_data(uint16_t index, uint16_t value, uint32_t size)
{
    if (value > BAR_MAX_VALUE)
    {
        value = BAR_MAX_VALUE;
    }
    spectrum[index] = value;
}

void lv_music_fft_load()
{
    lv_obj_invalidate(spectrum_obj);
}

void lv_music_img_load()
{
    lv_obj_invalidate(album_image_obj);
}

void lv_music_set_sound_info(uint16_t bit, uint32_t rate, uint8_t channel, uint32_t bps)
{
    float temp = (float)rate / 1000;

    if (play_music_type == MUSIC_TYPE_MP3)
    {
        lv_label_set_text_fmt(sound_info_obj, "mp3 %dK %dch %02.1fK %dbit", bps / 1000, channel, temp, bit);
    }
    else if (play_music_type == MUSIC_TYPE_FLAC)
    {
        lv_label_set_text_fmt(sound_info_obj, "flac %dch %02.1fK %dbit", channel, temp, bit);
    }
}

void lv_music_fft_clear()
{
    for (int i = 0; i < BAR_CNT; i++)
    {
        spectrum[i] = 0;
    }
}

void lv_music_set_play_mode()
{
    if (play_music_mode == MUSIC_MODE_LOOP)
    {
        lv_image_set_src(play_mode_obj, &img_lv_demo_music_btn_loop);
    }
    else if (play_music_mode == MUSIC_MODE_RND)
    {
        lv_image_set_src(play_mode_obj, &img_lv_demo_music_btn_rnd);
    }
}

void lv_music_set_play()
{
    playing = true;
    lv_obj_add_state(play_obj, LV_STATE_CHECKED);
}

void lv_music_set_pause()
{
    playing = false;
    lv_obj_invalidate(spectrum_obj);
    lv_obj_remove_state(play_obj, LV_STATE_CHECKED);
}

void lv_music_set_volume(float value)
{
    lv_slider_set_value(volume_slider_obj, (int32_t)value, LV_ANIM_OFF);
}

void lv_music_fadein()
{
    lv_obj_fade_in(title_box, 1000, INTRO_TIME);
    lv_obj_fade_in(ctrl_box, 1000, INTRO_TIME);
    lv_obj_fade_in(handle_box, 1000, INTRO_TIME);
    lv_obj_fade_in(image_bg, 1000, INTRO_TIME);
    lv_obj_fade_in(album_image_obj, 1000, INTRO_TIME);
    lv_obj_fade_in(spectrum_obj, 0, INTRO_TIME);
}