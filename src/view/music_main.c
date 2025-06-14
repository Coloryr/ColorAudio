/**
 * @file lv_demo_music_main.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "music_main.h"

#include "music_list.h"
#include "assets/spectrum_1.h"

#include "utils.h"
#include "player.h"

#include "lvgl/src/misc/lv_types.h"
#include "lvgl/src/draw/lv_image_decoder_private.h"

#include <malloc.h>
#include <string.h>
#include <turbojpeg.h>
#include <png.h>

/*********************
 *      DEFINES
 *********************/
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

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_obj_t *create_cont(lv_obj_t *parent);
static void create_wave_images(lv_obj_t *parent);
static lv_obj_t *create_title_box(lv_obj_t *parent);
static lv_obj_t *create_spectrum_obj(lv_obj_t *parent);
static lv_obj_t *create_ctrl_box(lv_obj_t *parent);
static lv_obj_t *create_handle(lv_obj_t *parent);
static lv_obj_t *create_album_image_obj(lv_obj_t *parent);
static lv_obj_t *create_album_obj(lv_obj_t *parent);

static void spectrum_draw_event_cb(lv_event_t *e);
static void album_gesture_event_cb(lv_event_t *e);
static void play_event_click_cb(lv_event_t *e);
static void prev_click_event_cb(lv_event_t *e);
static void next_click_event_cb(lv_event_t *e);
static void track_load(uint32_t id);
static void spectrum_end_cb(lv_anim_t *a);
static void album_fade_anim_cb(void *var, int32_t v);
static int32_t get_cos(int32_t deg, int32_t a);
static int32_t get_sin(int32_t deg, int32_t a);

/**********************
 *  STATIC VARIABLES
 **********************/
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
static uint32_t bar_ofs = 0;
static uint32_t bar_rot = 0;
static uint32_t track_id;
static bool playing;
static lv_obj_t *play_obj;
static uint32_t spectrum[20] = {0};
static lv_image_dsc_t img_dsc;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/*
 * Callback adapter function to convert parameter types to avoid compile-time
 * warning.
 */
static void _image_set_scale_anim_cb(void *obj, int32_t scale)
{
    lv_image_set_scale((lv_obj_t *)obj, (uint16_t)scale);
}

/*
 * Callback adapter function to convert parameter types to avoid compile-time
 * warning.
 */
static void _obj_set_x_anim_cb(void *obj, int32_t x)
{
    lv_obj_set_x((lv_obj_t *)obj, (int32_t)x);
}

