#include "cryp163.h"

#include <string>

#include <iostream>
#include <string>
#include <cryptopp/base64.h>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/modes.h>
#include <cryptopp/hex.h>

using namespace CryptoPP;

const uint8_t key[] = {0x23, 0x31, 0x34, 0x6C,
                       0x6A, 0x6B, 0x5F, 0x21,
                       0x5C, 0x5D, 0x26, 0x30,
                       0x55, 0x3C, 0x27, 0x28};

static std::string decrypt(const std::string &data)
{
    try
    {
        std::string decrypted;
        ECB_Mode<AES>::Decryption decryptor;
        decryptor.SetKey(key, sizeof(key));

        StringSource ss(data, true, new StreamTransformationFilter(decryptor, new StringSink(decrypted), StreamTransformationFilter::PKCS_PADDING));
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
        StringSource ss(input, true, new Base64Decoder(new StringSink(decoded)));

        return decrypt(decoded);
    }
    catch (const CryptoPP::Exception &e)
    {
    }
    return "";
}