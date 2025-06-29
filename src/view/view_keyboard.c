/*********************
 *      INCLUDES
 *********************/
#include "view_keyborad.h"
#include "lvgl/src/core/lv_obj_class_private.h"

#include "lvgl/src/widgets/textarea/lv_textarea.h"
#include "lvgl/src/misc/lv_assert.h"
#include "lvgl/src/stdlib/lv_string.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS (&lv_keyboard_view_class)
#define LV_KB_BTN(width) LV_BUTTONMATRIX_CTRL_POPOVER | width

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_keyboard_view_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);

static void lv_keyboard_view_update_map(lv_obj_t *obj);

static void lv_keyboard_view_update_ctrl_map(lv_obj_t *obj);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_keyboard_view_class = {
    .constructor_cb = lv_keyboard_view_constructor,
    .width_def = LV_PCT(100),
    .height_def = LV_PCT(30),
    .instance_size = sizeof(lv_keyboard_view_t),
    .editable = 1,
    .base_class = &lv_buttonmatrix_class,
    .name = "lv_keyboard_view"};

static const char *const default_kb_view_map_lc[] = {"1#", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", LV_SYMBOL_BACKSPACE, "\n",
                                                     "ABC", "a", "s", "d", "f", "g", "h", "j", "k", "l", LV_SYMBOL_NEW_LINE, "\n",
                                                     "_", "-", "z", "x", "c", "v", "b", "n", "m", ".", ",", ":", "\n",
                                                     LV_SYMBOL_KEYBOARD,
                                                     LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""};

static const lv_buttonmatrix_ctrl_t default_kb_view_ctrl_lc_map[] = {
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 5, LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_BUTTONMATRIX_CTRL_CHECKED | 7,
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 6, LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_BUTTONMATRIX_CTRL_CHECKED | 7,
    LV_BUTTONMATRIX_CTRL_CHECKED | LV_KB_BTN(1), LV_BUTTONMATRIX_CTRL_CHECKED | LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_BUTTONMATRIX_CTRL_CHECKED | LV_KB_BTN(1), LV_BUTTONMATRIX_CTRL_CHECKED | LV_KB_BTN(1), LV_BUTTONMATRIX_CTRL_CHECKED | LV_KB_BTN(1),
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2,
    LV_BUTTONMATRIX_CTRL_CHECKED | 2, 6, LV_BUTTONMATRIX_CTRL_CHECKED | 2, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2};

static const char *const default_kb_view_map_uc[] = {"1#", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", LV_SYMBOL_BACKSPACE, "\n",
                                                     "abc", "A", "S", "D", "F", "G", "H", "J", "K", "L", LV_SYMBOL_NEW_LINE, "\n",
                                                     "_", "-", "Z", "X", "C", "V", "B", "N", "M", ".", ",", ":", "\n",
                                                     LV_SYMBOL_KEYBOARD,
                                                     LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""};

static const lv_buttonmatrix_ctrl_t default_kb_view_ctrl_uc_map[] = {
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 5, LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_BUTTONMATRIX_CTRL_CHECKED | 7,
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 6, LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_BUTTONMATRIX_CTRL_CHECKED | 7,
    LV_BUTTONMATRIX_CTRL_CHECKED | LV_KB_BTN(1), LV_BUTTONMATRIX_CTRL_CHECKED | LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_BUTTONMATRIX_CTRL_CHECKED | LV_KB_BTN(1), LV_BUTTONMATRIX_CTRL_CHECKED | LV_KB_BTN(1), LV_BUTTONMATRIX_CTRL_CHECKED | LV_KB_BTN(1),
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2,
    LV_BUTTONMATRIX_CTRL_CHECKED | 2, 6, LV_BUTTONMATRIX_CTRL_CHECKED | 2, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2};

static const char *const default_kb_view_map_spec[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
                                                       "abc", "+", "&", "/", "*", "=", "%", "!", "?", "#", "<", ">", "\n",
                                                       "\\", "@", "$", "(", ")", "{", "}", "[", "]", ";", "\"", "'", "\n",
                                                       LV_SYMBOL_KEYBOARD,
                                                       LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""};

