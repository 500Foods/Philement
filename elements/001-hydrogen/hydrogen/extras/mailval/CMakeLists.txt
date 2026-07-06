cmake_minimum_required(VERSION 3.10)
project(mailval C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JANSSON REQUIRED jansson)

add_executable(mailval
    mailval.c
    util.c
    tls.c
    capture.c
    store.c
    listener.c
    proto_smtp.c
    proto_imap.c
    proto_jmap.c
)

target_include_directories(mailval PRIVATE
    ${OPENSSL_INCLUDE_DIR}
    ${JANSSON_INCLUDE_DIRS}
)

target_link_libraries(mailval
    OpenSSL::SSL
    OpenSSL::Crypto
    ${JANSSON_LIBRARIES}
    pthread
)
