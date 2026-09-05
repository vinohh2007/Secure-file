#include "crypto_error.h"

namespace securefs {

CryptoException::CryptoException(const std::string& message)
    : std::runtime_error(message) {}

}  // namespace securefs
