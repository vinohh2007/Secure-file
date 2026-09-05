#pragma once

#include "crypto_error.h"

#include <openssl/evp.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace securefs {

struct EvpPkeyDeleter {
    void operator()(EVP_PKEY* p) const;
};

using PKeyPtr = std::unique_ptr<EVP_PKEY, EvpPkeyDeleter>;

class KeyManager {
public:
    KeyManager();

    void generateRsaKeyPair(const std::string& publicPemPath,
                            const std::string& privatePemPath,
                            int bits);

    PKeyPtr loadPublicPem(const std::string& path) const;
    PKeyPtr loadPrivatePem(const std::string& path) const;

    std::vector<uint8_t> rsaOaepSha256Encrypt(const EVP_PKEY* publicKey,
                                              const uint8_t* data,
                                              size_t dataLen) const;

    std::vector<uint8_t> rsaOaepSha256Decrypt(const EVP_PKEY* privateKey,
                                              const uint8_t* data,
                                              size_t dataLen) const;

    std::vector<uint8_t> randomBytes(size_t count) const;

private:
    static void ensureRsaKey(const EVP_PKEY* key, bool needPrivate);
};

}  // namespace securefs
