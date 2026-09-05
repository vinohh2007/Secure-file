#pragma once

#include "user.h"

#include <memory>

namespace securefs {

class Session {
public:
    Session();

    void setUser(std::shared_ptr<User> user);
    void clear();

    std::shared_ptr<User> getUser() const;
    bool isAuthenticated() const;

private:
    std::shared_ptr<User> currentUser_;
};

}  // namespace securefs
