#include "anim.h"

#include "lvgl.h"

void anim_opa_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa(var, v, 0);
}

void anim_hide_on_done_cb(lv_anim_t *a)
{
    lv_obj_t *view = a->var;
    if (view == NULL)
    {
        return;
    }
    lv_obj_add_flag(view, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_background(view);
}

void anim_set_display(lv_obj_t *obj, bool display)
{
    if (display)
    {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(obj);

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_exec_cb(&a, anim_opa_cb);
        lv_anim_set_values(&a, 0, 255);
        lv_anim_set_duration(&a, 100);
        lv_anim_set_var(&a, obj);
        lv_anim_start(&a);
    }
    else
    {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_exec_cb(&a, anim_opa_cb);
        lv_anim_set_values(&a, 255, 0);
        lv_anim_set_duration(&a, 100);
        lv_anim_set_var(&a, obj);
        lv_anim_set_completed_cb(&a, anim_hide_on_done_cb);
        lv_anim_start(&a);
    }
}