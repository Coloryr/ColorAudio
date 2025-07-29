#include "info_view.h"

#include "view/view_top_info.h"
#include "view/view_dialog.h"

static bool display;
static bool use_bar;
static std::string text;

static bool update_top_info;

static dialog_cb dialog;

static void timer_tick(lv_timer_t *timer)
{
    if (update_top_info)
    {
        view_top_info_update();
        update_top_info = false;
    }
}

static void dialog_button_cb(lv_event_t *event)
{
    uint8_t *data = static_cast<uint8_t *>(lv_event_get_user_data(event));
    if (dialog)
    {
        dialog(*data == 1);
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

    lv_create_dialog(parent, dialog_button_cb);
}

void view_dialog_show(dialog_cb cb, const char *message)
{
    dialog = cb;
    lv_dialog_set_text(message);
    lv_dialog_display(true);
}

void view_dialog_close()
{
    lv_dialog_display(false);
}