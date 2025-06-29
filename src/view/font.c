#include "font.h"
#include "lvgl.h"

const lv_font_t *font_16;
const lv_font_t *font_18;
const lv_font_t *font_22;
const lv_font_t *font_32;

static lv_font_manager_t *g_font_manager = NULL;

void load_font()
{
    /* Create font manager, with 8 fonts recycling buffers */
    g_font_manager = lv_font_manager_create(8);

    LV_LOG_USER("load font: %s", TTF_FONT);

    /* Add font path mapping to font manager */
    lv_font_manager_add_src_static(g_font_manager,
                                   "MiSans",
                                   TTF_FONT,
                                   &lv_freetype_font_class);

    /* Create font from font manager */
    font_16 = lv_font_manager_create_font(g_font_manager,
                                          "MiSans",
                                          LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                          16,
                                          LV_FREETYPE_FONT_STYLE_NORMAL,
                                          LV_FONT_KERNING_NONE);
    
    font_18 = lv_font_manager_create_font(g_font_manager,
                                          "MiSans",
                                          LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                          18,
                                          LV_FREETYPE_FONT_STYLE_NORMAL,
                                          LV_FONT_KERNING_NONE);

    font_22 = lv_font_manager_create_font(g_font_manager,
                                          "MiSans",
                                          LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                          22,
                                          LV_FREETYPE_FONT_STYLE_NORMAL,
                                          LV_FONT_KERNING_NONE);

    font_32 = lv_font_manager_create_font(g_font_manager,
                                          "MiSans",
                                          LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                          32,
                                          LV_FREETYPE_FONT_STYLE_NORMAL,
                                          LV_FONT_KERNING_NONE);
}
