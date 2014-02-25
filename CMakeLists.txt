cmake_minimum_required(VERSION 2.8)
project(llvm-prof)

set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	${CMAKE_SOURCE_DIR}/cmake
	)

set(LLVM_RECOMMAND_VERSION "" CACHE STRING "Switch the llvm version")
find_package(LLVM)

configure_file(llvm-prof.pc.in ${CMAKE_BINARY_DIR}/llvm-prof.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/llvm-prof.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
message(STATUS "Add ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig to $PKG_CONFIG_PATH "
"environment variable to let pkg-config find llvm-prof.pc")

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(libprofile)
