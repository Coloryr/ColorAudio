#include "music_list.h"
#include "music_main.h"
#include "view/view_input.h"
#include "view/view_music_list.h"

#include "../music/music.h"
#include "../player/player.h"

#include <boost/container/flat_map.hpp>
#include <boost/system.hpp>
#include <string.h>
#include <malloc.h>
#include <map>

static void search_done(bool iscancel);
static void clear_click_event_cb(lv_event_t *e);
static void search_click_event_cb(lv_event_t *e);
static void play_click_event_cb(lv_event_t *e);
static void list_delete_event_cb(lv_event_t *e);

static char view_list_search_data[512] = {0};

static boost::container::flat_map<uint32_t, view_play_item_t *> view_play_list;

static bool is_search = false;

static void search_done(bool iscancel)
{
    view_play_item_t *item = NULL;
    if (strlen(view_list_search_data) == 0)
    {
        is_search = false;
        view_music_list_search_display(false);

        for (const auto &pair : view_play_list)
        {
            lv_obj_remove_flag(pair.second->view, LV_OBJ_FLAG_HIDDEN);
        }

        item = view_play_list[play_now_index];
        if (item != NULL)
        {
            lv_obj_scroll_to_view(item->view, LV_ANIM_ON);
        }

        return;
    }

    for (const auto &pair : view_play_list)
    {
        if (strstr(lv_label_get_text(pair.second->title), view_list_search_data) || strstr(lv_label_get_text(pair.second->auther), view_list_search_data))
        {
            if (item == NULL)
            {
                item = pair.second;
            }
            lv_obj_remove_flag(pair.second->view, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(pair.second->view, LV_OBJ_FLAG_HIDDEN);
        }
    }

    view_music_list_search_text(view_list_search_data);
    view_music_list_search_display(true);
    if (item != NULL)
    {
        lv_obj_scroll_to_view(item->view, LV_ANIM_ON);
    }
}

static void clear_click_event_cb(lv_event_t *e)
{
    memset(view_list_search_data, 0, sizeof(view_list_search_data));
    search_done(false);
    view_play_item_t *item = view_play_list[play_now_index];
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
    lv_obj_t *btn = static_cast<lv_obj_t *>(lv_event_get_target(e));
    view_play_item_t *item = static_cast<view_play_item_t *>(lv_obj_get_user_data(btn));

    if (item->index != play_now_index)
    {
        play_jump_index(item->index);
    }
}

void view_music_list_button_check(uint32_t index, bool state)
{
    if (!view_play_list.contains(index))
    {
        return;
    }
    view_play_item_t *item = view_play_list[index];
    if (item != NULL)
    {
        item_check(item, state);
    }
}

void lv_list_clear()
{
    for (const auto &pair : view_play_list)
    {
        lv_obj_delete(pair.second->view);
        free(pair.second);
    }

    view_play_list.clear();
}

void lv_muisc_list_item_reload(play_item *item)
{
    view_play_item_t *view = view_play_list[item->index];
    if (view != NULL)
    {
        uint32_t time = static_cast<uint32_t>(item->time);
        lv_label_set_text(view->title, item->title.c_str());
        lv_label_set_text(view->auther, item->auther.c_str());
        lv_label_set_text_fmt(view->time, "%" LV_PRIu32 ":%02" LV_PRIu32, time / 60, time % 60);
    }
}

lv_obj_t *lv_music_list_create(lv_obj_t *parent)
{
    return view_music_list_create(parent, clear_click_event_cb, search_click_event_cb);
}

void lv_list_add_item(play_item *item)
{
    view_play_item_t *view = view_list_add_item(item->title.c_str(), item->auther.c_str(),
                                                static_cast<uint32_t>(item->time), play_click_event_cb);
    view->index = item->index;
    view_play_list[item->index] = view;
    auto temp = view_play_list.end();
    uint32_t size = view_play_list.size();
}