lv_obj_t *lv_demo_music_main_create(lv_obj_t *parent)
{
    /*Create the content of the music player*/
    lv_obj_t *cont = create_cont(parent);

    create_wave_images(cont);
    lv_obj_t *title_box = create_title_box(cont);
    lv_obj_t *ctrl_box = create_ctrl_box(cont);
    spectrum_obj = create_spectrum_obj(cont);
    lv_obj_set_style_margin_top(spectrum_obj, 100, 0);
    lv_obj_t *handle_box = create_handle(cont);

    // album_obj = create_album_obj(cont);
    album_image_obj = create_album_image_obj(cont);
    lv_obj_set_style_pad_bottom(album_image_obj, 80, 0);

    /*Arrange the content into a grid*/
    static const int32_t grid_cols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static const int32_t grid_rows[] = {LV_DEMO_MUSIC_HANDLE_SIZE, /*Spacing*/
                                        LV_GRID_FR(1),             /*Spacer*/
                                        LV_GRID_CONTENT,           /*Title box*/
                                        LV_GRID_FR(1),             /*Spacer*/
                                        LV_GRID_CONTENT,           /*Handle box*/
                                        LV_GRID_FR(1),             /*Spacer*/
                                        LV_DEMO_MUSIC_HANDLE_SIZE, /*Spacing*/
                                        LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(cont, grid_cols, grid_rows);
    lv_obj_set_style_grid_row_align(cont, LV_GRID_ALIGN_SPACE_BETWEEN, 0);
    lv_obj_set_grid_cell(title_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);
    lv_obj_set_grid_cell(ctrl_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_END, 4, 1);
    lv_obj_set_grid_cell(handle_box, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 5, 1);
    lv_obj_set_grid_cell(album_image_obj, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 5);
    lv_obj_set_grid_cell(spectrum_obj, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_END, 4, 1);

    lv_obj_fade_in(title_box, 1000, INTRO_TIME);
    lv_obj_fade_in(ctrl_box, 1000, INTRO_TIME);
    lv_obj_fade_in(handle_box, 1000, INTRO_TIME);
    lv_obj_fade_in(album_image_obj, 800, INTRO_TIME);
    lv_obj_fade_in(spectrum_obj, 0, INTRO_TIME);

    lv_obj_update_layout(main_cont);

    return main_cont;
}

void lv_music_album_next(bool next)
{
    if (next)
    {
        play_next();
    }
    else
    {
        play_last();
    }
}

void lv_demo_music_play(uint32_t id)
{
    track_load(id);

    lv_music_resume();
}

void lv_music_set_resume()
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

void lv_music_resume(void)
{
    if (play_resume())
    {
        lv_music_set_resume();
    }
}

void lv_music_pause(void)
{
    if (play_pause())
    {
        lv_music_set_pause();
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

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

    lv_obj_set_size(obj, 280, 280);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);

    lv_image_set_inner_align(obj, LV_IMAGE_ALIGN_STRETCH);

    return obj;
}

static lv_obj_t *create_album_obj(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);

    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(obj, 250, 250);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(obj, true, 0);

    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(obj, album_gesture_event_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);

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
    /*Create the titles*/
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_height(cont, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_refresh_ext_draw_size(cont);

    uint32_t wid = lv_obj_get_width(parent) / 2;

    title_label = lv_label_create(cont);
    lv_obj_set_style_text_font(title_label, font_32, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x504d6d), 0);
    lv_label_set_text(title_label, lv_demo_music_get_title(track_id));
    lv_obj_set_width(title_label, wid - 40);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_set_height(title_label, lv_font_get_line_height(font_32) * 3 / 2);

    lv_obj_set_style_margin_top(title_label, 10, 0);

    artist_label = lv_label_create(cont);
    lv_obj_set_style_text_font(artist_label, font_22, 0);
    lv_obj_set_style_text_color(artist_label, lv_color_hex(0x504d6d), 0);
    lv_label_set_text(artist_label, lv_demo_music_get_artist(track_id));
    lv_obj_set_width(artist_label, wid - 40);
    lv_obj_set_style_text_align(artist_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(artist_label, LV_LABEL_LONG_MODE_WRAP);

    genre_label = lv_label_create(cont);
    lv_obj_set_style_text_font(genre_label, font_22, 0);
    lv_obj_set_style_text_color(genre_label, lv_color_hex(0x8a86b8), 0);
    lv_obj_set_width(genre_label, wid - 40);
    lv_obj_set_style_text_align(genre_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(genre_label, LV_LABEL_LONG_MODE_WRAP);
    lv_label_set_text(genre_label, lv_demo_music_get_genre(track_id));

    return cont;
}

static lv_obj_t *create_spectrum_obj(lv_obj_t *parent)
{
    /*Create the spectrum visualizer*/
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_height(obj, 160);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(obj, spectrum_draw_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_refresh_ext_draw_size(obj);

    return obj;
}

static lv_obj_t *create_ctrl_box(lv_obj_t *parent)
{
    /*Create the control box*/
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_height(cont, LV_SIZE_CONTENT);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_style_pad_bottom(cont, 17, 0);
    static const int32_t grid_col[] = {LV_GRID_FR(2), LV_GRID_FR(3), LV_GRID_FR(5), LV_GRID_FR(5), LV_GRID_FR(5), LV_GRID_FR(3), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
    static const int32_t grid_row[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(cont, grid_col, grid_row);

    LV_IMAGE_DECLARE(img_lv_demo_music_btn_loop);
    LV_IMAGE_DECLARE(img_lv_demo_music_btn_rnd);
    LV_IMAGE_DECLARE(img_lv_demo_music_btn_next);
    LV_IMAGE_DECLARE(img_lv_demo_music_btn_prev);
    LV_IMAGE_DECLARE(img_lv_demo_music_btn_play);
    LV_IMAGE_DECLARE(img_lv_demo_music_btn_pause);

    lv_obj_t *icon;
    icon = lv_image_create(cont);
    lv_image_set_src(icon, &img_lv_demo_music_btn_rnd);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    icon = lv_image_create(cont);
    lv_image_set_src(icon, &img_lv_demo_music_btn_loop);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_END, 5, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    icon = lv_image_create(cont);
    lv_image_set_src(icon, &img_lv_demo_music_btn_prev);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(icon, prev_click_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);

    play_obj = lv_imagebutton_create(cont);
    lv_imagebutton_set_src(play_obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_lv_demo_music_btn_play, NULL);
    lv_imagebutton_set_src(play_obj, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &img_lv_demo_music_btn_pause, NULL);
    lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_grid_cell(play_obj, LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    lv_obj_add_event_cb(play_obj, play_event_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(play_obj, img_lv_demo_music_btn_play.header.w);

    icon = lv_image_create(cont);
    lv_image_set_src(icon, &img_lv_demo_music_btn_next);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(icon, next_click_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);

    slider_obj = lv_slider_create(cont);
    lv_obj_set_style_anim_duration(slider_obj, 100, 0);
    lv_obj_add_flag(slider_obj, LV_OBJ_FLAG_CLICKABLE); /*No input from the slider*/
    lv_obj_remove_flag(slider_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    lv_obj_set_height(slider_obj, 6);
    lv_obj_set_grid_cell(slider_obj, LV_GRID_ALIGN_STRETCH, 1, 5, LV_GRID_ALIGN_CENTER, 2, 1);

    lv_obj_set_style_bg_opa(slider_obj, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider_obj, 10, LV_PART_KNOB);
    lv_obj_set_style_bg_grad_dir(slider_obj, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_obj, lv_color_hex(0x569af8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(slider_obj, lv_color_hex(0xa666f1), LV_PART_INDICATOR);
    lv_obj_set_style_outline_width(slider_obj, 0, 0);

    time_all_obj = lv_label_create(cont);
    lv_obj_set_style_text_font(time_all_obj, font_22, 0);
    lv_obj_set_style_text_color(time_all_obj, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(time_all_obj, "/:/");
    lv_obj_set_grid_cell(time_all_obj, LV_GRID_ALIGN_END, 5, 1, LV_GRID_ALIGN_CENTER, 1, 1);

    time_now_obj = lv_label_create(cont);
    lv_obj_set_style_text_font(time_now_obj, font_22, 0);
    lv_obj_set_style_text_color(time_now_obj, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(time_now_obj, "0:00");
    lv_obj_set_grid_cell(time_now_obj, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);

    return cont;
}

static lv_obj_t *create_handle(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);

    lv_obj_set_size(cont, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(cont, 8, 0);

    /*A handle to scroll to the track list*/
    lv_obj_t *handle_label = lv_label_create(cont);
    lv_label_set_text(handle_label, "ALL TRACKS");
    lv_obj_set_style_text_font(handle_label, font_22, 0);
    lv_obj_set_style_text_color(handle_label, lv_color_hex(0x8a86b8), 0);

    lv_obj_t *handle_rect = lv_obj_create(cont);
    lv_obj_set_size(handle_rect, 40, 3);

    lv_obj_set_style_bg_color(handle_rect, lv_color_hex(0x8a86b8), 0);
    lv_obj_set_style_border_width(handle_rect, 0, 0);

    return cont;
}

static void track_load(uint32_t id)
{
    lv_slider_set_value(slider_obj, 0, LV_ANIM_OFF);
    lv_label_set_text(time_all_obj, "0:00");
    lv_label_set_text(time_now_obj, "0:00");

    if (id == track_id)
        return;
    bool next = false;
    if ((track_id + 1) % ACTIVE_TRACK_CNT == id)
        next = true;

    lv_demo_music_list_button_check(track_id, false);

    track_id = id;

    lv_demo_music_list_button_check(id, true);

    lv_label_set_text(title_label, "No music");
    lv_label_set_text(artist_label, "");
    lv_label_set_text(genre_label, "");

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, album_image_obj);
    lv_anim_set_values(&a, lv_obj_get_style_image_opa(album_image_obj, 0), LV_OPA_TRANSP);
    lv_anim_set_exec_cb(&a, album_fade_anim_cb);
    lv_anim_set_duration(&a, 500);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_var(&a, album_image_obj);
    lv_anim_set_duration(&a, 500);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);

    if (next)
    {
        lv_anim_set_values(&a, 0, -LV_HOR_RES / 7);
    }
    else
    {
        lv_anim_set_values(&a, 0, LV_HOR_RES / 7);
    }

    lv_anim_set_exec_cb(&a, _obj_set_x_anim_cb);
    lv_anim_set_completed_cb(&a, lv_obj_delete_anim_completed_cb);
    lv_anim_start(&a);

    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_var(&a, album_image_obj);
    lv_anim_set_duration(&a, 500);
    lv_anim_set_values(&a, LV_SCALE_NONE, LV_SCALE_NONE / 2);
    lv_anim_set_exec_cb(&a, _image_set_scale_anim_cb);
    lv_anim_set_completed_cb(&a, NULL);
    lv_anim_start(&a);

    lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
    lv_anim_set_var(&a, album_image_obj);
    lv_anim_set_duration(&a, 500);
    lv_anim_set_delay(&a, 100);
    lv_anim_set_values(&a, LV_SCALE_NONE / 4, LV_SCALE_NONE);
    lv_anim_set_exec_cb(&a, _image_set_scale_anim_cb);
    lv_anim_set_completed_cb(&a, NULL);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_var(&a, album_image_obj);
    lv_anim_set_values(&a, 0, LV_OPA_COVER);
    lv_anim_set_exec_cb(&a, album_fade_anim_cb);
    lv_anim_set_duration(&a, 500);
    lv_anim_set_delay(&a, 100);
    lv_anim_start(&a);
}

int32_t get_cos(int32_t deg, int32_t a)
{
    int32_t r = (lv_trigo_cos(deg) * a);

    r += LV_TRIGO_SIN_MAX / 2;
    return r >> LV_TRIGO_SHIFT;
}

int32_t get_sin(int32_t deg, int32_t a)
{
    int32_t r = lv_trigo_sin(deg) * a;

    return (r + LV_TRIGO_SIN_MAX / 2) >> LV_TRIGO_SHIFT;
}

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

            lv_area_t area = {.x1 = center.x + count, .x2 = center.x + temp + count, .y1 = center.y + height - v - 20, .y2 = center.y + height - 20};
            lv_draw_rect(layer, &draw_dsc, &area);
            count += pad + temp;
        }
    }
    else if (code == LV_EVENT_DELETE)
    {
    }
}

static void album_gesture_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (dir == LV_DIR_LEFT)
        lv_music_album_next(true);
    if (dir == LV_DIR_RIGHT)
        lv_music_album_next(false);
}

static void play_event_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    if (lv_obj_has_state(obj, LV_STATE_CHECKED))
    {
        lv_music_resume();
    }
    else
    {
        lv_music_pause();
    }
}

static void prev_click_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    lv_music_album_next(false);
}

static void next_click_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        lv_music_album_next(true);
    }
}

static void spectrum_end_cb(lv_anim_t *a)
{
    LV_UNUSED(a);
    lv_music_album_next(true);
}

static void album_fade_anim_cb(void *var, int32_t v)
{
    lv_obj_set_style_image_opa(var, v, 0);
}

void view_set_all_time(float time)
{
    lv_slider_set_range(slider_obj, 0, (uint32_t)time);
    lv_label_set_text_fmt(time_all_obj, "%" LV_PRIu32 ":%02" LV_PRIu32, (uint32_t)(time / 60), ((uint32_t)time % 60));
}
void view_set_now_time(float time)
{
    lv_slider_set_value(slider_obj, (uint32_t)time, LV_ANIM_ON);
    lv_label_set_text_fmt(time_now_obj, "%" LV_PRIu32 ":%02" LV_PRIu32, (uint32_t)(time / 60), ((uint32_t)time % 60));
}

void view_set_title(uint8_t *data)
{
    lv_label_set_text(title_label, data);
}
void view_set_album(uint8_t *data)
{
    lv_label_set_text(genre_label, data);
}
void view_set_auther(uint8_t *data)
{
    lv_label_set_text(artist_label, data);
}

const uint8_t jpg_signature[] = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46};
const uint8_t png_signature[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

static int is_jpg(uint8_t *raw_data, size_t len)
{
    if (len < sizeof(jpg_signature))
        return false;
    return memcmp(jpg_signature, raw_data, sizeof(jpg_signature)) == 0;
}

static int is_png(uint8_t *raw_data, size_t len)
{
    if (len < sizeof(png_signature))
        return false;
    return memcmp(png_signature, raw_data, sizeof(png_signature)) == 0;
}

static void istream_png_reader(png_structp png_ptr, png_bytep png_data, png_size_t data_size)
{
    stream_mem *st = (stream_mem *)png_get_io_ptr(png_ptr);
    if (st->pos + data_size > st->size)
    {
        return;
    }
    memcpy(png_data, st->data + st->pos, data_size);
    st->pos += data_size;
};

static size_t read_cb(void *out, size_t size, size_t nmemb, void *userp)
{
}

void view_set_image(uint8_t *data, uint32_t size)
{
    if (img_dsc.data)
    {
        free((uint8_t *)img_dsc.data);
    }
    if (data == NULL)
    {
        lv_image_set_src(album_image_obj, NULL);
        return;
    }

    if (is_jpg(data, size))
    {
        tjhandle handle = tjInitDecompress();
        if (!handle)
            return;

        int width = 0, height = 0;
        int res = tjDecompressHeader(handle, data, size, &width, &height);
        if (res != 0)
        {
            tjDestroy(handle);
            return;
        }

        img_dsc.header.w = width;
        img_dsc.header.h = height;
        img_dsc.data_size = width * height * 3;
        img_dsc.header.stride = width * 3;
        img_dsc.header.cf = LV_COLOR_FORMAT_RGB888;
        img_dsc.data = (uint8_t *)malloc(img_dsc.data_size); // RGB 24bpp
        if (!img_dsc.data)
        {
            tjDestroy(handle);
            return;
        }

        if (tjDecompress2(handle, data, size, (uint8_t *)img_dsc.data, width, 0, height, TJPF_BGR, 0) != 0)
        {
            tjDestroy(handle);
            return;
        }

        tjDestroy(handle);
    }
    else if (png_sig_cmp(data, 0, 8) == 0)
    {
        png_structp png_ptr = png_create_read_struct(
            PNG_LIBPNG_VER_STRING,
            NULL,
            NULL,
            NULL);
        if (!png_ptr)
        {
            return;
        }
        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
        {
            png_destroy_read_struct(&png_ptr, NULL, NULL); // 释放已经分配的资源
            return;
        }

        if (setjmp(png_jmpbuf(png_ptr)))
        {
            LV_LOG_ERROR("Png decode error");
        }

        stream_mem st = {size, 0, STREAM_TYPE_MEM, data};
        png_set_read_fn(png_ptr, &st, istream_png_reader);

        png_read_info(png_ptr, info_ptr);
        png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
        png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
        png_byte color_type = png_get_color_type(png_ptr, info_ptr);
        png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        if (color_type == PNG_COLOR_TYPE_PALETTE)
            png_set_palette_to_rgb(png_ptr); // 调色板转RGB
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
            png_set_expand_gray_1_2_4_to_8(png_ptr); // 灰度位扩展
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
            png_set_tRNS_to_alpha(png_ptr); // 透明度通道支持
        if (bit_depth == 16)
            png_set_strip_16(png_ptr); // 16位->8位
        if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(png_ptr); // 灰度转RGB
        if (!(color_type & PNG_COLOR_MASK_ALPHA))
            png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER); // 添加不透明Alpha通道
        // png_set_swap_alpha(png_ptr);                            // RGBA -> ARGB
        png_set_bgr(png_ptr);

        png_read_update_info(png_ptr, info_ptr); // 更新格式信息

        img_dsc.header.w = width;
        img_dsc.header.h = height;
        img_dsc.data_size = width * height * 4;
        img_dsc.header.stride = width * 4;
        img_dsc.header.cf = LV_COLOR_FORMAT_ARGB8888;
        img_dsc.data = (png_bytep)malloc(img_dsc.data_size);
        if (!img_dsc.data)
        {
            LV_LOG_ERROR("Png decode error");
        }

        png_bytepp row_pointers = malloc(height * sizeof(png_bytep));
        for (png_uint_32 y = 0; y < height; y++)
        {
            row_pointers[y] = (uint8_t *)img_dsc.data + y * width * 4;
        }

        png_read_image(png_ptr, row_pointers);
        png_read_end(png_ptr, NULL);

        free(row_pointers);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    }
    else
    {
        lv_image_set_src(album_image_obj, NULL);
        return;
    }

    lv_image_set_src(album_image_obj, &img_dsc);
    lv_image_set_antialias(album_image_obj, true);
}

void view_set_fft_data(uint16_t index, uint16_t value, uint32_t size)
{
    if (value > 50)
    {
        value = 50;
    }
    spectrum[index] = value;
}

void view_fft_load()
{
    lv_obj_invalidate(spectrum_obj);
}