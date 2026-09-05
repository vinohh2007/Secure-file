#include "aes_gcm_rsa_encryption_service.h"

#include "bundle_format.h"
#include "crypto_error.h"

#include <openssl/err.h>

#include <cstring>

namespace securefs {

namespace {

void throwCrypto(const std::string& msg) {
    ERR_clear_error();
    throw CryptoException(msg);
}

}  // namespace

AesGcmRsaEncryptionService::AesGcmRsaEncryptionService(KeyManager& keyManager)
    : keyManager_(keyManager) {}

std::vector<uint8_t> AesGcmRsaEncryptionService::encryptFileContent(
    const std::vector<uint8_t>& plaintext,
    const EVP_PKEY* recipientPublicKey) {
    std::vector<uint8_t> aesKey = keyManager_.randomBytes(bundle::kAes256KeyLength);
    std::vector<uint8_t> iv = keyManager_.randomBytes(bundle::kIvLength);

    std::vector<uint8_t> wrappedKey =
        keyManager_.rsaOaepSha256Encrypt(recipientPublicKey, aesKey.data(), aesKey.size());

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throwCrypto("EVP_CIPHER_CTX_new failed");
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throwCrypto("EVP_EncryptInit_ex (cipher)");
    }
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, aesKey.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throwCrypto("EVP_EncryptInit_ex (key/iv)");
    }

    std::vector<uint8_t> ciphertext(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
    int len = 0;
    int outLen = 0;
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(),
                          static_cast<int>(plaintext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throwCrypto("EVP_EncryptUpdate");
    }
    outLen = len;
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + outLen, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throwCrypto("EVP_EncryptFinal_ex");
    }
    outLen += len;
    ciphertext.resize(static_cast<size_t>(outLen));

    unsigned char tag[bundle::kGcmTagLength];
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, static_cast<int>(bundle::kGcmTagLength), tag) !=
        1) {
        EVP_CIPHER_CTX_free(ctx);
        throwCrypto("EVP_CTRL_GCM_GET_TAG");
    }
    EVP_CIPHER_CTX_free(ctx);

    std::vector<uint8_t> bundle;
    bundle.reserve(4 + 4 + 4 + wrappedKey.size() + bundle::kIvLength + ciphertext.size() +
                   bundle::kGcmTagLength);
    bundle.insert(bundle.end(), std::begin(bundle::kMagic), std::end(bundle::kMagic));

    uint8_t verBuf[4];
    bundle::writeUint32Be(verBuf, bundle::kVersion);
    bundle.insert(bundle.end(), verBuf, verBuf + 4);

    uint8_t wlenBuf[4];
    bundle::writeUint32Be(wlenBuf, static_cast<uint32_t>(wrappedKey.size()));
    bundle.insert(bundle.end(), wlenBuf, wlenBuf + 4);
    bundle.insert(bundle.end(), wrappedKey.begin(), wrappedKey.end());
    bundle.insert(bundle.end(), iv.begin(), iv.end());
    bundle.insert(bundle.end(), ciphertext.begin(), ciphertext.end());
    bundle.insert(bundle.end(), tag, tag + bundle::kGcmTagLength);

    return bundle;
}

std::vector<uint8_t> AesGcmRsaEncryptionService::decryptFileContent(
    const std::vector<uint8_t>& bundleData,
    const EVP_PKEY* recipientPrivateKey) {
    size_t pos = 0;
    if (bundleData.size() < 4 + 4 + 4 + bundle::kIvLength + bundle::kGcmTagLength) {
        throw CryptoException("Bundle too small");
    }
    if (std::memcmp(bundleData.data(), bundle::kMagic, 4) != 0) {
        throw CryptoException("Invalid bundle magic");
    }
    pos += 4;
    uint32_t version = bundle::readUint32Be(bundleData.data() + pos);
    pos += 4;
    if (version != bundle::kVersion) {
        throw CryptoException("Unsupported bundle version");
    }
    uint32_t wrappedLen = bundle::readUint32Be(bundleData.data() + pos);
    pos += 4;
    if (bundleData.size() < pos + wrappedLen + bundle::kIvLength + bundle::kGcmTagLength) {
        throw CryptoException("Corrupt bundle (wrapped key)");
    }
    const uint8_t* wrappedPtr = bundleData.data() + pos;
    pos += wrappedLen;

    std::vector<uint8_t> aesKey = keyManager_.rsaOaepSha256Decrypt(
        recipientPrivateKey, wrappedPtr, static_cast<size_t>(wrappedLen));

    if (aesKey.size() != bundle::kAes256KeyLength) {
        throw CryptoException("Unexpected AES key length after RSA unwrap");
    }

    std::vector<uint8_t> iv(bundleData.begin() + static_cast<std::ptrdiff_t>(pos),
                            bundleData.begin() + static_cast<std::ptrdiff_t>(pos + bundle::kIvLength));
    pos += bundle::kIvLength;

    size_t tagPos = bundleData.size() - bundle::kGcmTagLength;
    if (tagPos <= pos) {
        throw CryptoException("Corrupt bundle (ciphertext)");
    }
    std::vector<uint8_t> ciphertext(bundleData.begin() + static_cast<std::ptrdiff_t>(pos),
                                    bundleData.begin() + static_cast<std::ptrdiff_t>(tagPos));
    std::vector<uint8_t> tag(bundleData.begin() + static_cast<std::ptrdiff_t>(tagPos),
                             bundleData.end());

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throwCrypto("EVP_CIPHER_CTX_new (decrypt)");
    }
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throwCrypto("EVP_DecryptInit_ex (cipher)");
    }
    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, aesKey.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throwCrypto("EVP_DecryptInit_ex (key/iv)");
    }

    std::vector<uint8_t> plaintext(ciphertext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
    int len = 0;
    int outLen = 0;
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(),
                          static_cast<int>(ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throwCrypto("EVP_DecryptUpdate");
    }
    outLen = len;
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.size()), tag.data()) !=
        1) {
        EVP_CIPHER_CTX_free(ctx);
        throwCrypto("EVP_CTRL_GCM_SET_TAG");
    }
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + outLen, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throwCrypto("EVP_DecryptFinal_ex (auth failed or corrupt data)");
    }
    outLen += len;
    plaintext.resize(static_cast<size_t>(outLen));
    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}

}  // namespace securefs
