#include "view_ble.h"
#include "view_wave.h"
#include "view_spectrum.h"
#include "view_volume.h"

#include "ui/view_setting.h"
#include "ui/font.h"
#include "ui/lang.h"
#include "ui/anim.h"

static lv_obj_t *state_obj;
static lv_obj_t *title_label;
static lv_obj_t *artist_label;
static lv_obj_t *genre_label;
static lv_obj_t *spectrum_obj;
static lv_obj_t *button_back_obj;
static lv_obj_t *time_all_obj;
static lv_obj_t *time_now_obj;
static lv_obj_t *volume_obj;
static lv_obj_t *play_obj;
static lv_obj_t *slider_obj;
static lv_obj_t *button_par_obj;

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
    lv_obj_set_size(cont, LV_SIZE_CONTENT, 130);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_refresh_ext_draw_size(cont);

    uint32_t wid = lv_obj_get_width(parent);

    title_label = lv_label_create(cont);
    lv_obj_set_style_text_font(title_label, font_32, 0);
    lv_obj_set_height(title_label, lv_font_get_line_height(font_32));
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x504d6d), 0);
    lv_obj_set_width(title_label, wid - LV_MUSIC_HANDLE_SIZE);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_style(title_label, &label_style, LV_STATE_DEFAULT);
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_MODE_SCROLL);
    lv_label_set_text(title_label, now_lang->music_text1);

    lv_obj_set_style_margin_top(title_label, 25, 0);

    artist_label = lv_label_create(cont);
    lv_obj_set_style_text_font(artist_label, font_22, 0);
    lv_obj_set_height(artist_label, lv_font_get_line_height(font_22));
    lv_obj_set_style_text_color(artist_label, lv_color_hex(0x504d6d), 0);
    lv_obj_set_width(artist_label, wid - LV_MUSIC_HANDLE_SIZE);
    lv_obj_set_style_text_align(artist_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(artist_label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_label_set_text(artist_label, now_lang->empty);

    genre_label = lv_label_create(cont);
    lv_obj_set_style_text_font(genre_label, font_22, 0);
    lv_obj_set_height(genre_label, lv_font_get_line_height(font_22));
    lv_obj_set_style_text_color(genre_label, lv_color_hex(0x8a86b8), 0);
    lv_obj_set_width(genre_label, wid - LV_MUSIC_HANDLE_SIZE);
    lv_obj_set_style_text_align(genre_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_style(genre_label, &label_style, LV_STATE_DEFAULT);
    lv_label_set_long_mode(genre_label, LV_LABEL_LONG_MODE_DOTS);
    // lv_label_set_long_mode(genre_label, LV_LABEL_LONG_MODE_SCROLL);
    lv_label_set_text(genre_label, now_lang->empty);

    return cont;
}

static lv_obj_t *create_ctrl_box(lv_obj_t *parent, lv_event_cb_t prev, lv_event_cb_t play, lv_event_cb_t next)
{
    uint32_t wid = lv_obj_get_width(parent);

    /*Create the control box*/
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, wid, 220);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    static const int32_t grid_col[] = {LV_GRID_FR(2), LV_GRID_FR(3), LV_GRID_FR(5), LV_GRID_FR(5), LV_GRID_FR(5), LV_GRID_FR(3), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
    static const int32_t grid_row[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(cont, grid_col, grid_row);

    LV_IMAGE_DECLARE(img_lv_next);
    LV_IMAGE_DECLARE(img_lv_last);
    LV_IMAGE_DECLARE(img_lv_demo_music_btn_play);
    LV_IMAGE_DECLARE(img_lv_demo_music_btn_pause);
    LV_IMAGE_DECLARE(img_lv_music_speaker);

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

static lv_obj_t *create_timer_box(lv_obj_t *parent)
{
    uint32_t wid = lv_obj_get_width(parent);

    /*Create the control box*/
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, wid, LV_SIZE_CONTENT);
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

    // lv_obj_add_event_cb(slider_obj, time, LV_EVENT_RELEASED, NULL);

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

    return cont;
}

lv_obj_t *lv_ble_create(lv_obj_t *parent, lv_event_cb_t prev,
                        lv_event_cb_t play, lv_event_cb_t next,
                        lv_event_cb_t par)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_set_size(obj, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(obj, 255, 0);

    lv_obj_update_layout(obj);

    lv_wave_images_create(obj, 0);

    uint32_t wid = lv_obj_get_width(parent);

    state_obj = lv_label_create(obj);
    lv_obj_align(state_obj, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_size(state_obj, wid - LV_MUSIC_HANDLE_SIZE, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(state_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(state_obj, font_22, 0);
    lv_ble_disconnect();

    lv_obj_t *title = create_title_box(obj);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 120);

    spectrum_obj = lv_spectrum_create(obj);
    lv_obj_align(spectrum_obj, LV_ALIGN_TOP_MID, 0, 380);

    lv_obj_t *ctrl = create_ctrl_box(obj, prev, play, next);
    lv_obj_align(ctrl, LV_ALIGN_DEFAULT, 0, 440);

    lv_obj_t *time_box = create_timer_box(obj);
    lv_obj_align(time_box, LV_ALIGN_DEFAULT, 0, 580);

    button_par_obj = lv_button_create(obj);
    lv_obj_align(button_par_obj, LV_ALIGN_TOP_MID, 0, 660);
    lv_obj_add_event_cb(button_par_obj, par, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label = lv_label_create(button_par_obj);
    lv_label_set_text(label, now_lang->ble_text1);
    lv_obj_set_align(label, LV_ALIGN_CENTER);

    return obj;
}

void lv_ble_set_title(const char *data)
{
    lv_label_set_text(title_label, data);
}

void lv_ble_set_artlist(const char *data)
{
    lv_label_set_text(artist_label, data);
}

void lv_ble_set_album(const char *data)
{
    lv_label_set_text(genre_label, data);
}

void lv_ble_par_display(bool display)
{
    if (display)
    {
        lv_obj_remove_flag(button_par_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(button_par_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void lv_ble_set_all_time(float time)
{
    if (!lv_slider_is_dragged(slider_obj))
    {
        lv_slider_set_range(slider_obj, 0, (uint32_t)time * 1000);
    }
    lv_label_set_text_fmt(time_all_obj, "%" LV_PRIu32 ":%02" LV_PRIu32, (uint32_t)(time / 60), ((uint32_t)time % 60));
}

void lv_ble_set_now_time(float time)
{
    if (!lv_slider_is_dragged(slider_obj))
    {
        lv_slider_set_value(slider_obj, (uint32_t)time * 1000, LV_ANIM_ON);
    }
    lv_label_set_text_fmt(time_now_obj, "%" LV_PRIu32 ":%02" LV_PRIu32, (uint32_t)(time / 60), ((uint32_t)time % 60));
}

void lv_ble_set_play()
{
    lv_obj_add_state(play_obj, LV_STATE_CHECKED);
}

void lv_ble_set_pause()
{
    lv_obj_invalidate(spectrum_obj);
    lv_obj_remove_state(play_obj, LV_STATE_CHECKED);
}

void lv_ble_connect(const char *text)
{
    lv_label_set_text_fmt(state_obj, now_lang->ble_text2, text);
}

void lv_ble_disconnect()
{
    lv_label_set_text(state_obj, now_lang->ble_text3);
}

void lv_ble_fft_load()
{
    lv_obj_invalidate(spectrum_obj);
}

void lv_ble_set_fft_data(uint16_t index, uint16_t value)
{
    lv_spectrum_set_value(spectrum_obj, index, value);
}