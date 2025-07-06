#include "mp4.h"

#include "../common/data_item.h"
#include "../stream/stream_mem.h"

#include "lvgl.h"
#include <minimp4/minimp4.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <vector>
#include <memory>
#include <wels/codec_api.h>
#include <algorithm>

using namespace ColorAudio;

static lv_image_dsc_t video_dsc;

// ===== 1. MP4文件结构解析 =====
class MP4Parser
{
public:
    struct BoxHeader
    {
        uint32_t size;
        char type[4];
    };

    struct TrackInfo
    {
        std::vector<uint32_t> sampleSizes;  // stsz box
        std::vector<uint32_t> chunkOffsets; // stco box
    };

    static bool Parse(StreamMemory *st, TrackInfo &videoTrack)
    {
        while (st->can_read())
        {
            BoxHeader header;
            if (st->read(reinterpret_cast<uint8_t *>(&header), sizeof(BoxHeader)) < sizeof(BoxHeader))
                break;

            header.size = __builtin_bswap32(header.size); // 大端转小端

            // 关键box处理
            if (memcmp(header.type, "moov", 4) == 0)
            {
                ParseMoov(st, header.size - 8, videoTrack);
            }
            else if (memcmp(header.type, "mdat", 4) == 0)
            {
                mdatStart = st->get_pos();
                st->seek(header.size - 8, SEEK_CUR); // 跳过mdat内容
            }
            else
            {
                st->seek(header.size - 8, SEEK_CUR); // 跳过无关box
            }
        }
        return true;
    }

private:
    static void ParseMoov(StreamMemory *st, uint32_t size, TrackInfo &track)
    {
        uint32_t parsed = 0;
        while (parsed < size)
        {
            BoxHeader header;
            st->read(reinterpret_cast<uint8_t *>(&header), sizeof(BoxHeader));
            header.size = __builtin_bswap32(header.size);

            if (memcmp(header.type, "trak", 4) == 0)
            {
                ParseTrak(st, header.size - 8, track);
            }
            else
            {
                st->seek(header.size - 8, SEEK_CUR);
            }
            parsed += header.size;
        }
    }

    static void ParseTrak(StreamMemory *st, uint32_t size, TrackInfo &track)
    {
        uint32_t parsed = 0;
        while (parsed < size)
        {
            BoxHeader header;
            st->read(reinterpret_cast<uint8_t *>(&header), sizeof(BoxHeader));
            header.size = __builtin_bswap32(header.size);

            if (memcmp(header.type, "stbl", 4) == 0)
            {
                ParseStbl(st, header.size - 8, track);
            }
            else
            {
                st->seek(header.size - 8, SEEK_CUR);
            }
            parsed += header.size;
        }
    }

    static void ParseStbl(StreamMemory *st, uint32_t size, TrackInfo &track)
    {
        uint32_t parsed = 0;
        while (parsed < size)
        {
            BoxHeader header;
            st->read(reinterpret_cast<uint8_t *>(&header), sizeof(BoxHeader));
            header.size = __builtin_bswap32(header.size);

            if (memcmp(header.type, "stsz", 4) == 0)
            {                          // 样本大小表
                st->seek(4, SEEK_CUR); // 跳过version+flags
                uint32_t sampleCount;
                st->read(reinterpret_cast<uint8_t *>(&sampleCount), 4);
                sampleCount = __builtin_bswap32(sampleCount);
                track.sampleSizes.resize(sampleCount);
                st->read(reinterpret_cast<uint8_t *>(track.sampleSizes.data()), sampleCount * 4);
            }
            else if (memcmp(header.type, "stco", 4) == 0)
            {                          // 块偏移表
                st->seek(4, SEEK_CUR); // 跳过version+flags
                uint32_t chunkCount;
                st->read(reinterpret_cast<uint8_t *>(&chunkCount), 4);
                chunkCount = __builtin_bswap32(chunkCount);
                track.chunkOffsets.resize(chunkCount);
                st->read(reinterpret_cast<uint8_t *>(track.chunkOffsets.data()), chunkCount * 4);
            }
            else
            {
                st->seek(header.size - 8, SEEK_CUR);
            }
            parsed += header.size;
        }
    }

public:
    static int64_t mdatStart; // mdat数据起始位置
};
int64_t MP4Parser::mdatStart = 0;

