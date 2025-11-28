cmake_minimum_required(VERSION 3.10)
project(brotli_udf_db2)

find_package(PkgConfig REQUIRED)
pkg_check_modules(BROTLI REQUIRED libbrotlienc libbrotlidec)

include_directories(${BROTLI_INCLUDE_DIRS})
add_library(brotli_decompress SHARED brotli_decompress.c)
target_link_libraries(brotli_decompress ${BROTLI_LIBRARIES})
set_target_properties(brotli_decompress PROPERTIES PREFIX "")</code>
</update_todo_list>