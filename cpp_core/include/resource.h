#pragma once

#include <string>

namespace securefs {

class Resource {
public:
    explicit Resource(std::string resourceId);
    virtual ~Resource() = default;

    const std::string& getResourceId() const;

private:
    std::string resourceId_;
};

}  // namespace securefs
