cmake_minimum_required(VERSION 2.8)
project(llvm-prof)

set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	${CMAKE_SOURCE_DIR}/cmake
	)

set(LLVM_RECOMMAND_VERSION "" CACHE STRING "Switch the llvm version")
find_package(LLVM)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(libprofile)
