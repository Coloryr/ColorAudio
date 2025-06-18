#include "mp3_header.h"
#include "player.h"

#include "stream.h"

#include <stdint.h>
#include <string.h>

// MP3帧头结构体
typedef struct
{
    uint16_t sync : 11;          // 同步字 0x7FF
    uint8_t version : 2;         // MPEG版本
    uint8_t layer : 2;           // 层类型
    uint8_t protection : 1;      // CRC保护标志
    uint8_t bitrate_index : 4;   // 比特率索引
    uint8_t sample_rate_idx : 2; // 采样率索引
    uint8_t padding : 1;         // 填充位
    uint8_t private_bit : 1;     // 私有位
    uint8_t channel_mode : 2;    // 声道模式
    uint8_t mode_extension : 2;  // 扩充模式
    uint8_t copyright : 1;       // 版权
    uint8_t original : 1;        // 原版标志
    uint8_t emphasis : 2;        // 强调模式
} MP3FrameHeader;

// VBR头结构体
typedef struct
{
    char identifier[5];    // "Xing"或"Info"
    uint8_t flags;         // 存在字段标志
    uint32_t total_frames; // 总帧数
    uint32_t total_bytes;  // 文件总字节数
    uint8_t toc[100];      // TOC表
    uint32_t quality;      // 质量指示
} VBRHeader;

// 采样率表 (Hz)
static const int sample_rates[4][4] = {
    {11025, 12000, 8000, 0},  // MPEG 2.5
    {0, 0, 0, 0},             // Reserved
    {22050, 24000, 16000, 0}, // MPEG 2
    {44100, 48000, 32000, 0}  // MPEG 1
};

// MPEG版本与层对应的每帧采样数
const int samples_per_frame[4][4] = {
    /* MPEG2.5 */ {0, 576, 1152, 384},
    /* Reserved */ {0},
    /* MPEG2   */ {0, 576, 1152, 384},
    /* MPEG1   */ {0, 1152, 1152, 384}
    /*             Layer3 Layer2 Layer1   */
};

const float ms_per_frame[4][4] = {
    {8.707483f, 8.0f, 12.0f},
    {26.12245f, 24.0f, 36.0f},
    {26.12245f, 24.0f, 36.0f}};

// 比特率表 (kbps) [version][layer][bitrate_index]
const uint32_t bitrates[4][4][16] = {
    {
        // MPEG 2.5
        {0},                                                                                                             // Reserved
        {0, 8000, 16000, 24000, 32000, 40000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 144000, 160000, 0},     // Layer III
        {0, 8000, 16000, 24000, 32000, 40000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 144000, 160000, 0},     // Layer II
        {0, 32000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 144000, 160000, 176000, 192000, 224000, 256000, 0} // Layer I
    },
    {{0}, {0}, {0}, {0}},
    {
        // MPEG 2
        {0},                                                                                                             // Reserved
        {0, 8000, 16000, 24000, 32000, 40000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 144000, 160000, 0},     // Layer III
        {0, 8000, 16000, 24000, 32000, 40000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 144000, 160000, 0},     // Layer II
        {0, 32000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 144000, 160000, 176000, 192000, 224000, 256000, 0} // Layer I
    },
    {
        // MPEG 1
        {0},                                                                                                                // Reserved
        {0, 32000, 40000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 160000, 192000, 224000, 256000, 320000, 0},    // Layer III
        {0, 32000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 160000, 192000, 224000, 256000, 320000, 384000, 0},   // Layer II
        {0, 32000, 64000, 96000, 128000, 160000, 192000, 224000, 256000, 288000, 320000, 352000, 384000, 416000, 448000, 0} // Layer I
    }};

