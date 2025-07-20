#include "main_view.h"

#include "view/view_main.h"

#include "lvgl.h"

static lv_obj_t * main_view;

lv_obj_t *lv_main_view_create(lv_obj_t *parent)
{
    main_view = lv_main_create(parent);

    return main_view;
}