#include "access_control.h"

namespace securefs {

bool AccessControl::canUpload(const User& sender, const User& receiver) const {
    if (sender.getId().empty() || receiver.getId().empty()) {
        return false;
    }
    return sender.getId() != receiver.getId();
}

bool AccessControl::canDownload(const User& requester, const FileRecord& file) const {
    if (!file.validate()) {
        return false;
    }
    return requester.getId() == file.getReceiverId() || requester.getId() == file.getSenderId();
}

}  // namespace securefs
