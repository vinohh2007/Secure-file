#include "aes_gcm_rsa_encryption_service.h"
#include "key_manager.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

int main() {
    using namespace securefs;
    KeyManager km;
    std::vector<uint8_t> plain(1024);
    for (size_t i = 0; i < plain.size(); ++i) {
        plain[i] = static_cast<uint8_t>(i & 0xff);
    }

    km.generateRsaKeyPair("/tmp/securefs_test_pub.pem", "/tmp/securefs_test_priv.pem", 2048);
    auto pub = km.loadPublicPem("/tmp/securefs_test_pub.pem");
    auto priv = km.loadPrivatePem("/tmp/securefs_test_priv.pem");

    AesGcmRsaEncryptionService enc(km);
    std::vector<uint8_t> bundle = enc.encryptFileContent(plain, pub.get());
    assert(!bundle.empty());
    std::vector<uint8_t> out = enc.decryptFileContent(bundle, priv.get());
    assert(out == plain);
    std::cout << "test_roundtrip: OK\n";
    return 0;
}
