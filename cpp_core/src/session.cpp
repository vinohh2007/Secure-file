#include "session.h"

namespace securefs {

Session::Session() = default;

void Session::setUser(std::shared_ptr<User> user) { currentUser_ = std::move(user); }

void Session::clear() { currentUser_.reset(); }

std::shared_ptr<User> Session::getUser() const { return currentUser_; }

bool Session::isAuthenticated() const { return static_cast<bool>(currentUser_); }

}  // namespace securefs
