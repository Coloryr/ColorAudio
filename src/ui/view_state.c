#include "view_state.h"

#include <stdbool.h>
#include <stdint.h>

bool update_info;
bool update_img;
bool update_state;
bool update_list;

bool init_list;

bool clear_info;

bool update_top_info;

bool enable_mp4;

bool mp4_have_update;

int32_t volume_down;

void view_enable_mp4(bool enable)
{
    enable_mp4 = enable;
}

void view_mp4_update()
{
    mp4_have_update = true;
}

void view_init_list()
{
    init_list = true;
}
void view_update_list()
{
    update_list = true;
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
void view_music_clear()
{
    clear_info = true;
}
void view_top_info()
{
    update_top_info = true;
}