static const lv_buttonmatrix_ctrl_t default_kb_view_ctrl_spec_map[] = {
    LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_BUTTONMATRIX_CTRL_CHECKED | 2,
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2, LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1),
    LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1),
    LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2,
    LV_BUTTONMATRIX_CTRL_CHECKED | 2, 6, LV_BUTTONMATRIX_CTRL_CHECKED | 2, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2};

static const char *const default_kb_view_map_num[] = {"1", "2", "3", LV_SYMBOL_KEYBOARD, "\n",
                                                      "4", "5", "6", LV_SYMBOL_OK, "\n",
                                                      "7", "8", "9", LV_SYMBOL_BACKSPACE, "\n",
                                                      "+/-", "0", ".", LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""};

static const lv_buttonmatrix_ctrl_t default_kb_view_ctrl_num_map[] = {
    1, 1, 1, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2,
    1, 1, 1, LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2,
    1, 1, 1, 2,
    1, 1, 1, 1, 1};

static const char *const *kb_view_map[5] = {
    default_kb_view_map_lc,
    default_kb_view_map_uc,
    default_kb_view_map_spec,
    default_kb_view_map_num,
    NULL};
static const lv_buttonmatrix_ctrl_t *kb_view_ctrl[5] = {
    default_kb_view_ctrl_lc_map,
    default_kb_view_ctrl_uc_map,
    default_kb_view_ctrl_spec_map,
    default_kb_view_ctrl_num_map,
    NULL};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_keyboard_view_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(&lv_keyboard_view_class, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_keyboard_view_set_input(lv_obj_t *obj, lv_keyboard_input_t input)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_keyboard_view_t *keyboard = (lv_keyboard_view_t *)obj;
    keyboard->input = input;
}

void lv_keyboard_view_set_mode(lv_obj_t *obj, lv_keyboard_mode_t mode)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_keyboard_view_t *keyboard = (lv_keyboard_view_t *)obj;
    if (keyboard->mode == mode)
        return;

    keyboard->mode = mode;
    lv_keyboard_view_update_map(obj);
}

void lv_keyboard_view_set_map(lv_obj_t *obj, lv_keyboard_mode_t mode, const char *const map[],
                              const lv_buttonmatrix_ctrl_t ctrl_map[])
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    kb_view_map[mode] = map;
    kb_view_ctrl[mode] = ctrl_map;
    lv_keyboard_view_update_map(obj);
}

/*=====================
 * Getter functions
 *====================*/

lv_keyboard_mode_t lv_keyboard_view_get_mode(const lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_keyboard_view_t *keyboard = (lv_keyboard_view_t *)obj;
    return keyboard->mode;
}

bool lv_keyboard_view_get_popovers(const lv_obj_t *obj)
{
    lv_keyboard_view_t *keyboard = (lv_keyboard_view_t *)obj;
    return keyboard->popovers;
}

/*=====================
 * Other functions
 *====================*/

