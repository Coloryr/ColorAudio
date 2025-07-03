#include "dep.h"

#include <string>

#include <iostream>
#include <string>
#include <cryptopp/base64.h>
#include <cryptopp/aes.h>     // 包含AES加密算法的头文件
#include <cryptopp/filters.h> // 包含加解密过程中使用的filters头文件
#include <cryptopp/modes.h>   // 包含加解密过程中使用的modes头文件
#include <cryptopp/hex.h>     // 包含将二进制转换为十六进制的头文件

using namespace CryptoPP;

const uint8_t key[] = {0x23, 0x31, 0x34, 0x6C,
                       0x6A, 0x6B, 0x5F, 0x21,
                       0x5C, 0x5D, 0x26, 0x30,
                       0x55, 0x3C, 0x27, 0x28};

static std::string decrypt(const std::string &data)
{
    const int keylen = 16;

    try
    {
        std::string decrypted;
        CryptoPP::ECB_Mode<CryptoPP::AES>::Decryption decryptor;
        decryptor.SetKey(key, keylen);

        // 使用PKCS#7填充方案
        CryptoPP::StringSource ss(data, true,
                                  new CryptoPP::StreamTransformationFilter(decryptor,
                                                                           new CryptoPP::StringSink(decrypted),
                                                                           CryptoPP::StreamTransformationFilter::PKCS_PADDING));
        return decrypted;
    }
    catch (const CryptoPP::Exception &e)
    {
        return "";
    }
}

std::string dep(std::string &input)
{
    try
    {
        std::string decoded;
        CryptoPP::StringSource ss(input, true,
                                  new CryptoPP::Base64Decoder(
                                      new CryptoPP::StringSink(decoded)));

        return decrypt(decoded);
    }
    catch (const CryptoPP::Exception &e)
    {
    }
    return "";
}