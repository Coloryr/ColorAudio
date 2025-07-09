#include "utilspp.h"

#include <string>
#include <cstring>

bool startsWith(const std::string &str, const std::string prefix)
{
    return (str.rfind(prefix, 0) == 0);
}

bool endsWith(const std::string &str, const std::string suffix)
{
    if (suffix.length() > str.length())
    {
        return false;
    }

    return (str.rfind(suffix) == (str.length() - suffix.length()));
}

void getfilename(std::string &filename, std::string &name)
{
    int len = filename.length();
    int i;
    for (i = (len - 1); i >= 0; i--)
    {
        if ((filename[i] == '\\') || (filename[i] == '/'))
        {
            break;
        }
    }
    name = filename.substr(i + 1);
}