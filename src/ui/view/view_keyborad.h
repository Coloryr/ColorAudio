#ifndef LV_KEYBOARD_VIEW_H
#define LV_KEYBOARD_VIEW_H

/*********************
 *      INCLUDES
 *********************/
#include "../lvgl/src/widgets/buttonmatrix/lv_buttonmatrix.h"
#include "../lvgl/src/widgets/buttonmatrix/lv_buttonmatrix_private.h"
#include "../lvgl/src/widgets/keyboard/lv_keyboard.h"

/*********************
 *      DEFINES
 *********************/
#define LV_KEYBOARD_CTRL_BUTTON_FLAGS (LV_BUTTONMATRIX_CTRL_NO_REPEAT | LV_BUTTONMATRIX_CTRL_CLICK_TRIG | LV_BUTTONMATRIX_CTRL_CHECKED)

typedef void (*lv_keyboard_input_t)(uint8_t *input);
typedef struct _lv_keyboard_view_t lv_keyboard_view_t;

/** Data of keyboard */
struct _lv_keyboard_view_t
{
    lv_buttonmatrix_t btnm;
    lv_keyboard_input_t input;   /**< Pointer to the assigned text input */
    lv_keyboard_mode_t mode; /**< Key map type */
    uint8_t popovers : 1;         /**< Show button titles in popovers on press */
};

LV_ATTRIBUTE_EXTERN_DATA extern const lv_obj_class_t lv_keyboard_view_class;

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *lv_keyboard_view_create(lv_obj_t *parent);
void lv_keyboard_view_set_input(lv_obj_t *obj, lv_keyboard_input_t input);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_KEYBOARD_H*/
