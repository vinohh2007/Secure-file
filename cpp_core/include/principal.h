#pragma once

#include <string>

namespace securefs {

class Principal {
public:
    explicit Principal(std::string id);
    virtual ~Principal() = default;

    const std::string& getId() const;

private:
    std::string id_;
};

}  // namespace securefs
