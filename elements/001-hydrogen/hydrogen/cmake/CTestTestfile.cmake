# CMake generated Testfile for 
# Source directory: /home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake
# Build directory: /home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(hydrogen_basic_test "/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake/../tests/22_startup_shutdown.sh")
set_tests_properties(hydrogen_basic_test PROPERTIES  ENVIRONMENT "CMAKE_BUILD=1" TIMEOUT "300" WORKING_DIRECTORY "/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake" _BACKTRACE_TRIPLES "/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake/CMakeLists.txt;680;add_test;/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake/CMakeLists.txt;0;")
add_test(hydrogen_compilation_test "/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake/../tests/test_01_compilation.sh")
set_tests_properties(hydrogen_compilation_test PROPERTIES  ENVIRONMENT "CMAKE_BUILD=1" TIMEOUT "300" WORKING_DIRECTORY "/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake" _BACKTRACE_TRIPLES "/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake/CMakeLists.txt;685;add_test;/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake/CMakeLists.txt;0;")
add_test(hydrogen_dependencies_test "/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake/../tests/test_16_library_dependencies.sh")
set_tests_properties(hydrogen_dependencies_test PROPERTIES  ENVIRONMENT "CMAKE_BUILD=1" TIMEOUT "300" WORKING_DIRECTORY "/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake" _BACKTRACE_TRIPLES "/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake/CMakeLists.txt;690;add_test;/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake/CMakeLists.txt;0;")
add_test(hydrogen_unity_tests "/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake/../tests/test_11_unity.sh")
set_tests_properties(hydrogen_unity_tests PROPERTIES  WORKING_DIRECTORY "/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake" _BACKTRACE_TRIPLES "/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake/CMakeLists.txt;695;add_test;/home/asimard/Projects/Philement/elements/001-hydrogen/hydrogen/cmake/CMakeLists.txt;0;")
