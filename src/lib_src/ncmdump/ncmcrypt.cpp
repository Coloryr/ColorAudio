#include "ncmcrypt.h"
#include "aes.h"
#include "base64.h"
#include "json/json.hpp"

#include <stdexcept>
#include <string>
#include <filesystem>
#include <malloc.h>

using namespace nlohmann;

const unsigned char NeteaseCrypt::sCoreKey[17] = {0x68, 0x7A, 0x48, 0x52, 0x41, 0x6D, 0x73, 0x6F, 0x35, 0x6B, 0x49, 0x6E, 0x62, 0x61, 0x78, 0x57, 0};
const unsigned char NeteaseCrypt::sModifyKey[17] = {0x23, 0x31, 0x34, 0x6C, 0x6A, 0x6B, 0x5F, 0x21, 0x5C, 0x5D, 0x26, 0x30, 0x55, 0x3C, 0x27, 0x28, 0};

const unsigned char NeteaseCrypt::mPng[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

static void aesEcbDecrypt(const unsigned char *key, std::string &src, std::string &dst)
{
    int n, i;

    unsigned char out[16];

    n = src.length() >> 4;

    dst.clear();

    AES aes(key);

    for (i = 0; i < n - 1; i++)
    {
        aes.decrypt((unsigned char *)src.c_str() + (i << 4), out);
        dst += std::string((char *)out, 16);
    }

    aes.decrypt((unsigned char *)src.c_str() + (i << 4), out);
    char pad = out[15];
    if (pad > 16)
    {
        pad = 0;
    }
    dst += std::string((char *)out, 16 - pad);
}

static void replace(std::string &str, const std::string &from, const std::string &to)
{
    if (from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

NeteaseMusicMetadata::~NeteaseMusicMetadata()
{
}

NeteaseMusicMetadata::NeteaseMusicMetadata(std::string &raw)
{
    if (raw.empty())
    {
        return;
    }

    int artistLen, i;

    mRaw = json::parse(raw);

    json swap = mRaw["musicName"];
    if (swap.is_string())
    {
        mName = swap.get<std::string>();
    }

    swap = mRaw["album"];
    if (swap.is_string())
    {
        mAlbum = swap.get<std::string>();
    }

    swap = mRaw["artist"];
    if (swap.is_array())
    {
        artistLen = swap.size();

        for (const auto &artist : swap)
        {
            if (artist.size() > 0)
            {
                if (!mArtist.empty())
                {
                    mArtist += "/";
                }
                mArtist += artist[0].get<std::string>();
            }
        }
    }

    swap = mRaw["bitrate"];
    if (swap.is_number())
    {
        mBitrate = swap.get<int>();
    }

    swap = mRaw["duration"];
    if (swap.is_number())
    {
        mDuration = swap.get<int>();
    }

    swap = mRaw["format"];
    if (swap.is_string())
    {
        mFormat = swap.get<std::string>();
    }
}

bool NeteaseCrypt::isNcmFile()
{
    uint32_t header;

    mFile->read(reinterpret_cast<uint8_t *>(&header), sizeof(header));
    if (header != (uint32_t)0x4e455443)
    {
        return false;
    }

    mFile->read(reinterpret_cast<uint8_t *>(&header), sizeof(header));
    if (header != (uint32_t)0x4d414446)
    {
        return false;
    }

    return true;
}

void NeteaseCrypt::buildKeyBox(unsigned char *key, int keyLen)
{
    int i;
    for (i = 0; i < 256; ++i)
    {
        mKeyBox[i] = (unsigned char)i;
    }

    unsigned char swap = 0;
    unsigned char c = 0;
    unsigned char last_byte = 0;
    unsigned char key_offset = 0;

    for (i = 0; i < 256; ++i)
    {
        swap = mKeyBox[i];
        c = ((swap + last_byte + key[key_offset++]) & 0xff);
        if (key_offset >= keyLen)
            key_offset = 0;
        mKeyBox[i] = mKeyBox[c];
        mKeyBox[c] = swap;
        last_byte = c;
    }
}

std::string NeteaseCrypt::mimeType(std::string &data)
{
    if (memcmp(data.c_str(), mPng, 8) == 0)
    {
        return std::string("image/png");
    }

    return std::string("image/jpeg");
}

void NeteaseCrypt::Dump(uint8_t *output, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        uint32_t j = (i + 1) & 0xff;
        output[i] ^= mKeyBox[(mKeyBox[j] + mKeyBox[(mKeyBox[j] + j) & 0xff]) & 0xff];
    }
}

NeteaseCrypt::~NeteaseCrypt()
{
    if (mMetaData != NULL)
    {
        delete mMetaData;
    }
    if (mImageData != NULL)
    {
        free(mImageData);
    }
}

NeteaseCrypt::NeteaseCrypt(ColorAudio::Stream *st, bool meta)
{
    mFile = st;

    if (!isNcmFile())
    {
        return;
    }

    mFile->seek(2, SEEK_CUR);

    uint32_t n;
    mFile->read(reinterpret_cast<uint8_t *>(&n), sizeof(n));

    if (n <= 0)
    {
        throw std::invalid_argument("Broken NCM file");
    }

    std::vector<char> keydata(n);
    mFile->read(reinterpret_cast<uint8_t *>(keydata.data()), n);

    for (size_t i = 0; i < n; i++)
    {
        keydata[i] ^= 0x64;
    }

    std::string rawKeyData(keydata.begin(), keydata.end());
    std::string mKeyData;

    aesEcbDecrypt(sCoreKey, rawKeyData, mKeyData);

    buildKeyBox((unsigned char *)mKeyData.c_str() + 17, mKeyData.length() - 17);

    mFile->read(reinterpret_cast<uint8_t *>(&n), sizeof(n));

    if (n <= 0)
    {
        mMetaData = NULL;
    }
    else
    {
        if (meta)
        {
            std::vector<char> modifyData(n);
            mFile->read(reinterpret_cast<uint8_t *>(modifyData.data()), n);

            for (size_t i = 0; i < n; i++)
            {
                modifyData[i] ^= 0x63;
            }

            modify = std::string(modifyData.begin(), modifyData.end());

            std::string modifyOutData;
            std::string modifyDecryptData;
            std::string swapModifyData = modify.substr(22);

            // escape `163 key(Don't modify):`
            Base64::Decode(swapModifyData, modifyOutData);

            aesEcbDecrypt(sModifyKey, modifyOutData, modifyDecryptData);

            // escape `music:`
            modifyDecryptData = std::string(modifyDecryptData.begin() + 6, modifyDecryptData.end());

            mMetaData = new NeteaseMusicMetadata(modifyDecryptData);
        }
        else
        {
            mFile->seek(n, SEEK_CUR);
        }
    }

    // skip crc32 & image version
    mFile->seek(5, SEEK_CUR);

    uint32_t cover_frame_len{0};
    mFile->read(reinterpret_cast<uint8_t *>(&cover_frame_len), 4);
    mFile->read(reinterpret_cast<uint8_t *>(&n), sizeof(n));

    if (n > 0)
    {
        if (meta)
        {
            imageSize = n;
            mImageData = static_cast<uint8_t *>(malloc(n));
            mFile->read(mImageData, n);
        }
        else
        {
            mFile->seek(n, SEEK_CUR);
        }
    }

    mFile->seek(cover_frame_len - n, SEEK_CUR);
}
