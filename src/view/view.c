#include "view.h"
#include "lvgl.h"

#include "music_list.h"
#include "music_main.h"

#include "player.h"

static lv_obj_t *ctrl;
static lv_obj_t *list;

static lv_timer_t *view_update_timer;

const lv_font_t *font_16;
const lv_font_t *font_22;
const lv_font_t *font_32;

static bool update_info;
static bool update_img;
static bool update_state;

static lv_font_manager_t *g_font_manager = NULL;

static void view_timer_tick(lv_timer_t *timer)
{
    view_set_all_time(time_all);
    view_set_now_time(time_now);

    if (update_img)
    {
        if (image.data)
        {
            view_set_image(image.data, image.size);
        }
        else
        {
            view_set_image(NULL, 0);
        }

        update_img = false;
    }

    if (update_info)
    {
        if (title.data)
        {
            view_set_title(title.data);
        }
        else
        {
            view_set_title("");
        }

        if (album.data)
        {
            view_set_album(album.data);
        }
        else
        {
            view_set_album("");
        }

        if (auther.data)
        {
            view_set_auther(auther.data);
        }
        else
        {
            view_set_auther("");
        }

        update_info = false;
    }

    if (update_state)
    {
        if (is_play)
        {
            lv_music_set_resume();
        }
        else
        {
            lv_music_set_pause();
        }

        update_state = false;
    }

    view_fft_load();
}

void load_font()
{
    /* Create font manager, with 8 fonts recycling buffers */
    g_font_manager = lv_font_manager_create(8);

    /* Add font path mapping to font manager */
    lv_font_manager_add_src_static(g_font_manager,
                                   "MiSans",
                                   "/home/coloryr/MiSans-Regular.ttf",
                                   &lv_freetype_font_class);

    /* Create font from font manager */
    font_16 = lv_font_manager_create_font(g_font_manager,
                                          "MiSans",
                                          LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                          16,
                                          LV_FREETYPE_FONT_STYLE_NORMAL,
                                          LV_FONT_KERNING_NONE);

    font_22 = lv_font_manager_create_font(g_font_manager,
                                          "MiSans",
                                          LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                          22,
                                          LV_FREETYPE_FONT_STYLE_NORMAL,
                                          LV_FONT_KERNING_NONE);

    font_32 = lv_font_manager_create_font(g_font_manager,
                                          "MiSans",
                                          LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                          32,
                                          LV_FREETYPE_FONT_STYLE_NORMAL,
                                          LV_FONT_KERNING_NONE);
}

void view_update_info()
{
    update_info = true;
}
void view_update_img()
{
    update_img = true;
}
void view_update_state()
{
    update_state = true;
}

void view_init()
{
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x343247), 0);

    list = lv_demo_music_list_create(lv_screen_active());
    ctrl = lv_demo_music_main_create(lv_screen_active());

    view_update_timer = lv_timer_create(view_timer_tick, 0, NULL);
}

const char *lv_demo_music_get_title(uint32_t track_id)
{
    return "";
}

const char *lv_demo_music_get_artist(uint32_t track_id)
{
    return "";
}

const char *lv_demo_music_get_genre(uint32_t track_id)
{
    return "";
}

uint32_t lv_demo_music_get_track_length(uint32_t track_id)
{
    return 0;
}