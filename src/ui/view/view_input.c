#include "view_input.h"
#include "view_keyborad.h"

#include "../anim.h"
#include "../font.h"

#include "../input/rime_input.h"

#include <string.h>

static void select_key(uint8_t *key);
static void create_select(lv_obj_t *parent);
static void update();
static void show_composition(RimeComposition *composition);
static void select_click_event_cb(lv_event_t *e);
static void show_menu(RimeMenu *menu);
static void put_text(uint8_t *text);
static void select_update();
static void input_text(uint8_t *txt);
static void keyboard_event_cb(lv_event_t *e);

static lv_obj_t *kb;
static lv_obj_t *input_obj;
static lv_obj_t *input_select_obj;
static lv_obj_t *input_select_text_obj;
static lv_obj_t *input_select_list_obj;
static lv_obj_t *input_select_page_obj;

static lv_obj_t *input_text_obj;

static lv_style_t style_select_scrollbar;
static lv_style_t style_select_btn;

static uint8_t input_temp[256] = {0};
static uint32_t input_max_page;
static uint32_t input_now_page;
static uint16_t input_max_len;

static bool input_have_data = false;
static bool is_ch_mode;

static uint8_t *input_char_table[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9"};
static void(*close)();

static void create_select(lv_obj_t *parent)
{
    input_select_obj = lv_obj_create(parent);
    lv_obj_remove_style_all(input_select_obj);
    lv_obj_set_style_bg_opa(input_select_obj, 255, 0);
    lv_obj_set_style_bg_color(input_select_obj, lv_color_hex(0xf5f5f5), 0);
    lv_obj_set_size(input_select_obj, LV_HOR_RES, LV_SIZE_CONTENT);
    lv_obj_add_flag(input_select_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_pad_all(input_select_obj, 5, 0);

    static const int32_t grid_cols[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static const int32_t grid_rows[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

    lv_obj_set_style_grid_column_dsc_array(input_select_obj, grid_cols, 0);
    lv_obj_set_style_grid_row_dsc_array(input_select_obj, grid_rows, 0);
    lv_obj_set_style_grid_column_align(input_select_obj, LV_GRID_ALIGN_START, 0);
    lv_obj_set_style_grid_row_align(input_select_obj, LV_GRID_ALIGN_CENTER, 0);
    lv_obj_set_style_layout(input_select_obj, LV_LAYOUT_GRID, 0);

    lv_obj_t *obj = lv_label_create(input_select_obj);
    lv_label_set_text(obj, "中");
    lv_obj_set_style_text_font(obj, font_32, 0);
    lv_obj_set_style_pad_left(obj, 5, 0);
    lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 2);

    input_select_page_obj = lv_label_create(input_select_obj);
    lv_label_set_text(input_select_page_obj, "0 / 0");
    lv_obj_set_align(input_select_page_obj, LV_ALIGN_LEFT_MID);
    lv_obj_set_style_pad_left(input_select_page_obj, 10, 0);
    lv_obj_set_grid_cell(input_select_page_obj, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);

    input_select_text_obj = lv_label_create(input_select_obj);
    lv_obj_remove_style_all(input_select_text_obj);
    lv_obj_set_height(input_select_text_obj, LV_SIZE_CONTENT);
    lv_label_set_text(input_select_text_obj, "");
    lv_obj_set_align(input_select_text_obj, LV_ALIGN_LEFT_MID);
    lv_obj_set_style_pad_left(input_select_text_obj, 10, 0);
    lv_obj_set_grid_cell(input_select_text_obj, LV_GRID_ALIGN_START, 1, 2, LV_GRID_ALIGN_CENTER, 0, 1);

    input_select_list_obj = lv_obj_create(input_select_obj);
    lv_obj_remove_style_all(input_select_list_obj);
    // lv_obj_set_width(input_select_list_obj, LV_SIZE_CONTENT);
    lv_obj_set_height(input_select_list_obj, LV_SIZE_CONTENT);
    // lv_obj_set_style_pad_left(input_select_list_obj, 5, 0);
    lv_obj_set_style_margin_left(input_select_list_obj, 5, 0);
    lv_obj_set_style_margin_right(input_select_list_obj, 5, 0);
    lv_obj_set_grid_cell(input_select_list_obj, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_add_style(input_select_list_obj, &style_select_scrollbar, LV_PART_SCROLLBAR);
    lv_obj_set_flex_flow(input_select_list_obj, LV_FLEX_FLOW_ROW);
}

static void update()
{
    if (is_ch_mode)
    {
        lv_obj_remove_flag(input_select_obj, LV_OBJ_FLAG_HIDDEN);
        rime_start_session();
    }
    else
    {
        lv_obj_add_flag(input_select_obj, LV_OBJ_FLAG_HIDDEN);
        rime_close_session();
        lv_obj_clean(input_select_list_obj);
        lv_label_set_text(input_select_text_obj, "");
        lv_label_set_text(input_select_page_obj, "");
    }
}

static void show_composition(RimeComposition *composition)
{
    const char *preedit = composition->preedit;
    if (!preedit)
        return;
    size_t index = 0;
    size_t len = strlen(preedit);
    size_t start = composition->sel_start;
    size_t end = composition->sel_end;
    size_t cursor = composition->cursor_pos;
    for (size_t i = 0; i <= len; ++i)
    {
        if (start < end)
        {
            if (i == start)
            {
                input_temp[index++] = '[';
            }
            else if (i == end)
            {
                input_temp[index++] = ']';
            }
        }
        if (i == cursor)
            input_temp[index++] = '|';
        if (i < len)
        {
            input_have_data = true;
            input_temp[index++] = preedit[i];
        }
    }
    input_temp[index++] = 0;
}

static void select_click_event_cb(lv_event_t *e)
{
    uint8_t *data = (uint8_t *)lv_event_get_user_data(e);
    select_key(data);
}

static void show_menu(RimeMenu *menu)
{
    input_now_page = 0;
    input_max_page = 0;
    if (menu->num_candidates == 0)
    {
        lv_label_set_text(input_select_page_obj, "");
        return;
    }

    lv_label_set_text_fmt(input_select_page_obj, "%d / %d", menu->page_no + 1, menu->page_size);

    input_now_page = menu->page_no;
    input_max_page = menu->page_size;

    for (uint16_t i = 0; i < menu->num_candidates; ++i)
    {
        bool highlighted = i == menu->highlighted_candidate_index;

        lv_obj_t *button = lv_button_create(input_select_list_obj);
        lv_obj_t *label = lv_label_create(button);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

        lv_obj_remove_style_all(button);
        lv_obj_remove_style_all(label);

        lv_obj_set_height(label, 20);
        lv_obj_set_height(button, LV_SIZE_CONTENT);

        lv_obj_add_style(button, &style_select_btn, 0);
        lv_obj_add_event_cb(button, select_click_event_cb, LV_EVENT_CLICKED, input_char_table[i]);

        lv_label_set_text_fmt(label, "%d. %c%s%c%s\n", i + 1, highlighted ? '[' : ' ',
                              menu->candidates[i].text, highlighted ? ']' : ' ',
                              menu->candidates[i].comment ? menu->candidates[i].comment : "");
    }
}

static void put_text(uint8_t *text)
{
    uint16_t len = strlen(text);
    if (len + strlen(lv_textarea_get_text(input_text_obj)) > input_max_len - 1)
    {
        return;
    }
    lv_textarea_add_text(input_text_obj, text);
}

static void select_update()
{
    lv_obj_clean(input_select_list_obj);

    RIME_STRUCT(RimeCommit, commit);
    if (rime_get_commit(&commit))
    {
        put_text(commit.text);
    }
    rime_close_commit(&commit);

    RIME_STRUCT(RimeContext, context);
    if (rime_get_context(&context))
    {
        input_have_data = false;
        memset(input_temp, 0, sizeof(input_temp));
        if (context.composition.length > 0 || context.menu.num_candidates > 0)
        {
            show_composition(&context.composition);
            lv_label_set_text(input_select_text_obj, input_temp);
        }
        else
        {
            lv_label_set_text(input_select_text_obj, "");
        }

        show_menu(&context.menu);
    }
    rime_close_context(&context);
}

static void select_key(uint8_t *key)
{
    if (rime_put_key(key))
    {
        select_update();
    }
}

static void input_text(uint8_t *txt)
{
    if (lv_strcmp(txt, "Enter") == 0 || lv_strcmp(txt, LV_SYMBOL_NEW_LINE) == 0)
    {
        lv_textarea_add_char(input_text_obj, '\n');
        if (lv_textarea_get_one_line(input_text_obj))
        {
            lv_result_t res = lv_obj_send_event(input_text_obj, LV_EVENT_READY, NULL);
            if (res != LV_RESULT_OK)
                return;
        }
    }
    else if (lv_strcmp(txt, LV_SYMBOL_LEFT) == 0)
    {
        if (is_ch_mode && input_have_data)
        {
            if (input_now_page != 0)
            {
                if (rime_change_page(true))
                {
                    select_update();
                }
            }
        }
        else
        {
            lv_textarea_cursor_left(input_text_obj);
        }
    }
    else if (lv_strcmp(txt, LV_SYMBOL_RIGHT) == 0)
    {
        if (is_ch_mode && input_have_data)
        {
            if (input_max_page > 0 && input_now_page + 1 < input_max_page)
            {
                if (rime_change_page(false))
                {
                    select_update();
                }
            }
        }
        else
        {
            lv_textarea_cursor_right(input_text_obj);
        }
    }
    else if (lv_strcmp(txt, LV_SYMBOL_BACKSPACE) == 0)
    {
        if (is_ch_mode && input_have_data)
        {
            select_key("{BackSpace}");
        }
        else
        {
            lv_textarea_delete_char(input_text_obj);
        }
    }
    else if (lv_strcmp(txt, "+/-") == 0)
    {
        uint32_t cur = lv_textarea_get_cursor_pos(input_text_obj);
        const char *ta_txt = lv_textarea_get_text(input_text_obj);
        if (ta_txt[0] == '-')
        {
            lv_textarea_set_cursor_pos(input_text_obj, 1);
            lv_textarea_delete_char(input_text_obj);
            lv_textarea_add_char(input_text_obj, '+');
            lv_textarea_set_cursor_pos(input_text_obj, cur);
        }
        else if (ta_txt[0] == '+')
        {
            lv_textarea_set_cursor_pos(input_text_obj, 1);
            lv_textarea_delete_char(input_text_obj);
            lv_textarea_add_char(input_text_obj, '-');
            lv_textarea_set_cursor_pos(input_text_obj, cur);
        }
        else
        {
            lv_textarea_set_cursor_pos(input_text_obj, 0);
            lv_textarea_add_char(input_text_obj, '-');
            lv_textarea_set_cursor_pos(input_text_obj, cur + 1);
        }
    }
    else if (is_ch_mode)
    {
        select_key((uint8_t *)txt);
    }
    else
    {
        put_text(txt);
    }
}

static void keyboard_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CANCEL)
    {
        is_ch_mode = !is_ch_mode;
        update();
    }
    else if (code == LV_EVENT_READY)
    {
        close();
    }
}

void lv_input_set_max_size(uint16_t size)
{
    input_max_len = size;
}

void lv_input_set_text(const char *text)
{
    lv_textarea_set_text(input_text_obj, text);
}

const char *lv_input_get_text()
{
    return lv_textarea_get_text(input_text_obj);
}

void lv_input_display(bool display)
{
    anim_set_display(input_obj, display);
}

void lv_input_create(lv_obj_t *parent, void(*close_cb)())
{
    close = close_cb;

    lv_style_init(&style_select_scrollbar);
    // lv_style_set_width(&style_select_scrollbar, 4);
    // lv_style_set_bg_opa(&style_select_scrollbar, LV_OPA_COVER);
    // lv_style_set_bg_color(&style_select_scrollbar, lv_color_hex3(0xeee));
    // lv_style_set_radius(&style_select_scrollbar, LV_RADIUS_CIRCLE);
    // lv_style_set_pad_right(&style_select_scrollbar, 4);

    lv_style_init(&style_select_btn);
    lv_style_set_bg_opa(&style_select_btn, 255);
    lv_style_set_bg_color(&style_select_btn, lv_color_hex(0xFFFFFF));
    lv_style_set_pad_left(&style_select_btn, 5);
    lv_style_set_pad_right(&style_select_btn, 5);
    lv_style_set_margin_left(&style_select_btn, 5);
    lv_style_set_margin_right(&style_select_btn, 5);
    lv_style_set_radius(&style_select_btn, 5);

    input_obj = lv_obj_create(parent);
    lv_obj_remove_style_all(input_obj);
    lv_obj_set_size(input_obj, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(input_obj, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(input_obj, 100, 0);

    kb = lv_keyboard_view_create(input_obj);

    lv_keyboard_view_set_input(kb, input_text);
    lv_obj_add_event_cb(kb, keyboard_event_cb, LV_EVENT_ALL, NULL);

    input_text_obj = lv_textarea_create(input_obj);
    lv_obj_align(input_text_obj, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_pad_all(input_text_obj, 8, 0);
    lv_obj_set_size(input_text_obj, LV_HOR_RES - 20, LV_SIZE_CONTENT);

    lv_obj_set_style_text_font(kb, &lv_font_dejavu_16_persian_hebrew, 0);

    create_select(input_obj);
    lv_obj_align_to(input_select_obj, kb, LV_ALIGN_OUT_TOP_MID, 0, 10);

    lv_obj_add_flag(input_obj, LV_OBJ_FLAG_HIDDEN);
}
