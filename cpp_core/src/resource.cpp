#include "resource.h"

namespace securefs {

Resource::Resource(std::string resourceId) : resourceId_(std::move(resourceId)) {}

const std::string& Resource::getResourceId() const { return resourceId_; }

}  // namespace securefs
