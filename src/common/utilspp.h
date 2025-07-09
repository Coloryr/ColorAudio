#ifndef __UTILSPP_H__
#define __UTILSPP_H__

#include <string>

bool startsWith(const std::string &str, const std::string prefix);
bool endsWith(const std::string &str, const std::string suffix);
void getfilename(std::string &filename, std::string &name);

#endif // __UTILSPP_H__