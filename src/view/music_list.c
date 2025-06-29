/*********************
 *      INCLUDES
 *********************/
#include "music_list.h"
#include "music_main.h"
#include "view_input.h"
#include "font.h"
#include "player.h"

#include "mln_utils.h"

#include <string.h>
#include <malloc.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void search_done(bool iscancel);
static view_play_item *get_item(uint32_t index);
static void clear_click_event_cb(lv_event_t *e);
static void search_click_event_cb(lv_event_t *e);
static void play_click_event_cb(lv_event_t *e);
static void list_delete_event_cb(lv_event_t *e);
static void item_check(view_play_item *item, bool state);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t *list_obj;
static lv_obj_t *search_obj;
static lv_obj_t *search_text_obj;
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

static mln_list_t view_play_list = mln_list_null();

static uint8_t view_list_search_data[1024] = {0};
static uint8_t *view_list_empty = "";

static bool is_search = false;

static void search_done(bool iscancel)
{
    view_play_item *t, *first = NULL;

    if (strlen(view_list_search_data) == 0)
    {
        is_search = false;
        lv_obj_add_flag(search_obj, LV_OBJ_FLAG_HIDDEN);
        for (t = mln_container_of(mln_list_head(&view_play_list), view_play_item, node);
             t != NULL;
             t = mln_container_of(mln_list_next(&t->node), view_play_item, node))
        {
            lv_obj_remove_flag(t->view, LV_OBJ_FLAG_HIDDEN);

            view_play_item *item = get_item(play_now_index);
            if (item != NULL)
            {
                lv_obj_scroll_to_view(item->view, LV_ANIM_ON);
            }
        }

        return;
    }

    for (t = mln_container_of(mln_list_head(&view_play_list), view_play_item, node);
         t != NULL;
         t = mln_container_of(mln_list_next(&t->node), view_play_item, node))
    {
        if (strstr(lv_label_get_text(t->title), view_list_search_data) || strstr(lv_label_get_text(t->auther), view_list_search_data))
        {
            if (first == NULL)
            {
                first = t;
            }
            lv_obj_remove_flag(t->view, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(t->view, LV_OBJ_FLAG_HIDDEN);
        }
    }

    lv_label_set_text_fmt(search_text_obj, "搜索包含“%s”的结果", view_list_search_data);
    if (first != NULL)
    {
        lv_obj_scroll_to_view(first->view, LV_ANIM_ON);
    }
    lv_obj_remove_flag(search_obj, LV_OBJ_FLAG_HIDDEN);
}

static view_play_item *get_item(uint32_t index)
{
    view_play_item *t;
    for (t = mln_container_of(mln_list_head(&view_play_list), view_play_item, node);
         t != NULL;
         t = mln_container_of(mln_list_next(&t->node), view_play_item, node))
    {
        if (t->index == index)
        {
            return t;
        }
    }

    return NULL;
}

static void clear_click_event_cb(lv_event_t *e)
{
    memset(view_list_search_data, 0, sizeof(view_list_search_data));
    search_done(false);
    view_play_item *item = get_item(play_now_index);
    if (item != NULL)
    {
        lv_obj_scroll_to_view(item->view, LV_ANIM_ON);
    }
}

static void search_click_event_cb(lv_event_t *e)
{
    lv_memset(view_list_search_data, 0, sizeof(view_list_search_data));
    input_show(view_list_search_data, sizeof(view_list_search_data), search_done);
}

static void play_click_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);

    view_play_item *item = lv_obj_get_user_data(btn);

    if (item->index != play_now_index)
    {
        uint32_t last = play_now_index;
        play_jump_index(item->index);
        lv_music_list_button_check(last, false);
    }
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
}

static void item_check(view_play_item *item, bool state)
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

lv_obj_t *lv_music_list_create(lv_obj_t *parent)
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
    lv_obj_add_event_cb(list_obj, list_delete_event_cb, LV_EVENT_DELETE, NULL);
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

    lv_obj_t *button = lv_button_create(obj);
    lv_obj_remove_style_all(button);
    lv_obj_set_size(button, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_margin_bottom(button, 10, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(button, 255, 0);
    lv_obj_set_style_radius(button, 5, 0);
    lv_obj_set_style_pad_left(button, 5, 0);
    lv_obj_set_style_pad_right(button, 5, 0);
    lv_obj_set_style_border_color(button, lv_color_hex(0x4c4965), 0);
    lv_obj_set_style_border_opa(button, 255, 0);
    lv_obj_add_event_cb(button, clear_click_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, "清除搜索结果");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_grid_cell(button, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);

    lv_obj_t *border = lv_image_create(search_obj);
    lv_image_set_src(border, &img_lv_demo_music_list_border);
    lv_image_set_inner_align(border, LV_IMAGE_ALIGN_TILE);
    lv_obj_set_width(border, lv_pct(120));
    lv_obj_align(border, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(border, LV_OBJ_FLAG_IGNORE_LAYOUT);

    LV_IMAGE_DECLARE(img_lv_list_search);

    lv_obj_t *search_obj = lv_image_create(parent);
    lv_image_set_src(search_obj, &img_lv_list_search);
    lv_obj_add_event_cb(search_obj, search_click_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(search_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(search_obj, LV_ALIGN_BOTTOM_RIGHT, -10, 0);

    return list_obj;
}

void lv_music_list_button_check(uint32_t index, bool state)
{
    view_play_item *item = get_item(index);
    if (item != NULL)
    {
        item_check(item, state);
    }
}

void lv_list_button_reload(play_item *item)
{
    view_play_item *view = get_item(item->index);
    if (view != NULL)
    {
        uint8_t *title = item->title ? item->title : item->path;
        uint8_t *artist = item->auther ? item->auther : view_list_empty;
        uint32_t time = (uint32_t)item->time;
        lv_label_set_text(view->title, title);
        lv_label_set_text(view->auther, artist);
        lv_label_set_text_fmt(view->time, "%" LV_PRIu32 ":%02" LV_PRIu32, time / 60, time % 60);
    }
}

void lv_list_add_item(play_item *item)
{
    view_play_item *view;
    view = (view_play_item *)calloc(1, sizeof(*view));
    if (view == NULL)
    {
        return;
    }

    uint32_t time = (uint32_t)item->time;
    uint8_t *title = item->title ? item->title : item->path;
    uint8_t *artist = item->auther ? item->auther : view_list_empty;

    lv_obj_t *btn = lv_obj_create(list_obj);
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, lv_pct(100), 110);

    lv_obj_add_style(btn, &style_btn, 0);
    lv_obj_add_style(btn, &style_button_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn, &style_button_chk, LV_STATE_CHECKED);
    lv_obj_add_style(btn, &style_button_dis, LV_STATE_DISABLED);
    lv_obj_add_event_cb(btn, play_click_event_cb, LV_EVENT_CLICKED, NULL);

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

    view->index = item->index;
    view->view = btn;
    view->title = title_label;
    view->auther = artist_label;
    view->time = time_label;

    lv_obj_set_user_data(btn, view);

    mln_list_add(&view_play_list, &view->node);
}

void lv_list_clear()
{
    view_play_item *t, *fr;
    for (t = mln_container_of(mln_list_head(&view_play_list), view_play_item, node); t != NULL;)
    {
        fr = t;
        lv_obj_delete(t->view);
        t = mln_container_of(mln_list_next(&t->node), view_play_item, node);
        free(fr);
    }
}
