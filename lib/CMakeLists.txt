set(SOURCES
	EdgeProfiling.cpp
	GCOVProfiling.cpp
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

#disable debug ability
add_definitions(${LLVM_DEFINITIONS} -DNDEBUG)
include_directories(
	${LLVM_INCLUDE_DIRS}
	../include
	)
llvm_map_components_to_libraries(LLVM_CORE_LIBS 
	analysis transformutils)

add_library(LLVMProfiling-static STATIC
	${SOURCES}
	)
target_link_libraries(LLVMProfiling-static
	${LLVM_CORE_LIBS}
	)

set_target_properties(LLVMProfiling-static
	PROPERTIES
	OUTPUT_NAME "LLVMProfiling" )

add_library(LLVMProfiling-shared SHARED
	${SOURCES}
	)
target_link_libraries(LLVMProfiling-shared
	${LLVM_CORE_LIBS}
	)

set_target_properties(LLVMProfiling-shared
	PROPERTIES
	OUTPUT_NAME "LLVMProfiling" )
