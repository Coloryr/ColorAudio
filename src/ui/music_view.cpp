#include "music_view.h"

#include "input_view.h"
#include "view_setting.h"
#include "view/view_lyric.h"
#include "view/view_music_main.h"
#include "view/view_music_list.h"

#include "../music/player_info.h"
#include "../music/music_player.h"
#include "../sound/sound.h"
#include "../music/lyric.h"

#include "lvgl.h"

#include <boost/container/flat_map.hpp>

static LyricParser *lyric_data = NULL;
static LyricParser *lyric_tr_data = NULL;

static lv_obj_t *ctrl_view;
static lv_obj_t *list_view;

static pthread_mutex_t lyric_mutex;

static lyric_state ly_state = LYRIC_UNKNOW;

static bool enable_mp4;
static uint32_t image_w, image_h;
static uint32_t check_list_button, uncheck_list_button;
static uint8_t *image_d;

static float last_mute;

static bool update_info;
static bool clear_info;
static bool update_img;
static bool update_state;
static bool update_list;
static bool init_list;
static bool mp4_have_update;

static void lyric_tick(lv_timer_t *timer)
{
    if (ly_state == LYRIC_NONE)
    {
        view_music_set_lyric(nullptr, nullptr);
        lv_lyric_set_text("无歌词");
        lv_lyric_tr_set_text("");
        lv_lyric_k_set_text("");
        lv_lyric_k_now_set_text("");
        ly_state = LYRIC_UNKNOW;
        return;
    }
    else if (ly_state == LYRIC_FAIL)
    {
        view_music_set_lyric(nullptr, nullptr);
        lv_lyric_set_text("歌词获取失败");
        lv_lyric_tr_set_text("");
        lv_lyric_k_set_text("");
        lv_lyric_k_now_set_text("");
        ly_state = LYRIC_UNKNOW;
        return;
    }
    else if (ly_state == LYRIC_CLEAR)
    {
        view_music_set_lyric(nullptr, nullptr);
        lv_lyric_set_text("");
        lv_lyric_k_set_text("");
        lv_lyric_k_now_set_text("");
        lv_lyric_tr_set_text("");
        ly_state = LYRIC_UNKNOW;
        return;
    }

    if (play_state != MUSIC_STATE_PLAY && play_state != MUSIC_STATE_PAUSE)
    {
        return;
    }

    pthread_mutex_lock(&lyric_mutex);

    bool find = false;
    std::string text, ktext, know_text;
    float kp;
    bool have_k;
    if (lyric_data)
    {
        if (lyric_data->get_lyric(text, ktext, know_text, kp, have_k, time_now * 1000))
        {
            find = true;
            lv_lyric_set_text(text.c_str());
            lv_lyric_k_set_text(ktext.c_str());
            lv_lyric_k_now_set_text(know_text.c_str());
            lv_lyric_set_have_k(have_k);
            lv_lyric_kp_set_text(kp);
        }
        else
        {
            lv_lyric_set_text("");
        }
    }
    if (lyric_tr_data)
    {
        if (!find)
        {
            lv_lyric_tr_set_text("");
        }
        else
        {
            if (lyric_tr_data->get_lyric(text, ktext, know_text, kp, have_k, time_now * 1000))
            {
                lv_lyric_tr_set_text(text.c_str());
            }
            else
            {
                lv_lyric_tr_set_text("");
            }
        }
    }

    if (find)
    {
        lv_lyric_draw();
    }

    pthread_mutex_unlock(&lyric_mutex);
}

static void timer_tick(lv_timer_t *timer)
{
    lv_music_set_now_time(time_now);
    lv_music_set_list_info(play_list_count, play_now_index);
    lv_music_volume_timer_tick();

    if (clear_info)
    {
        lv_music_fft_clear();
        lv_music_set_title("");
        clear_info = false;
    }

    if (update_img)
    {
        if (image)
        {
            lv_music_set_image(image->data, image->size);
        }
        else
        {
            lv_music_set_image(NULL, 0);
        }

        update_img = false;
    }

    if (update_info)
    {
        lv_music_set_all_time(time_all);
        lv_music_set_title(title.c_str());
        lv_music_set_album(album.c_str());
        lv_music_set_auther(auther.c_str());

        lv_music_set_sound_info(pcm_now_format, pcm_now_rate, pcm_now_channels, play_music_bps);

        update_info = false;
    }

    if (update_state)
    {
        if (play_state == MUSIC_STATE_PLAY)
        {
            lv_music_set_play();
        }
        else
        {
            lv_music_set_pause();
        }

        lv_music_set_play_mode();
        lv_music_set_volume(alsa_get_volume());

        update_state = false;
    }

    if (init_list)
    {
        view_muisc_list_clear();

        for (const auto &it : play_list)
        {
            view_muisc_list_add(it);
        }

        view_music_list_check(play_now_index, true);

        init_list = false;
    }

    if (update_list)
    {
         for (const auto &it : play_list)
        {
            view_muisc_list_reload(it);
        }
        update_list = false;
    }

    if (check_list_button != UINT32_MAX)
    {
        view_music_list_check(check_list_button, true);

        check_list_button = UINT32_MAX;
    }
    if (uncheck_list_button != UINT32_MAX)
    {
        view_music_list_check(uncheck_list_button, false);

        uncheck_list_button = UINT32_MAX;
    }

    if (enable_mp4)
    {
        lv_music_set_image_data(image_w, image_h, image_d);
        enable_mp4 = false;
    }

    if (mp4_have_update)
    {
        lv_music_img_load();
        mp4_have_update = false;
    }
}

