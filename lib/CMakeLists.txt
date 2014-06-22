set(SOURCES
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
	ProfileInfoLoaderPass.cpp
	ProfileVerifierPass.cpp
	ProfilingUtils.cpp
	)
#some platform need disable rtti to void 
#undefined reference `typeinfo for xxx`
add_definitions(-fno-rtti)
#disable debug ability
add_definitions(${LLVM_DEFINITIONS} -DNDEBUG -Wall)
include_directories(
	${LLVM_INCLUDE_DIRS}
	../include
	)
link_directories(${LLVM_LIBRARY_DIRS})

add_library(LLVMProfiling-static STATIC
	${SOURCES}
	)

if(DYNAMIC_LINK)
	set(LLVM_PROF_LIBRARY ${LLVM_DYNAMIC_LIBRARY})
else()
	llvm_map_components_to_libraries(LLVM_PROF_LIBRARY bitreader analysis transformutils)
endif()

target_link_libraries(LLVMProfiling-static
	${LLVM_PROF_LIBRARY}
	${LLVM_LIBRARY}
	)

set_target_properties(LLVMProfiling-static
	PROPERTIES
	OUTPUT_NAME "LLVMProfiling" )

add_library(LLVMProfiling-shared SHARED
	${SOURCES}
	)
target_link_libraries(LLVMProfiling-shared
	${LLVM_PROF_LIBRARY}
	${LLVM_LIBRARY}
	)

set_target_properties(LLVMProfiling-shared
	PROPERTIES
	OUTPUT_NAME "LLVMProfiling" )

install(TARGETS LLVMProfiling-static LLVMProfiling-shared
	DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
