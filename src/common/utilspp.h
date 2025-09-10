#ifndef __UTILSPP_H__
#define __UTILSPP_H__

#include <string>

/**
 * 查找字符串是否以某个字符串开始
 * @param str 需要判断的字符串
 * @param prefix 需要查找的字符串
 * @return 判断结果
 */
bool start_with(const std::string &str, const std::string &prefix);
/**
 * 查找字符串是否以某个字符串结束
 * @param str 需要判断的字符串
 * @param prefix 需要查找的字符串
 * @return 判断结果
 */
bool end_with(const std::string &str, const std::string &suffix);
/**
 * 获取路径中的文件名
 * @param path 路径
 * @return 文件名
 */
std::string get_file_name(std::string &path);

std::string trim(std::string &str);

#endif // __UTILSPP_H__