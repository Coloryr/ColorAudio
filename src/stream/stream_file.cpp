#include <string.h>
#include <malloc.h>

#include "lvgl/src/misc/lv_log.h"

#include "common/utils.h"

#include "stream_file.h"

using namespace coloraudio::stream;

FileStream::FileStream(const char *path) : BaseStream(STREAM_TYPE_FILE)
{
    if (path == nullptr)
    {
        throw "[stream] Can't open null file";
    }
    file = fopen(path, "r");
    if (file == nullptr)
    {
        LV_LOG_ERROR("[stream] Can't open file: %s", path);
        return;
    }

    this->path = strdup(path);

    uint32_t fsta = ftell(file);
    fseek(file, 0, SEEK_END);
    uint32_t fend = ftell(file);
    uint32_t flen = fend - fsta;
    if (flen > 0)
    {
        fseek(file, 0, SEEK_SET);
    }

    size = flen;
}

FileStream::~FileStream()
{
    if (file)
    {
        fclose(file);
        file = NULL;
    }

    if (path)
    {
        free(path);
        path = NULL;
    }
}

FileStream *FileStream::copy()
{
    FileStream *st1 = new FileStream(path);
    st1->seek(pos, SEEK_SET);
    return st1;
}

uint32_t FileStream::read(uint8_t *buffer, uint32_t len)
{
    uint32_t temp = fread(buffer, 1, len, file);
    pos += temp;
    return temp;
}

uint32_t FileStream::write(uint8_t *buffer, uint32_t len)
{
    uint32_t written = fwrite(buffer, 1, len, file);
    pos += written;
    return written;
}

uint32_t FileStream::peek(uint8_t *buffer, uint32_t len)
{
    uint32_t temp = fread(buffer, 1, len, file);
    fseek(file, pos, SEEK_SET);
    return temp;
}

void FileStream::seek(int32_t pos, uint8_t where)
{
    fseek(file, pos, where);
    this->pos = ftell(file);
}
