/*********************
 *      INCLUDES
 *********************/
#include "music_list.h"
#include "music_main.h"
#include "font.h"
#include "player.h"

#include "mln_utils.h"

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
static void btn_click_event_cb(lv_event_t *e);
static void list_delete_event_cb(lv_event_t *e);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t *list;
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

static mln_list_t view_play_list = mln_list_null();

static uint8_t *view_list_empty = "";

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

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
    list = lv_obj_create(parent);
    lv_obj_add_event_cb(list, list_delete_event_cb, LV_EVENT_DELETE, NULL);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, LV_HOR_RES, LV_VER_RES - LV_DEMO_MUSIC_HANDLE_SIZE);
    lv_obj_set_y(list, LV_DEMO_MUSIC_HANDLE_SIZE);
    lv_obj_add_style(list, &style_scrollbar, LV_PART_SCROLLBAR);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

    return list;
}

void lv_music_list_button_check(uint32_t index, bool state)
{
    lv_obj_t *btn = lv_obj_get_child(list, index);
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

void view_list_button_reload(play_item *item)
{
    view_play_item *t, *fr;
    for (t = mln_container_of(mln_list_head(&view_play_list), view_play_item, node);
         t != NULL;
         t = mln_container_of(mln_list_next(&t->node), view_play_item, node))
    {
        if (t->index == item->index)
        {
            uint8_t *title = item->title ? item->title : item->path;
            uint8_t *artist = item->auther ? item->auther : view_list_empty;
            uint32_t time = (uint32_t)item->time;
            lv_label_set_text(t->title, title);
            lv_label_set_text(t->auther, artist);
            lv_label_set_text_fmt(t->time, "%" LV_PRIu32 ":%02" LV_PRIu32, time / 60, time % 60);
        }
    }
}

void view_add_list_button(play_item *item)
{
    view_play_item *t;
    t = (view_play_item *)calloc(1, sizeof(*t));
    if (t == NULL)
    {
        return;
    }

    uint32_t time = (uint32_t)item->time;
    uint8_t *title = item->title ? item->title : item->path;
    uint8_t *artist = item->auther ? item->auther : view_list_empty;

    lv_obj_t *btn = lv_obj_create(list);
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, lv_pct(100), 110);

    lv_obj_add_style(btn, &style_btn, 0);
    lv_obj_add_style(btn, &style_button_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn, &style_button_chk, LV_STATE_CHECKED);
    lv_obj_add_style(btn, &style_button_dis, LV_STATE_DISABLED);
    lv_obj_add_event_cb(btn, btn_click_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *icon = lv_image_create(btn);
    lv_image_set_src(icon, &img_lv_demo_music_btn_list_play);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 2);

    lv_obj_t *title_label = lv_label_create(btn);
    lv_label_set_text(title_label, title);
    lv_obj_set_grid_cell(title_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_style(title_label, &style_title, 0);

    lv_obj_t *artist_label = lv_label_create(btn);
    lv_label_set_text(artist_label, artist);
    lv_obj_add_style(artist_label, &style_artist, 0);
    lv_obj_set_grid_cell(artist_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);

    lv_obj_t *time_label = lv_label_create(btn);
    lv_label_set_text_fmt(time_label, "%" LV_PRIu32 ":%02" LV_PRIu32, time / 60, time % 60);
    lv_obj_add_style(time_label, &style_time, 0);
    lv_obj_set_grid_cell(time_label, LV_GRID_ALIGN_END, 2, 1, LV_GRID_ALIGN_CENTER, 0, 2);

    LV_IMAGE_DECLARE(img_lv_demo_music_list_border);
    lv_obj_t *border = lv_image_create(btn);
    lv_image_set_src(border, &img_lv_demo_music_list_border);
    lv_image_set_inner_align(border, LV_IMAGE_ALIGN_TILE);
    lv_obj_set_width(border, lv_pct(120));
    lv_obj_align(border, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(border, LV_OBJ_FLAG_IGNORE_LAYOUT);

    t->index = item->index;
    t->view = btn;
    t->title = title_label;
    t->auther = artist_label;
    t->time = time_label;

    mln_list_add(&view_play_list, &t->node);
}

static void btn_click_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);

    uint32_t idx = lv_obj_get_index(btn);

    if (idx != play_now_index)
    {
        uint32_t last = play_now_index;
        play_jump_index(idx);
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

void view_list_clear()
{
    view_play_item *t, *fr;
    for (t = mln_container_of(mln_list_head(&view_play_list), view_play_item, node); t != NULL;)
    {
        fr = t;
        t = mln_container_of(mln_list_next(&t->node), view_play_item, node);
        free(fr);
    }
    lv_obj_clean(list);
}
