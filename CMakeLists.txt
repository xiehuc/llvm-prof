cmake_minimum_required(VERSION 2.8)
project(llvm-prof)

set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	${CMAKE_SOURCE_DIR}/cmake
	)

option(OUTPUT_HASPID "Output name contain pid to support mpi program" ON)
option(ENABLE_COMPRESS "Enable Run Length Compress to decrease memory footprint" ON)
option(USE_LIB64_PATH "Trying to use lib64 when need" OFF)
if(USE_LIB64_PATH)
   set(LIB_DIRS "lib64")
else()
   set(LIB_DIRS "lib")
endif()

set(LLVM_RECOMMAND_VERSION "" CACHE STRING "Switch the llvm version")
set(MPIF90 mpif90)
find_package(LLVM REQUIRED)
find_package(GTest)
if(LLVM_VERSION VERSION_LESS "3.4")
   message(FATAL_ERROR "Need LLVM version greater than 3.4")
endif()

option(DYNAMIC_LINK "Link Dynamic LLVM Libraries" ${LLVM_DYNAMIC_LIBRARY_FOUND})

configure_file(llvm-prof.pc.in ${CMAKE_BINARY_DIR}/llvm-prof.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/llvm-prof.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
message(STATUS "Add ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig to $PKG_CONFIG_PATH "
"environment variable to let pkg-config find llvm-prof.pc")

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(libprofile)
if(GTEST_FOUND)
   add_subdirectory(unit)
endif()
