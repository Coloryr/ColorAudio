#include "main_view.h"

#include "view/view_main.h"

#include "lvgl.h"

static lv_obj_t * main_view;

void view_main_create(lv_obj_t *parent)
{
    main_view = lv_main_create(parent);
}