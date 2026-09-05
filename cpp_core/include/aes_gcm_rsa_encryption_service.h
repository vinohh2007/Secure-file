#pragma once

#include "i_encryption_service.h"
#include "key_manager.h"

#include <openssl/evp.h>

#include <vector>

namespace securefs {

class AesGcmRsaEncryptionService : public IEncryptionService {
public:
    explicit AesGcmRsaEncryptionService(KeyManager& keyManager);

    std::vector<uint8_t> encryptFileContent(const std::vector<uint8_t>& plaintext,
                                              const EVP_PKEY* recipientPublicKey) override;

    std::vector<uint8_t> decryptFileContent(const std::vector<uint8_t>& bundle,
                                            const EVP_PKEY* recipientPrivateKey) override;

private:
    KeyManager& keyManager_;
};

}  // namespace securefs
