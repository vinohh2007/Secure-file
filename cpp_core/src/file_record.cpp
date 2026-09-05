#include "file_record.h"

namespace securefs {

FileRecord::FileRecord(std::string fileId,
                       std::string senderId,
                       std::string receiverId,
                       std::string storagePath,
                       std::string originalName)
    : Resource(std::move(fileId)),
      senderId_(std::move(senderId)),
      receiverId_(std::move(receiverId)),
      storagePath_(std::move(storagePath)),
      originalName_(std::move(originalName)) {}

const std::string& FileRecord::getSenderId() const { return senderId_; }

const std::string& FileRecord::getReceiverId() const { return receiverId_; }

const std::string& FileRecord::getStoragePath() const { return storagePath_; }

const std::string& FileRecord::getOriginalName() const { return originalName_; }

bool FileRecord::validate() const {
    return !getResourceId().empty() && !senderId_.empty() && !receiverId_.empty() &&
           !storagePath_.empty();
}

}  // namespace securefs
