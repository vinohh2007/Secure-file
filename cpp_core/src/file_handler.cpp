#include "file_handler.h"

#include "crypto_error.h"

#include <fstream>
#include <iterator>

namespace securefs {

FileHandler::FileHandler(IEncryptionService& encryptionService) : encryptionService_(encryptionService) {}

std::vector<uint8_t> FileHandler::readBinaryFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw CryptoException("Cannot read file: " + path);
    }
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void FileHandler::writeBinaryFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw CryptoException("Cannot write file: " + path);
    }
    out.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
}

void FileHandler::encryptFileToBundle(const std::string& plaintextPath,
                                      const std::string& bundlePath,
                                      const EVP_PKEY* recipientPublicKey) {
    std::vector<uint8_t> plaintext = readBinaryFile(plaintextPath);
    std::vector<uint8_t> bundle = encryptionService_.encryptFileContent(plaintext, recipientPublicKey);
    writeBinaryFile(bundlePath, bundle);
}

void FileHandler::decryptBundleToFile(const std::string& bundlePath,
                                      const std::string& plaintextPath,
                                      const EVP_PKEY* recipientPrivateKey) {
    std::vector<uint8_t> bundle = readBinaryFile(bundlePath);
    std::vector<uint8_t> plaintext = encryptionService_.decryptFileContent(bundle, recipientPrivateKey);
    writeBinaryFile(plaintextPath, plaintext);
}

}  // namespace securefs
