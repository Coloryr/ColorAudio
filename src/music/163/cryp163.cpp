#include <string>
#include <stdint.h>

#include "ncmcrypt.h"
#include "aes.h"
#include "base64.h"

#include "cryp163.h"

std::string dep(std::string &input)
{
    std::string decoded;
    std::string dst;

    Base64::Decode(input, decoded);

    aesEcbDecrypt(NeteaseCrypt::sModifyKey, decoded, dst);
    return dst;
}