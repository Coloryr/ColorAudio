#include "input_view.h"

#include "view/view_input.h"

#include "lvgl.h"
#include <string.h>

static input_done_cb input_call;
static char *input_done_text;

static void view_call_close()
{
    view_input_close();
}

void view_input_close()
{
    lv_input_display(false);
    if (input_done_text != NULL)
    {
        const char *text = lv_input_get_text();
        uint16_t len = strlen(text);
        memcpy(input_done_text, text, len);
        input_done_text[len] = 0;
    }
    if (input_call != NULL)
    {
        input_call(false);
    }

    lv_input_set_text("");
    input_done_text = NULL;
    input_call = NULL;
}

void view_input_show(char *input, uint16_t max_len, input_done_cb call)
{
    if (input == NULL)
    {
        return;
    }
    input_call = call;
    lv_input_set_max_size(max_len);
    input_done_text = input;
    if (strlen(input) > 0)
    {
        lv_input_set_text(input);
    }
    else
    {
        lv_input_set_text("");
    }

    lv_input_display(true);
}

void view_input_create(lv_obj_t *parent)
{
    lv_input_create(parent, view_call_close);
}