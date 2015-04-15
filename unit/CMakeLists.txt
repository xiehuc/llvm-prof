enable_testing()
include_directories(
   ${GTEST_INCLUDE_DIRS} 
   ${PROJECT_SOURCE_DIR}/include
   ${PROJECT_SOURCE_DIR}/libprofile
   )
add_definitions(-std=c++11)
add_executable(unit-test
   FreeExprUnit.cpp
   )

target_link_libraries(unit-test
   ${GTEST_LIBRARIES}
   ${LLVM_DYNAMIC_LIBRARY}
   LLVMProfiling-static
   profile_rt-static
   pthread
   gtest_main
   )

