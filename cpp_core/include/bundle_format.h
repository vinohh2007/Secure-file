#pragma once

#include <cstddef>
#include <cstdint>

namespace securefs::bundle {

inline constexpr char kMagic[4] = {'S', 'F', 'E', '1'};
inline constexpr uint32_t kVersion = 1;
inline constexpr size_t kIvLength = 12;
inline constexpr size_t kGcmTagLength = 16;
inline constexpr size_t kAes256KeyLength = 32;

void writeUint32Be(uint8_t* out, uint32_t value);
uint32_t readUint32Be(const uint8_t* in);

}  // namespace securefs::bundle
