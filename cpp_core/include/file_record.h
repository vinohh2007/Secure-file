#pragma once

#include "resource.h"

#include <string>

namespace securefs {

class FileRecord : public Resource {
public:
    FileRecord(std::string fileId,
               std::string senderId,
               std::string receiverId,
               std::string storagePath,
               std::string originalName);

    const std::string& getSenderId() const;
    const std::string& getReceiverId() const;
    const std::string& getStoragePath() const;
    const std::string& getOriginalName() const;

    bool validate() const;

private:
    std::string senderId_;
    std::string receiverId_;
    std::string storagePath_;
    std::string originalName_;
};

}  // namespace securefs
