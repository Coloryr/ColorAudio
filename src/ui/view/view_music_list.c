#include "view_music_list.h"

#include "../view.h"
#include "../font.h"

#include <stdbool.h>
#include <malloc.h>

static lv_obj_t *list_obj;
static lv_obj_t *search_obj;
static lv_obj_t *search_text_obj;
static lv_obj_t *top_obj;
static lv_obj_t *clear_obj;
static lv_style_t style_scrollbar;
static lv_style_t style_btn;
static lv_style_t style_button_pr;
static lv_style_t style_button_chk;
static lv_style_t style_button_dis;
static lv_style_t style_title;
static lv_style_t style_artist;
static lv_style_t style_time;

LV_IMAGE_DECLARE(img_lv_demo_music_btn_list_play);
LV_IMAGE_DECLARE(img_lv_demo_music_btn_list_pause);
LV_IMAGE_DECLARE(img_lv_demo_music_list_border);

void view_music_list_search_display(bool display)
{
    if (display)
    {
        lv_obj_remove_flag(search_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(search_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void view_music_list_search_text(char *data)
{
    lv_label_set_text_fmt(search_text_obj, "搜索包含“%s”的结果", data);
}

static void list_delete_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_DELETE)
    {
        lv_style_reset(&style_scrollbar);
        lv_style_reset(&style_btn);
        lv_style_reset(&style_button_pr);
        lv_style_reset(&style_button_chk);
        lv_style_reset(&style_button_dis);
        lv_style_reset(&style_title);
        lv_style_reset(&style_artist);
        lv_style_reset(&style_time);
    }
    else if (code == LV_EVENT_SCROLL_END)
    {
        if (lv_obj_get_scroll_y(list_obj) > 100)
        {
            lv_obj_remove_flag(top_obj, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(top_obj, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void list_top_event_cb(lv_event_t *e)
{
    lv_obj_scroll_to_view(search_obj, LV_ANIM_ON);
}

void item_check(view_play_item_t *item, bool state)
{
    lv_obj_t *btn = item->view;
    lv_obj_t *icon = lv_obj_get_child(btn, 0);

    if (state)
    {
        lv_obj_add_state(btn, LV_STATE_CHECKED);
        lv_image_set_src(icon, &img_lv_demo_music_btn_list_pause);
        lv_obj_scroll_to_view(btn, LV_ANIM_ON);
    }
    else
    {
        lv_obj_remove_state(btn, LV_STATE_CHECKED);
        lv_image_set_src(icon, &img_lv_demo_music_btn_list_play);
    }
}

lv_obj_t *view_music_list_create(lv_obj_t *parent, lv_event_cb_t clear, lv_event_cb_t search)
{
    lv_style_init(&style_scrollbar);
    lv_style_set_width(&style_scrollbar, 4);
    lv_style_set_bg_opa(&style_scrollbar, LV_OPA_COVER);
    lv_style_set_bg_color(&style_scrollbar, lv_color_hex3(0xeee));
    lv_style_set_radius(&style_scrollbar, LV_RADIUS_CIRCLE);
    lv_style_set_pad_right(&style_scrollbar, 4);

    static const int32_t grid_cols[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    static const int32_t grid_rows[] = {35, 30, LV_GRID_TEMPLATE_LAST};

    lv_style_init(&style_btn);
    lv_style_set_bg_opa(&style_btn, LV_OPA_TRANSP);
    lv_style_set_grid_column_dsc_array(&style_btn, grid_cols);
    lv_style_set_grid_row_dsc_array(&style_btn, grid_rows);
    lv_style_set_grid_row_align(&style_btn, LV_GRID_ALIGN_CENTER);
    lv_style_set_layout(&style_btn, LV_LAYOUT_GRID);
    lv_style_set_pad_right(&style_btn, 30);

    lv_style_init(&style_button_pr);
    lv_style_set_bg_opa(&style_button_pr, LV_OPA_COVER);
    lv_style_set_bg_color(&style_button_pr, lv_color_hex(0x4c4965));

    lv_style_init(&style_button_chk);
    lv_style_set_bg_opa(&style_button_chk, LV_OPA_COVER);
    lv_style_set_bg_color(&style_button_chk, lv_color_hex(0x4c4965));

    lv_style_init(&style_button_dis);
    lv_style_set_text_opa(&style_button_dis, LV_OPA_40);
    lv_style_set_image_opa(&style_button_dis, LV_OPA_40);

    uint32_t wid = lv_obj_get_width(parent);

    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, font_22);
    lv_style_set_text_color(&style_title, lv_color_hex(0xffffff));

    lv_style_init(&style_artist);
    lv_style_set_text_font(&style_artist, font_16);
    lv_style_set_text_color(&style_artist, lv_color_hex(0xb1b0be));

    lv_style_init(&style_time);
    lv_style_set_text_font(&style_time, font_22);
    lv_style_set_text_color(&style_time, lv_color_hex(0xffffff));

    /*Create an empty transparent container*/
    list_obj = lv_obj_create(parent);
    lv_obj_add_event_cb(list_obj, list_delete_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_remove_style_all(list_obj);
    lv_obj_set_size(list_obj, LV_HOR_RES, LV_VER_RES - LV_DEMO_MUSIC_HANDLE_SIZE);
    lv_obj_set_y(list_obj, LV_DEMO_MUSIC_HANDLE_SIZE);
    lv_obj_add_style(list_obj, &style_scrollbar, LV_PART_SCROLLBAR);
    lv_obj_set_flex_flow(list_obj, LV_FLEX_FLOW_COLUMN);

    static const int32_t grid_cols_search[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    static const int32_t grid_rows_search[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

    search_obj = lv_obj_create(list_obj);
    lv_obj_remove_style_all(search_obj);
    lv_obj_set_size(search_obj, LV_HOR_RES, LV_SIZE_CONTENT);
    lv_obj_add_flag(search_obj, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *obj = lv_obj_create(search_obj);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_align(obj, LV_ALIGN_CENTER);
    lv_obj_set_style_grid_column_dsc_array(obj, grid_cols, 0);
    lv_obj_set_style_grid_row_dsc_array(obj, grid_rows, 0);
    lv_obj_set_style_grid_column_align(obj, LV_GRID_ALIGN_CENTER, 0);
    lv_obj_set_style_grid_row_align(obj, LV_GRID_ALIGN_CENTER, 0);
    lv_obj_set_style_layout(obj, LV_LAYOUT_GRID, 0);

    search_text_obj = lv_label_create(obj);
    lv_obj_remove_style_all(search_text_obj);
    lv_obj_set_size(search_text_obj, wid - LV_DEMO_MUSIC_HANDLE_SIZE, LV_SIZE_CONTENT);
    lv_obj_set_style_margin_all(search_text_obj, 10, 0);
    lv_obj_set_style_text_font(search_text_obj, font_22, 0);
    lv_obj_set_style_text_color(search_text_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(search_text_obj, 255, 0);
    lv_label_set_text(search_text_obj, "搜索包含的结果");
    lv_obj_set_style_text_align(search_text_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(search_text_obj, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_grid_cell(search_text_obj, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    clear_obj = lv_button_create(obj);
    lv_obj_remove_style_all(clear_obj);
    lv_obj_set_size(clear_obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_margin_bottom(clear_obj, 10, 0);
    lv_obj_set_style_bg_color(clear_obj, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(clear_obj, 255, 0);
    lv_obj_set_style_radius(clear_obj, 5, 0);
    lv_obj_set_style_pad_left(clear_obj, 5, 0);
    lv_obj_set_style_pad_right(clear_obj, 5, 0);
    lv_obj_set_style_border_color(clear_obj, lv_color_hex(0x4c4965), 0);
    lv_obj_set_style_border_opa(clear_obj, 255, 0);
    lv_obj_add_event_cb(clear_obj, clear, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label = lv_label_create(clear_obj);
    lv_label_set_text(label, "清除搜索结果");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_grid_cell(clear_obj, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);

    lv_obj_t *border = lv_image_create(search_obj);
    lv_image_set_src(border, &img_lv_demo_music_list_border);
    lv_image_set_inner_align(border, LV_IMAGE_ALIGN_TILE);
    lv_obj_set_width(border, lv_pct(120));
    lv_obj_align(border, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(border, LV_OBJ_FLAG_IGNORE_LAYOUT);

    LV_IMAGE_DECLARE(img_lv_list_search);
    LV_IMAGE_DECLARE(img_lv_top);

    lv_obj_t *button = lv_image_create(parent);
    lv_image_set_src(button, &img_lv_list_search);
    lv_obj_add_event_cb(button, search, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(button, LV_ALIGN_BOTTOM_RIGHT, -10, 0);

    top_obj = lv_image_create(parent);
    lv_image_set_src(top_obj, &img_lv_top);
    lv_obj_add_event_cb(top_obj, list_top_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(top_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(top_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(top_obj, LV_ALIGN_BOTTOM_RIGHT, -10, -80);

    return list_obj;
}

view_play_item_t *view_list_add_item(const char *title, const char *artist, uint32_t time, lv_event_cb_t click)
{
    view_play_item_t *view = (view_play_item_t *)calloc(1, sizeof(*view));

    lv_obj_t *btn = lv_obj_create(list_obj);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, lv_pct(100), 110);

    lv_obj_add_style(btn, &style_btn, 0);
    lv_obj_add_style(btn, &style_button_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn, &style_button_chk, LV_STATE_CHECKED);
    lv_obj_add_style(btn, &style_button_dis, LV_STATE_DISABLED);
    lv_obj_add_event_cb(btn, click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *icon = lv_image_create(btn);
    lv_image_set_src(icon, &img_lv_demo_music_btn_list_play);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 2);

    lv_obj_t *title_label = lv_label_create(btn);
    lv_label_set_text(title_label, title);
    lv_obj_set_grid_cell(title_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_style(title_label, &style_title, 0);
    lv_obj_set_width(title_label, 290);
    lv_obj_set_height(title_label, lv_font_get_line_height(font_22));
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_MODE_DOTS);

    lv_obj_t *artist_label = lv_label_create(btn);
    lv_label_set_text(artist_label, artist);
    lv_obj_add_style(artist_label, &style_artist, 0);
    lv_obj_set_grid_cell(artist_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_width(artist_label, 290);
    lv_obj_set_height(artist_label, lv_font_get_line_height(font_16));
    lv_label_set_long_mode(artist_label, LV_LABEL_LONG_MODE_DOTS);

    lv_obj_t *time_label = lv_label_create(btn);
    lv_label_set_text_fmt(time_label, "%" LV_PRIu32 ":%02" LV_PRIu32, time / 60, time % 60);
    lv_obj_add_style(time_label, &style_time, 0);
    lv_obj_set_grid_cell(time_label, LV_GRID_ALIGN_END, 2, 1, LV_GRID_ALIGN_CENTER, 0, 2);

    lv_obj_t *border = lv_image_create(btn);
    lv_image_set_src(border, &img_lv_demo_music_list_border);
    lv_image_set_inner_align(border, LV_IMAGE_ALIGN_TILE);
    lv_obj_set_width(border, lv_pct(120));
    lv_obj_align(border, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(border, LV_OBJ_FLAG_IGNORE_LAYOUT);

    view->view = btn;
    view->title = title_label;
    view->auther = artist_label;
    view->time = time_label;

    lv_obj_set_user_data(btn, view);

    return view;
}