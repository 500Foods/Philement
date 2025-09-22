# Packaging and Installation Configuration
#
# This file contains install targets and CPack configuration.

# Install targets
install(TARGETS regular debug valgrind perf release
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "Hydrogen")
set(CPACK_PACKAGE_VERSION "${HYDROGEN_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hydrogen Server")
set(CPACK_PACKAGE_VENDOR "Philement")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE.md")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")

include(CPack)

