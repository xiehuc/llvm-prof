set(SOURCES
  BasicBlockTracing.c
  CommonProfiling.c
  PathProfiling.c
  EdgeProfiling.c
  OptimalEdgeProfiling.c
  ValueProfiling.c
  SLGlobalProfiling.c
  MPIProfiling.c
  )

include_directories(../include ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS} -Wall)
if(OUTPUT_HASPID)
	add_definitions(-DOUTPUT_HASPID)
endif()
if(ENABLE_COMPRESS)
	add_definitions(-DENABLE_COMPRESS)
endif()

add_library( profile_rt-static ${SOURCES} )
set_target_properties( profile_rt-static
  PROPERTIES
  OUTPUT_NAME "profile_rt" )

install(TARGETS profile_rt-static #profile_rt-shared
	DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