static void search_done(bool iscancel);
static void clear_click_event_cb(lv_event_t *e);
static void search_click_event_cb(lv_event_t *e);
static void play_click_event_cb(lv_event_t *e);
static void list_event_cb(lv_event_t *e);

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
    view_input_show(view_list_search_data, sizeof(view_list_search_data), search_done);
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

static void mode_click_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    play_set_command(MUSIC_COMMAND_CHANGE_MODE);
}

static void prev_click_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    play_set_command(MUSIC_COMMAND_LAST);
}

static void next_click_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    play_set_command(MUSIC_COMMAND_NEXT);
}

static void time_change_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target_obj(e);
    if (!lv_slider_is_dragged(obj))
    {
        uint32_t value = lv_slider_get_value(obj);

        play_jump_time(value);
    }
}

static void play_event_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target_obj(e);
    if (lv_obj_has_state(obj, LV_STATE_CHECKED))
    {
        if (play_set_command(MUSIC_COMMAND_PLAY))
        {
            lv_music_set_play();
        }
    }
    else
    {
        if (play_set_command(MUSIC_COMMAND_PAUSE))
        {
            lv_music_set_pause();
        }
    }
}

static void volume_click_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target_obj(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        int sel = lv_slider_get_value(obj);
        alsa_set_volume(sel);
        last_mute = 0;
    }
    else if (code == LV_EVENT_PRESSED)
    {
        lv_music_set_volume_timer(UINT8_MAX);
    }
    else if (code == LV_EVENT_RELEASED)
    {
        lv_music_set_volume_timer(LV_MUSIC_VOLUME_DISPLAY_TIME);
    }
}

static void mute_click_event_cb(lv_event_t *e)
{
    if (last_mute != 0)
    {
        alsa_set_volume(last_mute);
        lv_music_set_volume(last_mute);
        last_mute = 0;
    }
    else
    {
        last_mute = alsa_get_volume();
        alsa_set_volume(0);
        lv_music_set_volume(0);
    }
    lv_music_set_volume_timer(LV_MUSIC_VOLUME_DISPLAY_TIME);
}

static lv_obj_t *list_create(lv_obj_t *parent)
{
    return lv_music_list_create(parent, clear_click_event_cb, search_click_event_cb);
}

static lv_obj_t *main_create(lv_obj_t *parent)
{
    return lv_music_main_create(parent, time_change_event_cb,
                                volume_click_event_cb, mode_click_event_cb,
                                prev_click_event_cb, play_event_click_cb,
                                next_click_event_cb, mute_click_event_cb);
}

void view_music_list_check(uint32_t index, bool state)
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

void view_muisc_list_clear()
{
    for (const auto &pair : view_play_list)
    {
        lv_obj_delete(pair.second->view);
        free(pair.second);
    }

    view_play_list.clear();
}

void view_muisc_list_reload(play_item *item)
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

void view_muisc_list_add(play_item *item)
{
    view_play_item_t *view = view_list_add_item(item->title.c_str(), item->auther.c_str(),
                                                static_cast<uint32_t>(item->time), play_click_event_cb);
    view->index = item->index;
    view_play_list[item->index] = view;
    auto temp = view_play_list.end();
    uint32_t size = view_play_list.size();
}

void view_music_set_image_data(uint32_t width, uint32_t height, uint8_t *data)
{
    image_w = width;
    image_h = height;
    image_d = data;
    enable_mp4 = true;
}

void view_music_set_lyric_state(lyric_state state)
{
    ly_state = state;
}

void view_music_set_check(uint32_t index, bool enable)
{
    if (enable)
    {
        check_list_button = index;
    }
    else
    {
        uncheck_list_button = index;
    }
}

void view_music_set_lyric(LyricParser *lyric, LyricParser *tlyric)
{
    pthread_mutex_lock(&lyric_mutex);
    if (lyric_data)
    {
        delete lyric_data;
    }
    if (lyric_tr_data)
    {
        delete lyric_tr_data;
    }
    lyric_data = lyric;
    lyric_tr_data = tlyric;
    pthread_mutex_unlock(&lyric_mutex);
}

void view_music_set_display(bool display)
{
    if (display)
    {
        lv_obj_remove_flag(list_view, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ctrl_view, LV_OBJ_FLAG_HIDDEN);
        lv_music_fadein();
    }
    else
    {
        lv_obj_add_flag(list_view, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ctrl_view, LV_OBJ_FLAG_HIDDEN);
    }
}

void view_music_create(lv_obj_t *parent)
{
    pthread_mutex_init(&lyric_mutex, NULL);

    lv_obj_t *obj = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, LV_HOR_RES, LV_VER_RES);

    list_view = list_create(obj);
    ctrl_view = main_create(obj);

    lv_timer_create(timer_tick, 500, NULL);
    lv_timer_create(lyric_tick, 50, NULL);
}

void view_music_tick()
{
    if (play_state == MUSIC_STATE_PLAY)
    {
        lv_music_fft_load();
    }
}

void view_music_update_info()
{
    update_info = true;
}

void view_music_clear_info()
{
    clear_info = true;
}

void view_music_update_img()
{
    update_img = true;
}

void view_music_mp4_update()
{
    mp4_have_update = true;
}

void view_music_update_state()
{
    update_state = true;
}

void view_music_update_list()
{
    update_list = true;
}

void view_music_init_list()
{
    init_list = true;
}