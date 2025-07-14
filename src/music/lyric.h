#ifndef _LYRIC_H_
#define _LYRIC_H_

#include <stdint.h>
#include <stdbool.h>
#include <string>
#include <vector>

typedef struct
{
    uint32_t start;
    uint32_t duration;
    std::string text;
} lyric_char_t;

typedef struct
{
    uint32_t time;
    uint32_t duration;
    std::string text;
    std::vector<lyric_char_t *> chars;
} lyric_line_t;

class LyricParser
{
private:
    std::vector<lyric_line_t *> lines;

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