#include "info_view.h"
#include "view_state.h"

#include "view/view_top_info.h"

static bool display;
static bool use_bar;
static std::string text;

void top_info_display(std::string info)
{
    use_bar = true;
    display = true;
    update_top_info = true;
    text = info;
}

void top_error_display(std::string info)
{
    use_bar = false;
    display = true;
    update_top_info = true;
    text = info;
}

void top_info_close()
{
    display = false;
    update_top_info = true;
}

void top_info_update()
{
    if (use_bar)
    {
        lv_info_scr_display(display);
    }
    lv_info_display(display);
    lv_info_set_text(text.c_str());
}