set(SOURCES
   ValueUtils.cpp
	ValueProfiling.cpp
   SLGlobalProfiling.cpp
	EdgeProfiling.cpp
	#GCOVProfiling.cpp					#seems llvm 3.4 keeps gcov profiling
	OptimalEdgeProfiling.cpp
	PathProfileInfo.cpp
	PathProfileVerifier.cpp
	PathProfiling.cpp
	PathNumbering.cpp
	ProfileDataLoader.cpp
	ProfileDataLoaderPass.cpp
	ProfileEstimatorPass.cpp
	ProfileInfo.cpp
	ProfileInfoLoader.cpp
   ProfileInfoWriter.cpp
	ProfileInfoLoaderPass.cpp
	ProfileVerifierPass.cpp
	ProfilingUtils.cpp
   TimingSource.cpp
   MPIProfiling.cpp
   PredBlockProfiling.cpp
   datatype.h
	)
#some platform need disable rtti to void 
#undefined reference `typeinfo for xxx`
add_definitions(-Wall -fno-rtti -std=c++11)
set(SELF ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(
	${LLVM_INCLUDE_DIRS}
	../include
   ${CMAKE_CURRENT_BINARY_DIR}
	)
link_directories(${LLVM_LIBRARY_DIRS})

add_custom_command(OUTPUT datatype.h 
   COMMAND ${MPIF90} ${SELF}/data.f -o datatype
   COMMAND ./datatype > datatype.h
   DEPENDS ${SELF}/data.f
   )
add_library(LLVMProfiling-static STATIC
	${SOURCES}
	)
target_link_libraries(LLVMProfiling-static
	${LLVM_LIBRARY}
	)
set_target_properties(LLVMProfiling-static
	PROPERTIES
	OUTPUT_NAME "LLVMProfiling" )

add_library(LLVMProfiling-shared SHARED
	${SOURCES}
	)
target_link_libraries(LLVMProfiling-shared
	${LLVM_LIBRARY}
	)
set_target_properties(LLVMProfiling-shared
	PROPERTIES
	OUTPUT_NAME "LLVMProfiling" )

install(TARGETS LLVMProfiling-static LLVMProfiling-shared
   DESTINATION ${LIB_DIRS})
