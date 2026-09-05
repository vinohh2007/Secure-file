#pragma once

#include "principal.h"

#include <string>

namespace securefs {

class User : public Principal {
public:
    User(std::string id, std::string displayName, std::string publicKeyPemPath);

    const std::string& getDisplayName() const;
    const std::string& getPublicKeyPath() const;

private:
    std::string displayName_;
    std::string publicKeyPemPath_;
};

}  // namespace securefs
