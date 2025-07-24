#include "flac_metadata.h"

#include "../music_player.h"
#include "../sound/sound.h"
#include "../sound/sound_fft.h"
#include "../common/data_item.h"

#include "../lvgl/src/misc/lv_log.h"

#include <string.h>
#include <unistd.h>
#include <string>

using namespace ColorAudio;

FlacMetadata::FlacMetadata(ColorAudio::Stream *st)
    : st(st)
{
    set_md5_checking(true);
    set_metadata_respond_all();

    info.image = NULL;
}

FlacMetadata::~FlacMetadata()
{
    finish();

    if (info.image != nullptr)
    {
        delete info.image;
        info.image = nullptr;
    }
}

FLAC__StreamDecoderReadStatus FlacMetadata::read_callback(FLAC__byte buffer[], size_t *bytes)
{
    if (st->can_read() == false)
    {
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }
    size_t n = st->read(buffer, *bytes);
    if (n == 0)
    {
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    }
    else
    {
        *bytes = n;
        return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    }
}

FLAC__StreamDecoderSeekStatus FlacMetadata::seek_callback(FLAC__uint64 absolute_byte_offset)
{
    st->seek((off_t)absolute_byte_offset, SEEK_SET);
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus FlacMetadata::tell_callback(FLAC__uint64 *absolute_byte_offset)
{
    *absolute_byte_offset = (FLAC__uint64)st->get_pos();

    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus FlacMetadata::length_callback(FLAC__uint64 *stream_length)
{
    *stream_length = (FLAC__uint64)st->get_all_size();
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

bool FlacMetadata::eof_callback()
{
    return st->can_read() == false;
}

FLAC__StreamDecoderWriteStatus FlacMetadata::write_callback(const ::FLAC__Frame *frame, const FLAC__int32 *const buffer[])
{
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FlacMetadata::metadata_callback(const ::FLAC__StreamMetadata *metadata)
{
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
    {
        uint32_t bps = metadata->data.stream_info.bits_per_sample;
        uint32_t rate = metadata->data.stream_info.sample_rate;
        uint32_t channel = metadata->data.stream_info.channels;

        info.time = (float)metadata->data.stream_info.total_samples / metadata->data.stream_info.sample_rate;
    }
    else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
    {
        for (size_t i = 0; i < metadata->data.vorbis_comment.num_comments; i++)
        {
            char key[128];

            FLAC__StreamMetadata_VorbisComment_Entry *enety = &metadata->data.vorbis_comment.comments[i];

            uint32_t len = enety->length;

            char *save = reinterpret_cast<char *>(enety->entry);

            size_t eq_pos = strcspn(save, "=");
            if (eq_pos < len)
            {
                if (eq_pos >= sizeof(key))
                {
                    continue;
                }
                strncpy(key, reinterpret_cast<char *>(enety->entry), eq_pos);
                key[eq_pos] = '\0';
            }

            save = reinterpret_cast<char *>(enety->entry) + eq_pos + 1;
            len -= eq_pos + 1;
            if (len <= 0)
            {
                continue;
            }
            if (strcasecmp(key, "title") == 0)
            {
                info.title = save;
            }
            else if (strcasecmp(key, "artist") == 0)
            {
                info.auther = save;
            }
            else if (strcasecmp(key, "album") == 0)
            {
                info.album = save;
            }
            else if (strcasecmp(key, "DESCRIPTION") == 0)
            {
                info.comment = save;
            }
        }
    }
    else if (metadata->type == FLAC__METADATA_TYPE_PICTURE)
    {
        uint32_t size = metadata->data.picture.data_length;
        info.image = new data_item(size);
        memcpy(info.image->data, metadata->data.picture.data, size);
    }
}

void FlacMetadata::error_callback(::FLAC__StreamDecoderErrorStatus status)
{
    LV_LOG_ERROR("Error: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}

bool FlacMetadata::decode_get_info()
{
    if (init() != FLAC__STREAM_DECODER_INIT_STATUS_OK || !process_until_end_of_metadata())
    {
        return false;
    }

    return true;
}
