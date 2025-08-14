#ifndef __NCMCRYPT_H__
#define __NCMCRYPT_H__

#include "aes.h"
#include "json/json.hpp"
#include "stream/stream.h"

using namespace nlohmann;

class NeteaseMusicMetadata
{

private:
	std::string mAlbum;
	std::string mArtist;
	std::string mFormat;
	std::string mName;
	int mDuration;
	int mBitrate;

private:
	json mRaw;

public:
	NeteaseMusicMetadata(std::string &);
	~NeteaseMusicMetadata();
	const std::string &name() const { return mName; }
	const std::string &album() const { return mAlbum; }
	const std::string &artist() const { return mArtist; }
	const std::string &format() const { return mFormat; }
	const int duration() const { return mDuration; }
	const int bitrate() const { return mBitrate; }
};

class NeteaseCrypt
{
private:
	static const unsigned char sCoreKey[17];
	static const unsigned char mPng[8];
	coloraudio::stream::BaseStream *mFile;
	unsigned char mKeyBox[256]{};

	bool isNcmFile();
	void buildKeyBox(unsigned char *key, int keyLen);

public:
	static const unsigned char sModifyKey[17];

	uint8_t *mImageData;
	uint32_t imageSize;

	NeteaseMusicMetadata *mMetaData;
	std::string modify;

	NeteaseCrypt(coloraudio::stream::BaseStream *, bool meta);
	~NeteaseCrypt();

	void decode(uint8_t *output, uint32_t size);
};

void aesEcbDecrypt(const unsigned char *key, std::string &src, std::string &dst);

#endif // __NCMCRYPT_H__