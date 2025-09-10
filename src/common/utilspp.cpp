#include "utilspp.h"

#include <string>

bool start_with(const std::string &str, const std::string &prefix)
{
    return (str.rfind(prefix, 0) == 0);
}

bool end_with(const std::string &str, const std::string &suffix)
{
    if (suffix.length() > str.length())
    {
        return false;
    }

    return (str.rfind(suffix) == (str.length() - suffix.length()));
}

std::string get_file_name(std::string &path)
{
    int len = path.length();
    int i;
    for (i = (len - 1); i >= 0; i--)
    {
        if ((path[i] == '\\') || (path[i] == '/'))
        {
            break;
        }
    }
    return path.substr(i + 1);
}

std::string trim(std::string &str)
{
    str.erase(0, str.find_first_not_of(" \t")); // 去掉头部空格
    str.erase(str.find_last_not_of(" \t") + 1); // 去掉尾部空格
    return str;
}