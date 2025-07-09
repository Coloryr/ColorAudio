#ifndef _LYRIC_H_
#define _LYRIC_H_

#include <stdint.h>
#include <stdbool.h>
#include <string>
#include <vector>

struct LyricChar
{
    uint32_t start;
    uint32_t duration;
    std::string text;
};

struct LyricLine
{
    uint32_t time;
    uint32_t duration;
    std::string text;
    std::vector<LyricChar *> chars;
};

class LyricParser
{
private:
    std::vector<LyricLine *> lines; // 歌词行集合

    void parse_line(std::string &text, std::string &time);
    void parse_line_json(std::string &text);
    void parse_line_yrc(std::string &text, std::string &time);

public:
    LyricParser(std::string &text);
    ~LyricParser();

    bool get_lyric(std::string &text, std::string &ktext,
                   std::string &now_ktext, float &kp, bool &have_k, uint32_t time);
};

#endif