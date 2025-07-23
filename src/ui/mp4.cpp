#include "mp4.h"

#include "../common/data_item.h"
#include "../stream/stream_mem.h"

#include "ui.h"
#include "music_view.h"

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
#include <unistd.h>

using namespace ColorAudio;

static lv_image_dsc_t video_dsc;

class H264Decoder
{
public:
    H264Decoder()
    {
        WelsCreateDecoder(&decoder);
        decoder->Initialize(&decParam);
    }

    ~H264Decoder()
    {
        decoder->Uninitialize();
    }

    bool Decode(const uint8_t *data, size_t size, SBufferInfo *bufferInfo)
    {
        DECODING_STATE state = decoder->DecodeFrame2(data, size, &vcm, bufferInfo);
        if (state != dsErrorFree)
            return false;

        return bufferInfo->iBufferStatus == 1;
    }

private:
    SDecodingParam decParam = {0};
    ISVCDecoder *decoder = nullptr;
    uint8_t *vcm;
};

class YUVtoRGBConverter
{
public:
    static void Convert(SBufferInfo *bufferInfo, uint8_t *rgb)
    {
        int width = bufferInfo->UsrData.sSystemBuffer.iWidth;
        int height = bufferInfo->UsrData.sSystemBuffer.iHeight;
        int yStride = bufferInfo->UsrData.sSystemBuffer.iStride[0];
        int uvStride = bufferInfo->UsrData.sSystemBuffer.iStride[1];
        uint8_t *yPlane = bufferInfo->pDst[0];
        uint8_t *uPlane = bufferInfo->pDst[1];
        uint8_t *vPlane = bufferInfo->pDst[2];

        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int Y = yPlane[y * yStride + x];
                int U = uPlane[(y / 2) * uvStride + (x / 2)];
                int V = vPlane[(y / 2) * uvStride + (x / 2)];

                int R = Y + 1.402 * (V - 128);
                int G = Y - 0.344 * (U - 128) - 0.714 * (V - 128);
                int B = Y + 1.772 * (U - 128);

                int idx = (y * width + x) * 3;
                rgb[idx + 0] = std::clamp(B, 0, 255); // R
                rgb[idx + 1] = std::clamp(G, 0, 255); // G
                rgb[idx + 2] = std::clamp(R, 0, 255); // B
            }
        }
    }
};

static int mp4_read_cb(int64_t offset, void *buffer, size_t size, void *token)
{
    StreamMemory *st = static_cast<StreamMemory *>(token);
    st->seek(offset, SEEK_SET);
    return st->read(static_cast<uint8_t *>(buffer), size) == 0;
}

void load_mp4(data_item *item)
{
    StreamMemory st = StreamMemory(item);
    H264Decoder dec = H264Decoder();
    YUVtoRGBConverter cov = YUVtoRGBConverter();
    MP4D_demux_t demuxer = {0};
    uint8_t *rgb;
    uint32_t bytes;
    uint32_t time;
    uint32_t dur;
    MP4D_track_t *track;
    uint8_t *temp = static_cast<uint8_t *>(malloc(8192));
    uint32_t temp_size = 8192;
    uint32_t i = 0;
    if (MP4D_open(&demuxer, mp4_read_cb, &st, item->size))
    {
        if (demuxer.track_count < 1)
        {
            return;
        }
        for (i = 0; i < demuxer.track_count; i++)
        {
            track = &demuxer.track[0];
            if (track->handler_type != MP4D_HANDLER_TYPE_VIDE)
            {
                track = nullptr;
            }
        }
        if (track == nullptr)
        {
            return;
        }
        uint8_t sync[4] = {0, 0, 0, 1};
        const void *spspps;
        int32_t spspps_bytes;
        i = 0;
        SBufferInfo out;
        while (spspps = MP4D_read_sps(&demuxer, 0, i, &spspps_bytes))
        {
            if (spspps_bytes + 4 > temp_size)
            {
                temp_size = spspps_bytes + 4;
                temp = static_cast<uint8_t *>(realloc(temp, temp_size));
            }
            memcpy(temp, sync, 4);
            memcpy(temp + 4, spspps, spspps_bytes);
            dec.Decode(temp, spspps_bytes + 4, &out);
            i++;
        }
        i = 0;
        while (spspps = MP4D_read_pps(&demuxer, 0, i, &spspps_bytes))
        {
            if (spspps_bytes + 4 > temp_size)
            {
                temp_size = spspps_bytes + 4;
                temp = static_cast<uint8_t *>(realloc(temp, temp_size));
            }
            memcpy(temp, sync, 4);
            memcpy(temp + 4, spspps, spspps_bytes);
            dec.Decode(temp, spspps_bytes + 4, &out);
            i++;
        }

        uint32_t frame = 0;
        rgb = static_cast<uint8_t *>(malloc(track->SampleDescription.video.width * track->SampleDescription.video.height * 3));
        view_music_set_image_data(track->SampleDescription.video.width, track->SampleDescription.video.height, rgb);
        for (uint32_t i = 0; i < track->sample_count; i++)
        {
            unsigned frame_bytes, timestamp, duration;
            MP4D_file_offset_t ofs = MP4D_frame_offset(&demuxer, 0, i, &frame_bytes, &timestamp, &duration);
            st.seek(ofs, SEEK_SET);
            while (frame_bytes)
            {
                st.read(temp, 4);
                uint32_t size = ((uint32_t)temp[0] << 24) | ((uint32_t)temp[1] << 16) | ((uint32_t)temp[2] << 8) | temp[3];
                size += 4;
                if (size > temp_size)
                {
                    temp_size = size;
                    temp = static_cast<uint8_t *>(realloc(temp, temp_size));
                }

                memcpy(temp, sync, 4);
                st.read(temp + 4, size - 4);

                if (dec.Decode(temp, size, &out))
                {
                    cov.Convert(&out, rgb);
                    view_music_mp4_update();
                    usleep(300000);
                    LV_LOG_USER("decoder frame: %d", frame++);
                }

                if (frame_bytes < size)
                {
                    printf("error: demux sample failed\n");
                    exit(1);
                }
                frame_bytes -= size;
            }
        }
    }
    MP4D_close(&demuxer);
}