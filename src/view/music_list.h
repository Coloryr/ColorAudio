#ifndef LV_DEMO_MUSIC_LIST_H
#define LV_DEMO_MUSIC_LIST_H

/*********************
 *      INCLUDES
 *********************/
#include "view.h"
#include "local_music.h"
#include "mln_list.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct
{
    mln_list_t node;
    uint32_t index;
    lv_obj_t *view;
    lv_obj_t *title;
    lv_obj_t *auther;
    lv_obj_t *time;
} view_play_item;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_obj_t *lv_music_list_create(lv_obj_t *parent);
void lv_music_list_button_check(uint32_t track_id, bool state);

void lv_list_add_item(play_item *item);
void lv_list_button_reload(play_item *item);
void lv_list_clear();

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DEMO_MUSIC*/
