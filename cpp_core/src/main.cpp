#include "access_control.h"
#include "aes_gcm_rsa_encryption_service.h"
#include "crypto_error.h"
#include "file_handler.h"
#include "file_record.h"
#include "key_manager.h"
#include "session.h"
#include "user.h"

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

void printUsage() {
    std::cerr
        << "secure_core — Secure File Sharing (OOP) — E2EE crypto CLI\n"
        << "  version\n"
        << "  keygen --pub <path.pem> --priv <path.pem> [--bits 2048]\n"
        << "  encrypt --in <file> --out <bundle.bin> --recipient-pub <pub.pem>\n"
        << "  decrypt --in <bundle.bin> --out <file> --private-key <priv.pem>\n"
        << "  check-access --sender <id> --receiver <id> --requester <id> --file-id <id> "
           "--path <storage> --name <original>\n";
}

bool argEq(const char* a, const std::string& b) { return std::strcmp(a, b.c_str()) == 0; }

std::string getOpt(int argc, char** argv, const std::string& name, size_t& i) {
    if (i + 1 < static_cast<size_t>(argc)) {
        return std::string(argv[++i]);
    }
    throw securefs::CryptoException("Missing value for " + name);
}

}  // namespace

int main(int argc, char** argv) {
    using namespace securefs;
    if (argc < 2) {
        printUsage();
        return 1;
    }
    try {
        if (argEq(argv[1], "version")) {
            std::cout << "secure_core 1.0.0 — Secure File Sharing (OOP) · AES-256-GCM + RSA-OAEP\n";
            return 0;
        }

        KeyManager keyManager;

        if (argEq(argv[1], "keygen")) {
            std::string pubPath;
            std::string privPath;
            int bits = 2048;
            for (size_t i = 2; i < static_cast<size_t>(argc); ++i) {
                if (argEq(argv[i], "--pub")) {
                    pubPath = getOpt(argc, argv, "--pub", i);
                } else if (argEq(argv[i], "--priv")) {
                    privPath = getOpt(argc, argv, "--priv", i);
                } else if (argEq(argv[i], "--bits")) {
                    bits = std::stoi(getOpt(argc, argv, "--bits", i));
                }
            }
            if (pubPath.empty() || privPath.empty()) {
                throw CryptoException("keygen requires --pub and --priv");
            }
            keyManager.generateRsaKeyPair(pubPath, privPath, bits);
            std::cout << "OK: wrote RSA key pair (" << bits << " bits)\n";
            return 0;
        }

        AesGcmRsaEncryptionService encryptionService(keyManager);
        FileHandler fileHandler(encryptionService);

        if (argEq(argv[1], "encrypt")) {
            std::string inPath;
            std::string outPath;
            std::string recipientPub;
            for (size_t i = 2; i < static_cast<size_t>(argc); ++i) {
                if (argEq(argv[i], "--in")) {
                    inPath = getOpt(argc, argv, "--in", i);
                } else if (argEq(argv[i], "--out")) {
                    outPath = getOpt(argc, argv, "--out", i);
                } else if (argEq(argv[i], "--recipient-pub")) {
                    recipientPub = getOpt(argc, argv, "--recipient-pub", i);
                }
            }
            if (inPath.empty() || outPath.empty() || recipientPub.empty()) {
                throw CryptoException("encrypt requires --in --out --recipient-pub");
            }
            auto pub = keyManager.loadPublicPem(recipientPub);
            fileHandler.encryptFileToBundle(inPath, outPath, pub.get());
            std::cout << "OK: encrypted bundle written\n";
            return 0;
        }

        if (argEq(argv[1], "decrypt")) {
            std::string inPath;
            std::string outPath;
            std::string privPath;
            for (size_t i = 2; i < static_cast<size_t>(argc); ++i) {
                if (argEq(argv[i], "--in")) {
                    inPath = getOpt(argc, argv, "--in", i);
                } else if (argEq(argv[i], "--out")) {
                    outPath = getOpt(argc, argv, "--out", i);
                } else if (argEq(argv[i], "--private-key")) {
                    privPath = getOpt(argc, argv, "--private-key", i);
                }
            }
            if (inPath.empty() || outPath.empty() || privPath.empty()) {
                throw CryptoException("decrypt requires --in --out --private-key");
            }
            auto priv = keyManager.loadPrivatePem(privPath);
            fileHandler.decryptBundleToFile(inPath, outPath, priv.get());
            std::cout << "OK: decrypted file written\n";
            return 0;
        }

        if (argEq(argv[1], "check-access")) {
            std::string senderId;
            std::string receiverId;
            std::string requesterId;
            std::string fileId;
            std::string path;
            std::string name;
            for (size_t i = 2; i < static_cast<size_t>(argc); ++i) {
                if (argEq(argv[i], "--sender")) {
                    senderId = getOpt(argc, argv, "--sender", i);
                } else if (argEq(argv[i], "--receiver")) {
                    receiverId = getOpt(argc, argv, "--receiver", i);
                } else if (argEq(argv[i], "--requester")) {
                    requesterId = getOpt(argc, argv, "--requester", i);
                } else if (argEq(argv[i], "--file-id")) {
                    fileId = getOpt(argc, argv, "--file-id", i);
                } else if (argEq(argv[i], "--path")) {
                    path = getOpt(argc, argv, "--path", i);
                } else if (argEq(argv[i], "--name")) {
                    name = getOpt(argc, argv, "--name", i);
                }
            }
            User sender(senderId, "sender", "");
            User receiver(receiverId, "receiver", "");
            User requester(requesterId, "requester", "");
            FileRecord record(fileId, senderId, receiverId, path, name);
            AccessControl ac;
            bool upload = ac.canUpload(sender, receiver);
            bool download = ac.canDownload(requester, record);
            std::cout << (upload ? "upload_allowed\n" : "upload_denied\n");
            std::cout << (download ? "download_allowed\n" : "download_denied\n");
            return 0;
        }

        printUsage();
        return 1;
    } catch (const CryptoException& ex) {
        std::cerr << "ERROR: " << ex.what() << "\n";
        return 2;
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << "\n";
        return 2;
    }
}
