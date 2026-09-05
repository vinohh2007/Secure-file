#include "key_manager.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>

#include <cstring>
#include <fstream>
#include <memory>

namespace securefs {

void EvpPkeyDeleter::operator()(EVP_PKEY* p) const { EVP_PKEY_free(p); }

KeyManager::KeyManager() = default;

namespace {

void throwOpenSslError(const std::string& context) {
    char buf[256];
    unsigned long err = ERR_peek_last_error();
    if (err != 0) {
        ERR_error_string_n(err, buf, sizeof(buf));
        ERR_clear_error();
        throw CryptoException(context + ": " + buf);
    }
    throw CryptoException(context);
}

std::string readWholeFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw CryptoException("Cannot read file: " + path);
    }
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void writeWholeFile(const std::string& path, const std::string& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw CryptoException("Cannot write file: " + path);
    }
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
}

}  // namespace

void KeyManager::generateRsaKeyPair(const std::string& publicPemPath,
                                    const std::string& privatePemPath,
                                    int bits) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx) {
        throwOpenSslError("EVP_PKEY_CTX_new_id");
    }
    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throwOpenSslError("EVP_PKEY_keygen_init");
    }
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throwOpenSslError("EVP_PKEY_CTX_set_rsa_keygen_bits");
    }
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throwOpenSslError("EVP_PKEY_keygen");
    }
    EVP_PKEY_CTX_free(ctx);

    PKeyPtr key(pkey);

    BIO* pubBio = BIO_new(BIO_s_mem());
    if (!pubBio) {
        throw CryptoException("BIO_new failed");
    }
    if (PEM_write_bio_PUBKEY(pubBio, key.get()) != 1) {
        BIO_free(pubBio);
        throwOpenSslError("PEM_write_bio_PUBKEY");
    }
    char* pubData = nullptr;
    long pubLen = BIO_get_mem_data(pubBio, &pubData);
    writeWholeFile(publicPemPath, std::string(pubData, static_cast<size_t>(pubLen)));
    BIO_free(pubBio);

    BIO* privBio = BIO_new(BIO_s_mem());
    if (!privBio) {
        throw CryptoException("BIO_new failed");
    }
    if (PEM_write_bio_PrivateKey(privBio, key.get(), nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        BIO_free(privBio);
        throwOpenSslError("PEM_write_bio_PrivateKey");
    }
    char* privData = nullptr;
    long privLen = BIO_get_mem_data(privBio, &privData);
    writeWholeFile(privatePemPath, std::string(privData, static_cast<size_t>(privLen)));
    BIO_free(privBio);
}

PKeyPtr KeyManager::loadPublicPem(const std::string& path) const {
    std::string pem = readWholeFile(path);
    BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    if (!bio) {
        throw CryptoException("BIO_new_mem_buf failed");
    }
    EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!pkey) {
        ERR_clear_error();
        throw CryptoException("PEM_read_bio_PUBKEY failed: " + path);
    }
    PKeyPtr result(pkey);
    ensureRsaKey(result.get(), false);
    return result;
}

PKeyPtr KeyManager::loadPrivatePem(const std::string& path) const {
    std::string pem = readWholeFile(path);
    BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    if (!bio) {
        throw CryptoException("BIO_new_mem_buf failed");
    }
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!pkey) {
        ERR_clear_error();
        throw CryptoException("PEM_read_bio_PrivateKey failed: " + path);
    }
    PKeyPtr result(pkey);
    ensureRsaKey(result.get(), true);
    return result;
}

void KeyManager::ensureRsaKey(const EVP_PKEY* key, bool needPrivate) {
    (void)needPrivate;
    if (EVP_PKEY_get_base_id(key) != EVP_PKEY_RSA) {
        throw CryptoException("Key is not RSA");
    }
}

std::vector<uint8_t> KeyManager::rsaOaepSha256Encrypt(const EVP_PKEY* publicKey,
                                                      const uint8_t* data,
                                                      size_t dataLen) const {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(const_cast<EVP_PKEY*>(publicKey), nullptr);
    if (!ctx) {
        throwOpenSslError("EVP_PKEY_CTX_new");
    }
    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throwOpenSslError("EVP_PKEY_encrypt_init");
    }
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throwOpenSslError("EVP_PKEY_CTX_set_rsa_padding");
    }
    if (EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throwOpenSslError("EVP_PKEY_CTX_set_rsa_oaep_md");
    }
    if (EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throwOpenSslError("EVP_PKEY_CTX_set_rsa_mgf1_md");
    }

    size_t outlen = 0;
    if (EVP_PKEY_encrypt(ctx, nullptr, &outlen, data, dataLen) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throwOpenSslError("EVP_PKEY_encrypt (size)");
    }
    std::vector<uint8_t> out(outlen);
    if (EVP_PKEY_encrypt(ctx, out.data(), &outlen, data, dataLen) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throwOpenSslError("EVP_PKEY_encrypt");
    }
    EVP_PKEY_CTX_free(ctx);
    out.resize(outlen);
    return out;
}

std::vector<uint8_t> KeyManager::rsaOaepSha256Decrypt(const EVP_PKEY* privateKey,
                                                      const uint8_t* data,
                                                      size_t dataLen) const {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(const_cast<EVP_PKEY*>(privateKey), nullptr);
    if (!ctx) {
        throwOpenSslError("EVP_PKEY_CTX_new");
    }
    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throwOpenSslError("EVP_PKEY_decrypt_init");
    }
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throwOpenSslError("EVP_PKEY_CTX_set_rsa_padding decrypt");
    }
    if (EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throwOpenSslError("EVP_PKEY_CTX_set_rsa_oaep_md decrypt");
    }
    if (EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throwOpenSslError("EVP_PKEY_CTX_set_rsa_mgf1_md decrypt");
    }

    size_t outlen = 0;
    if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, data, dataLen) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throwOpenSslError("EVP_PKEY_decrypt (size)");
    }
    std::vector<uint8_t> out(outlen);
    if (EVP_PKEY_decrypt(ctx, out.data(), &outlen, data, dataLen) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throwOpenSslError("EVP_PKEY_decrypt");
    }
    EVP_PKEY_CTX_free(ctx);
    out.resize(outlen);
    return out;
}

std::vector<uint8_t> KeyManager::randomBytes(size_t count) const {
    std::vector<uint8_t> buf(count);
    if (RAND_bytes(buf.data(), static_cast<int>(count)) != 1) {
        throw CryptoException("RAND_bytes failed");
    }
    return buf;
}

}  // namespace securefs
