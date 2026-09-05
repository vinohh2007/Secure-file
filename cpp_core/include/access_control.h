#pragma once

#include "file_record.h"
#include "user.h"

#include <memory>

namespace securefs {

class AccessControl {
public:
    bool canUpload(const User& sender, const User& receiver) const;

    bool canDownload(const User& requester, const FileRecord& file) const;
};

}  // namespace securefs
