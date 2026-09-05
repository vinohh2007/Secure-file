#include "user.h"

namespace securefs {

User::User(std::string id, std::string displayName, std::string publicKeyPemPath)
    : Principal(std::move(id)),
      displayName_(std::move(displayName)),
      publicKeyPemPath_(std::move(publicKeyPemPath)) {}

const std::string& User::getDisplayName() const { return displayName_; }

const std::string& User::getPublicKeyPath() const { return publicKeyPemPath_; }

}  // namespace securefs
