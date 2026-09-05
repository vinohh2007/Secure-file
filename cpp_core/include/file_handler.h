#pragma once

#include "i_encryption_service.h"

#include <openssl/evp.h>

#include <string>
#include <vector>

namespace securefs {

class FileHandler {
public:
    explicit FileHandler(IEncryptionService& encryptionService);

    static std::vector<uint8_t> readBinaryFile(const std::string& path);
    static void writeBinaryFile(const std::string& path, const std::vector<uint8_t>& data);

    void encryptFileToBundle(const std::string& plaintextPath,
                             const std::string& bundlePath,
                             const EVP_PKEY* recipientPublicKey);

    void decryptBundleToFile(const std::string& bundlePath,
                             const std::string& plaintextPath,
                             const EVP_PKEY* recipientPrivateKey);

private:
    IEncryptionService& encryptionService_;
};

}  // namespace securefs
