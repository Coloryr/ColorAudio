#ifndef __LANG_H__
#define __LANG_H__

typedef struct
{
    const char* empty;
    const char* title;
    const char* time_text1;
    const char* time_text2;
    const char* main_text1;
    const char* main_text2;
    const char* main_text3;
    const char* main_text4;
    const char* main_text5;
    const char* main_text6;
    const char* main_text7;
    const char* main_text8;
    const char* music_text1;
    const char* music_text2;
    const char* music_text3;
    const char* music_text4;
    const char* music_text5;
    const char* music_text6;
    const char* music_text7;
    const char* music_text8;
    const char* music_text9;
    const char* music_text10;
    const char* music_text11;
    const char* dialog_text1;
    const char* dialog_text2;
    const char* ble_text1;
    const char* ble_text2;
    const char* ble_text3;
    const char* ble_text4;
    const char* usb_text1;
    const char* usb_text2;
    const char* usb_text3;
    const char* usb_text4;
    const char* usb_text5;
    const char* usb_text6;
    const char* usb_text7;
    const char* usb_text8;
    const char* usb_text9;
} lang_t;

extern const lang_t* now_lang;

#ifdef __cplusplus
extern "C" {
#endif

void lang_init();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __LANG_H__