#pragma once

#include "aes.h"
#include "json/json.hpp"
#include "../../../stream/stream.h"

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
	static const unsigned char sModifyKey[17];
	static const unsigned char mPng[8];
	ColorAudio::Stream *mFile;
	unsigned char mKeyBox[256]{};

	bool isNcmFile();
	void buildKeyBox(unsigned char *key, int keyLen);
	std::string mimeType(std::string &data);

public:
	uint8_t *mImageData;
	uint32_t imageSize;

	NeteaseMusicMetadata *mMetaData;
	std::string modify;

	NeteaseCrypt(ColorAudio::Stream *, bool meta);
	~NeteaseCrypt();

	void Dump(uint8_t *output, uint32_t size);
};