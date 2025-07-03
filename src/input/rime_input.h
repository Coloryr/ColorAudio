#ifndef _LIBRIME_INPUT_H_
#define _LIBRIME_INPUT_H_

#include <rime_api.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 输入法初始化
 */
void rime_init();
/**
 * 开始输入文字
 */
void rime_start_session();
/**
 * 输入按键
 */
bool rime_put_key(uint8_t *key);
/**
 * 获取结果
 * @param commit 返回结果
 * @return 是否获取成功
 */
bool rime_get_commit(RimeCommit *commit);
/**
 * 清理结果
 * @param commit 结果
 */
void rime_close_commit(RimeCommit *commit);
/**
 * 获取状态
 * @param status 返回状态
 * @return 是否获取成功
 */
bool rime_get_status(RimeStatus *status);
/**
 * 清理状态
 * @param status 返回状态
 */
void rime_close_status(RimeStatus *status);
/**
 * 获取待输入列表
 * @param context 选择列表
 * @return 是否获取成功
 */
bool rime_get_context(RimeContext *context);
/**
 * 关闭输入列表
 * @param context 选择列表
 */
void rime_close_context(RimeContext *context);
/**
 * 设置选择
 * @param index 选择的项目
 */
void rime_highlight_candidate(int index);
/**
 * 关闭输入
 */
void rime_close_session();
/**
 * 换页
 * @param backward 是否向后翻页
 * @return 是否翻页成功
 */
bool rime_change_page(bool backward);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif