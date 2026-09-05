#pragma once

#include <stdexcept>
#include <string>

namespace securefs {

class CryptoException : public std::runtime_error {
public:
    explicit CryptoException(const std::string& message);
};

}  // namespace securefs
