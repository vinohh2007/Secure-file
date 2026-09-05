#pragma once

#include <openssl/evp.h>

#include <vector>

namespace securefs {

class IEncryptionService {
public:
    virtual ~IEncryptionService() = default;

    virtual std::vector<uint8_t> encryptFileContent(const std::vector<uint8_t>& plaintext,
                                                    const EVP_PKEY* recipientPublicKey) = 0;

    virtual std::vector<uint8_t> decryptFileContent(const std::vector<uint8_t>& bundle,
                                                    const EVP_PKEY* recipientPrivateKey) = 0;
};

}  // namespace securefs
