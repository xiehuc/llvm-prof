enable_testing()
include_directories(
   ${GTEST_INCLUDE_DIRS} 
   ${PROJECT_SOURCE_DIR}/include
   ${PROJECT_SOURCE_DIR}/libprofile
   )

add_executable(rt-test
   rt_slg.cc
   )

target_link_libraries(rt-test
   ${GTEST_LIBRARIES}
   profile_rt-static
   )