static int parse_frame_header(stream *st, MP3FrameHeader *hdr)
{
    uint8_t header_bytes[4];

    if (stream_read(st, header_bytes, 4) != 4)
    {
        return 0;
    }

    // 组合同步字 (11位)
    uint16_t sync = ((header_bytes[0] << 3) | (header_bytes[1] >> 5)) & 0x7FF;

    // 验证同步字
    if (sync != 0x7FF)
    {
        stream_seek(st, -3, SEEK_CUR); // 回退3字节
        return 0;
    }

    // 解析帧头字段
    hdr->sync = sync;
    hdr->version = (header_bytes[1] >> 3) & 0x03;
    hdr->layer = (header_bytes[1] >> 1) & 0x03;
    hdr->private_bit = header_bytes[1] & 0x01;
    hdr->bitrate_index = header_bytes[2] >> 4;
    hdr->sample_rate_idx = (header_bytes[2] >> 2) & 0x03;
    hdr->padding = (header_bytes[2] >> 1) & 0x01;
    hdr->private_bit = header_bytes[2] & 0x01;
    hdr->channel_mode = header_bytes[3] >> 6;

    // 验证有效性
    if (hdr->version == 1 || hdr->layer == 0 ||
        hdr->bitrate_index == 0 || hdr->bitrate_index == 15 ||
        hdr->sample_rate_idx == 3)
    {
        return 0;
    }

    return 1;
}

static int calculate_frame_size(MP3FrameHeader *hdr)
{
    int bitrate = bitrates[hdr->version][hdr->layer][hdr->bitrate_index];
    int sample_rate = sample_rates[hdr->version][hdr->sample_rate_idx];
    int spf = samples_per_frame[hdr->version][hdr->layer];

    if (bitrate <= 0 || sample_rate <= 0)
    {
        return 0;
    }

    // 计算帧大小 (字节)
    if (hdr->layer == 3)
    { // Layer I
        return (((12 * bitrate) / sample_rate) + hdr->padding) * 4;
    }
    else
    { // Layer II & III
        uint32_t fsize = 144 * bitrate / sample_rate;
        if (hdr->version == 0 || hdr->version == 2)
        {
            fsize /= 2;
        }
        fsize += hdr->padding;

        return fsize;
    }
}

static int parse_vbr_header(stream *st, VBRHeader *vbr, const MP3FrameHeader *hdr)
{
    int32_t start_pos = st->pos;
    uint8_t buffer[16];

    stream_read(st, buffer, 16);

    if (memcmp(buffer, "Xing", 4) != 0 && memcmp(buffer, "Info", 4) != 0)
    {
        stream_seek(st, start_pos, SEEK_SET);
        return 0;
    }

    memcpy(vbr->identifier, buffer, 4);
    vbr->identifier[4] = '\0';

    vbr->flags = buffer[7];

    if (vbr->flags & 0x01)
    {
        vbr->total_frames = (buffer[8] << 24) | (buffer[9] << 16) |
                            (buffer[10] << 8) | buffer[11];
    }

    return 1;
}

float mp3_get_time_len(stream *st)
{
    float scan_time = 0;
    uint32_t pos = st->pos;
    MP3FrameHeader hdr;

    // 尝试检测VBR头
    int is_vbr = 0;
    VBRHeader vbr = {0};

    if (parse_frame_header(st, &hdr))
    {
        // 计算第一帧大小
        int frame_size = calculate_frame_size(&hdr);
        if (frame_size <= 0)
        {
            return 0;
        }

        // 跳过帧头，检查VBR头
        if (parse_vbr_header(st, &vbr, &hdr))
        {
            // 获取采样率
            int sample_rate = sample_rates[hdr.version][hdr.sample_rate_idx];

            // 每帧采样数 (Layer III固定为1152)
            int samples_per_frame = (hdr.layer == 3) ? 1152 : 0;
            if (samples_per_frame == 0)
            {
                return 0;
            }

            // 计算时长
            return (float)vbr.total_frames * samples_per_frame / sample_rate;
        }
        else
        {
            // CBR模式处理 - 逐帧扫描
            stream_seek(st, pos, SEEK_SET);
            while (st->pos < st->size)
            {
                if (!parse_frame_header(st, &hdr))
                {
                    stream_seek(st, 1, SEEK_CUR); // 移动1字节继续搜索
                    continue;
                }

                // 计算帧大小
                int frame_size = calculate_frame_size(&hdr);
                if (frame_size <= 4)
                { // 至少大于帧头大小
                    stream_seek(st, 1, SEEK_CUR);
                    continue;
                }

                // 计算帧时长并累加
                scan_time += ms_per_frame[hdr.layer][hdr.sample_rate_idx];

                // 跳到下一帧
                stream_seek(st, frame_size - 4, SEEK_CUR);
            }
        }
    }

    return scan_time / 1000;
}