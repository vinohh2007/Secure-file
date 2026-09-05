cmake_minimum_required(VERSION 3.16)
project(secure_file_sharing_core CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)

set(SECURE_FS_CORE_SOURCES
    src/crypto_error.cpp
    src/bundle_format.cpp
    src/principal.cpp
    src/user.cpp
    src/resource.cpp
    src/file_record.cpp
    src/session.cpp
    src/key_manager.cpp
    src/aes_gcm_rsa_encryption_service.cpp
    src/file_handler.cpp
    src/access_control.cpp
)

add_library(secure_core_lib STATIC ${SECURE_FS_CORE_SOURCES})
target_include_directories(secure_core_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(secure_core_lib PUBLIC OpenSSL::Crypto)

add_executable(secure_core src/main.cpp)
target_link_libraries(secure_core PRIVATE secure_core_lib)

add_executable(test_roundtrip tests/test_roundtrip.cpp)
target_link_libraries(test_roundtrip PRIVATE secure_core_lib)

enable_testing()
add_test(NAME roundtrip COMMAND test_roundtrip)
