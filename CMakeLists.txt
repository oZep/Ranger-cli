cmake_minimum_required(VERSION 3.16)
project(Ranger)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBURING REQUIRED liburing)

find_package(OpenSSL REQUIRED)

add_executable(
    Ranger
    src/common.cpp
    src/config.cpp
    src/metrics.cpp
    src/main.cpp
)

target_include_directories(Ranger PRIVATE ${LIBURING_INCLUDE_DIRS})
target_link_directories(Ranger PRIVATE ${LIBURING_LIBRARY_DIRS})
target_link_libraries(
    Ranger
    ${LIBURING_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
)