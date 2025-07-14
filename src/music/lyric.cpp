#include "lyric.h"

#include "../common/utils.h"

#include <json/json.hpp>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <stdbool.h>
#include <string>
#include <sstream>

using namespace nlohmann;

std::string trim(std::string &str)
{
    str.erase(0, str.find_first_not_of(" \t")); // 去掉头部空格
    str.erase(str.find_last_not_of(" \t") + 1); // 去掉尾部空格
    return str;
}

LyricParser::LyricParser(std::string &text)
{
    std::stringstream iss(text); // 输入流
    std::string line;
    while (std::getline(iss, line, '\n'))
    {
        if (line.empty() || line[0] == '#')
            continue;

        if (line[0] == '[')
        {
            size_t end_pos = line.find(']', 1);
            std::string time_str = line.substr(1, end_pos - 1);
            if (time_str.find(':') == std::string::npos &&
                time_str.find('.') == std::string::npos &&
                time_str.find(',') != std::string::npos)
            {
                parse_line_yrc(line, time_str);
            }
            else
            {
                parse_line(line, time_str);
            }
        }
        else if (line[0] == '{')
        {
            parse_line_json(line);
        }
    }
}

LyricParser::~LyricParser()
{
    for (const auto &part : lines)
    {
        for (const auto &part1 : part->chars)
        {
            delete part1;
        }
        part->chars.clear();
        delete part;
    }
    lines.clear();
}

void LyricParser::parse_line_json(std::string &text)
{
    json j = json::parse(text);
    json time = j["t"];
    json chars = j["c"];
    std::string text1;
    if (time.is_number() && chars.is_array())
    {
        for (const auto &pair : chars)
        {
            json text2 = pair["tx"];
            if (text2.is_string())
            {
                text1 += text2.get<std::string>();
            }
        }

        lyric_line_t *line = new lyric_line_t();
        line->time = time;
        line->text = text1;
        lines.push_back(line);
    }
}

void LyricParser::parse_line(std::string &text, std::string &time)
{
    size_t pos = 1;
    size_t end_pos = text.find(']', pos);

    auto min_pos = time.find(':');
    auto sec_pos = time.find('.');
    if (!isdigit(time[0]))
    {
        return;
    }
    uint32_t min = std::stoul(time.substr(0, min_pos));
    uint32_t sec = std::stoul(time.substr(min_pos + 1, time.length() - sec_pos));
    uint32_t ms = std::stoul(time.substr(sec_pos + 1));

    uint32_t timestamp = min * 60000 + sec * 1000 + ms;

    lyric_line_t *line = new lyric_line_t();
    line->time = timestamp;
    if (text.length() > end_pos)
    {
        line->text = text.substr(end_pos + 1);
        trim(line->text);
    }
    lines.push_back(line);
}

void LyricParser::parse_line_yrc(std::string &line, std::string &time)
{
    size_t pos = 1;
    size_t end_pos = line.find(']', pos);

    auto comma_pos = time.find(',');
    if (!isdigit(time[0]))
    {
        return;
    }
    uint32_t start = std::stoul(time.substr(0, comma_pos));
    uint32_t duration = std::stoul(time.substr(comma_pos + 1));

    lyric_line_t *new_line = new lyric_line_t();
    new_line->time = start;
    new_line->duration = duration;

    pos = end_pos + 1;
    while (pos < line.size())
    {
        if (line[pos] == '(')
        {
            size_t char_end = line.find(')', pos);
            std::string char_time = line.substr(pos + 1, char_end - pos - 1);

            std::istringstream iss(char_time);
            std::string token;
            std::vector<uint32_t> time_params;
            while (std::getline(iss, token, ','))
            {
                time_params.push_back(std::stoul(token));
            }

            if (time_params.size() >= 3)
            {
                std::string character;
                size_t next_start = line.find('(', char_end);
                if (next_start == std::string::npos)
                {
                    next_start = line.size();
                }
                uint32_t len = next_start - char_end - 1;
                if (len != 0)
                {
                    character = line.substr(char_end + 1, len);
                }

                lyric_char_t *chars = new lyric_char_t();
                chars->start = time_params[0];
                chars->duration = time_params[1];
                chars->text = character;
                new_line->chars.push_back(chars);

                pos = next_start;
            }
        }
        else
        {
            pos++;
        }
    }

    lines.push_back(new_line);
}

bool LyricParser::get_lyric(std::string &text, std::string &ktext,
                            std::string &now_ktext, float &kp, bool &have_k, uint32_t time)
{
    text.clear();
    ktext.clear();
    now_ktext.clear();
    kp = 0;
    for (uint32_t index = 0; index < lines.size(); index++)
    {
        lyric_line_t *part = lines[index];
        lyric_line_t *part1 = nullptr;
        if (index + 1 < lines.size())
        {
            part1 = lines[index + 1];
        }

        if (part->time > time)
        {
            continue;
        }

        if (part1 != nullptr && part1->time < time)
        {
            continue;
        }

        if (part->text.empty())
        {
            for (uint32_t index1 = 0; index1 < part->chars.size(); index1++)
            {
                lyric_char_t *part2 = part->chars[index1];
                lyric_char_t *part3 = nullptr;
                if (part2->start > time)
                {
                    text += part2->text;
                    continue;
                }
                if (index1 + 1 < part->chars.size())
                {
                    part3 = part->chars[index1 + 1];
                }
                if (part3 != nullptr)
                {
                    if (part2->start <= time && part3->start > time)
                    {
                        uint32_t pac = time - part2->start;
                        kp = (float)pac / part2->duration;
                        if (kp > 1)
                        {
                            kp = 1;
                        }
                        now_ktext = part2->text;
                    }
                    else
                    {
                        ktext += part2->text;
                    }
                }
                else
                {
                    now_ktext = part2->text;
                    uint32_t pac = time - part2->start;
                    kp = (float)pac / part2->duration;
                    if (kp > 1)
                    {
                        kp = 1;
                    }
                }
            }
            have_k = true;
            return true;
        }
        else
        {
            have_k = false;
            text = part->text;
            return true;
        }
    }

    return false;
}
