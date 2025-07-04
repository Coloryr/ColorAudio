#include "view.h"

#include <stdbool.h>

bool update_info;
bool update_img;
bool update_state;
bool update_list;
bool update_list_index;

bool init_list;

bool clear_info;

bool update_top_info;

void view_update_list_index()
{
    update_list_index = true;
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