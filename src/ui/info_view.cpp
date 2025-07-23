#include "info_view.h"

#include "view/view_top_info.h"

static bool display;
static bool use_bar;
static std::string text;

static bool update_top_info;

static void timer_tick(lv_timer_t *timer)
{
    if (update_top_info)
    {
        view_top_info_update();
        update_top_info = false;
    }
}

void view_top_info_display(std::string info)
{
    use_bar = true;
    display = true;
    update_top_info = true;
    text = info;
}

void view_top_error_display(std::string info)
{
    use_bar = false;
    display = true;
    update_top_info = true;
    text = info;
}

void view_top_info_close()
{
    display = false;
    update_top_info = true;
}

bool view_top_info_is_display()
{
    return display;
}

void view_top_info_update()
{
    if (use_bar)
    {
        lv_info_scr_display(display);
    }
    lv_info_display(display);
    lv_info_set_text(text.c_str());
}

void view_top_info_create(lv_obj_t *parent)
{
    lv_info_create(parent);
    lv_timer_create(timer_tick, 500, NULL);
}