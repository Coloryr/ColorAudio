#include "lang.h"

const lang_t *now_lang;

const lang_t lang_ch = {
    .empty = "",
    .title = "欢迎使用ColorAudio",
    .time_text1 = "/:/",
    .time_text2 = "0:00",
    .main_text1 = "本地音乐",
    .main_text2 = "蓝牙音频",
    .main_text3 = "USB音频",
    .main_text4 = "设置",
    .main_text5 = "是否要同时关闭播放",
    .main_text6 = "正在播放本地音乐",
    .main_text7 = "正在播放本地音乐：%s",
    .main_text8 = "正在读取本地歌曲列表",
    .music_text1 = "无音乐",
    .music_text2 = "无音频",
    .music_text3 = "所有列表",
    .music_text4 = "搜索包含“%s”的结果",
    .music_text5 = "清除搜索结果",
    .music_text6 = "无歌词",
    .music_text7 = "歌词获取失败",
    .music_text8 = "正在获取歌词",
    .music_text9 = "正在扫描音乐",
    .dialog_text1 = "取消",
    .dialog_text2 = "确认",
    .ble_text1 = "启用配对",
    .ble_text2 = "已连接：%s",
    .ble_text3 = "未连接设备",
    .ble_text4 = "设备请求配对，配对码：%d",
    .usb_text1 = "设置USB音频模式",
    .usb_text2 = "启用USB音频",
    .usb_text3 = "设备模式：",
    .usb_text4 = "采样率：",
    .usb_text5 = "比特位：",
    .usb_text6 = "未连接",
    .usb_text7 = "采样率：%d 位宽：%d",
};

void lang_init()
{
    now_lang = &lang_ch;
}