void lv_keyboard_view_def_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_current_target(e);

    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_keyboard_view_t *keyboard = (lv_keyboard_view_t *)obj;
    uint32_t btn_id = lv_buttonmatrix_get_selected_button(obj);
    if (btn_id == LV_BUTTONMATRIX_BUTTON_NONE)
        return;

    const char *txt = lv_buttonmatrix_get_button_text(obj, btn_id);
    if (txt == NULL)
        return;

    if (lv_strcmp(txt, "abc") == 0)
    {
        keyboard->mode = LV_KEYBOARD_MODE_TEXT_LOWER;
        lv_buttonmatrix_set_map(obj, kb_view_map[LV_KEYBOARD_MODE_TEXT_LOWER]);
        lv_keyboard_view_update_ctrl_map(obj);
        return;
    }
    else if (lv_strcmp(txt, "ABC") == 0)
    {
        keyboard->mode = LV_KEYBOARD_MODE_TEXT_UPPER;
        lv_buttonmatrix_set_map(obj, kb_view_map[LV_KEYBOARD_MODE_TEXT_UPPER]);
        lv_keyboard_view_update_ctrl_map(obj);
        return;
    }
    else if (lv_strcmp(txt, "1#") == 0)
    {
        keyboard->mode = LV_KEYBOARD_MODE_SPECIAL;
        lv_buttonmatrix_set_map(obj, kb_view_map[LV_KEYBOARD_MODE_SPECIAL]);
        lv_keyboard_view_update_ctrl_map(obj);
        return;
    }
    else if (lv_strcmp(txt, LV_SYMBOL_CLOSE) == 0 || lv_strcmp(txt, LV_SYMBOL_KEYBOARD) == 0)
    {
        lv_obj_send_event(obj, LV_EVENT_CANCEL, NULL);
        return;
    }
    else if (lv_strcmp(txt, LV_SYMBOL_OK) == 0)
    {
        lv_obj_send_event(obj, LV_EVENT_READY, NULL);
        return;
    }

    /*Add the characters to the text area if set*/
    if (keyboard->input == NULL)
        return;

    keyboard->input((uint8_t *)txt);
}

const char *const *lv_keyboard_view_get_map_array(const lv_obj_t *kb)
{
    return lv_buttonmatrix_get_map(kb);
}

uint32_t lv_keyboard_view_get_selected_button(const lv_obj_t *obj)
{
    return lv_buttonmatrix_get_selected_button(obj);
}

const char *lv_keyboard_view_get_button_text(const lv_obj_t *obj, uint32_t btn_id)
{
    return lv_buttonmatrix_get_button_text(obj, btn_id);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_keyboard_view_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    lv_keyboard_view_t *keyboard = (lv_keyboard_view_t *)obj;
    keyboard->input = NULL;
    keyboard->mode = LV_KEYBOARD_MODE_TEXT_LOWER;
    keyboard->popovers = 0;

    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(obj, lv_keyboard_view_def_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_style_base_dir(obj, LV_BASE_DIR_LTR, 0);

    lv_keyboard_view_update_map(obj);
}

/**
 * Update the key and control map for the current mode
 * @param obj pointer to a keyboard object
 */
static void lv_keyboard_view_update_map(lv_obj_t *obj)
{
    lv_keyboard_view_t *keyboard = (lv_keyboard_view_t *)obj;
    lv_buttonmatrix_set_map(obj, kb_view_map[keyboard->mode]);
    lv_keyboard_view_update_ctrl_map(obj);
}

/**
 * Update the control map for the current mode
 * @param obj pointer to a keyboard object
 */
static void lv_keyboard_view_update_ctrl_map(lv_obj_t *obj)
{
    lv_keyboard_view_t *keyboard = (lv_keyboard_view_t *)obj;

    if (keyboard->popovers)
    {
        /*Apply the current control map (already includes LV_BUTTONMATRIX_CTRL_POPOVER flags)*/
        lv_buttonmatrix_set_ctrl_map(obj, kb_view_ctrl[keyboard->mode]);
    }
    else
    {
        /*Make a copy of the current control map*/
        lv_buttonmatrix_t *btnm = (lv_buttonmatrix_t *)obj;
        lv_buttonmatrix_ctrl_t *ctrl_map = lv_malloc(btnm->btn_cnt * sizeof(lv_buttonmatrix_ctrl_t));
        lv_memcpy(ctrl_map, kb_view_ctrl[keyboard->mode], sizeof(lv_buttonmatrix_ctrl_t) * btnm->btn_cnt);

        /*Remove all LV_BUTTONMATRIX_CTRL_POPOVER flags*/
        uint32_t i;
        for (i = 0; i < btnm->btn_cnt; i++)
        {
            ctrl_map[i] &= (~LV_BUTTONMATRIX_CTRL_POPOVER);
        }

        /*Apply new control map and clean up*/
        lv_buttonmatrix_set_ctrl_map(obj, ctrl_map);
        lv_free(ctrl_map);
    }
}
