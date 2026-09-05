#include "principal.h"

namespace securefs {

Principal::Principal(std::string id) : id_(std::move(id)) {}

const std::string& Principal::getId() const { return id_; }

}  // namespace securefs