// ===== 2. H264 NAL单元提取 =====
class H264Extractor
{
public:
    static std::vector<uint8_t> GetNALU(StreamMemory *st, uint32_t size)
    {
        std::vector<uint8_t> sample(size);
        st->read(reinterpret_cast<uint8_t *>(sample.data()), size);

        // 处理竞争机制：删除0x000003中的0x03
        for (auto it = sample.begin(); it != sample.end() - 3;)
        {
            if (*it == 0x00 && *(it + 1) == 0x00 && *(it + 2) == 0x03)
            {
                it = sample.erase(it + 2);
            }
            else
            {
                ++it;
            }
        }
        return sample;
    }
};

// ===== 3. OpenH264解码器封装 =====
class H264Decoder
{
public:
    H264Decoder()
    {
        WelsCreateDecoder(&decoder_);
        SDecodingParam decParam = {0};
        decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
        decoder_->Initialize(&decParam);
    }

    ~H264Decoder()
    {
        decoder_->Uninitialize();
    }

    bool Decode(const uint8_t *data, size_t size, std::vector<uint8_t> &outYUV)
    {
        SBufferInfo bufferInfo;
        DECODING_STATE state = decoder_->DecodeFrame2(data, size, &vcm, &bufferInfo);
        if (state != dsErrorFree)
            return false;

        // 提取YUV数据
        outYUV.resize(bufferInfo.UsrData.sSystemBuffer.iWidth *
                      bufferInfo.UsrData.sSystemBuffer.iHeight * 3 / 2);
        uint8_t *dst = outYUV.data();
        for (int i = 0; i < 3; ++i)
        {
            uint8_t *src = bufferInfo.pDst[i];
            int stride = bufferInfo.UsrData.sSystemBuffer.iStride[i];
            int height = (i == 0) ? bufferInfo.UsrData.sSystemBuffer.iHeight
                                  : bufferInfo.UsrData.sSystemBuffer.iHeight / 2;
            for (int h = 0; h < height; ++h)
            {
                memcpy(dst, src, stride);
                dst += stride;
                src += stride;
            }
        }
        return true;
    }

private:
    ISVCDecoder *decoder_ = nullptr;
    uint8_t *vcm;
};

// ===== 4. YUV转RGB转换器 =====
class YUVtoRGBConverter
{
public:
    static void Convert(const uint8_t *yuv, uint8_t *rgb, int width, int height)
    {
        const int uvOffset = width * height;
        const uint8_t *yPtr = yuv;
        const uint8_t *uPtr = yuv + uvOffset;
        const uint8_t *vPtr = yuv + uvOffset + (uvOffset / 4);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int Y = yPtr[y * width + x] - 16;
                int U = uPtr[(y / 2) * (width / 2) + (x / 2)] - 128;
                int V = vPtr[(y / 2) * (width / 2) + (x / 2)] - 128;

                // BT.601标准转换公式
                int R = (298 * Y + 409 * V + 128) >> 8;
                int G = (298 * Y - 100 * U - 208 * V + 128) >> 8;
                int B = (298 * Y + 516 * U + 128) >> 8;

                // 裁剪到[0,255]范围
                rgb[(y * width + x) * 3 + 0] = std::clamp(R, 0, 255);
                rgb[(y * width + x) * 3 + 1] = std::clamp(G, 0, 255);
                rgb[(y * width + x) * 3 + 2] = std::clamp(B, 0, 255);
            }
        }
    }
};

void load_mp4(data_item *item)
{
    // StreamMemory st = StreamMemory(item->data, item->size);
    // MP4Parser::TrackInfo videoTrack;
    // if (!MP4Parser::Parse(&st, videoTrack))
    // {
    //     LV_LOG_ERROR("MP4解析失败");
    //     return;
    // }

    // // 步骤2：打开文件并定位到mdat
    // st.seek(MP4Parser::mdatStart, SEEK_SET);

    // // 步骤3：初始化解码器
    // H264Decoder decoder;
    // std::vector<uint8_t> rgbBuffer;

    // // 步骤4：逐帧解码
    // for (size_t i = 0; i < videoTrack.sampleSizes.size(); ++i)
    // {
    //     // 提取H264样本
    //     auto sample = H264Extractor::GetNALU(&st, videoTrack.sampleSizes[i]);

    //     // 解码为YUV
    //     std::vector<uint8_t> yuvData;
    //     if (!decoder.Decode(sample.data(), sample.size(), yuvData))
    //     {
    //         LV_LOG_ERROR("帧%ld解码失败", i);
    //         continue;
    //     }

    //     // 转换为RGB
    //     rgbBuffer.resize(yuvData.size() * 2); // RGB大小=宽×高×3
    //     YUVtoRGBConverter::Convert(yuvData.data(), rgbBuffer.data(), 1920, 1080);

    //     // 此处可添加RGB数据输出到屏幕/文件
    //     // 例如使用OpenGL或SDL显示
    // }
}