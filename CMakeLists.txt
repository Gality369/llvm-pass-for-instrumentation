cmake_minimum_required(VERSION 3.27)
project(llvm_pass_for_instrumentation)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_STANDARD 17)

add_executable(llvm_pass_for_instrumentation llvm-pass.cpp)

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs irreader ipo)

# Link against LLVM libraries
target_link_libraries(llvm_pass_for_instrumentation ${llvm_libs})