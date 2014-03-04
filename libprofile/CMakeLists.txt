set(SOURCES
  BasicBlockTracing.c
  CommonProfiling.c
  PathProfiling.c
  EdgeProfiling.c
  OptimalEdgeProfiling.c
  ValueProfiling.c
  )

include_directories(../include ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS} -Wall)
if(OUTPIT_HASPID)
	add_definitions(-DOUTPUT_HASPID)
endif()

add_library( profile_rt-static ${SOURCES} )
set_target_properties( profile_rt-static
  PROPERTIES
  OUTPUT_NAME "profile_rt" )

set(BUILD_SHARED_LIBS ON)
add_library( profile_rt-shared ${SOURCES} )
set_target_properties( profile_rt-shared
  PROPERTIES
  OUTPUT_NAME "profile_rt" )

install(TARGETS profile_rt-static profile_rt-shared
	DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
