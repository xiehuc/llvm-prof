set(SOURCE
	EdgeProfiling.cpp
	GCOVProfiling.cpp
	OptimalEdgeProfiling.cpp
	PathProfileInfo.cpp
	PathProfileVerifier.cpp
	PathProfiling.cpp
	ProfileDataLoader.cpp
	ProfileDataLoaderPass.cpp
	ProfileEstimatorPass.cpp
	ProfileInfo.cpp
	ProfileInfoLoader.cpp
	ProfileInfoLoaderPass.cpp
	ProfileVerifierPass.cpp
	ProfileUtils.cpp
	)

include_directories(../include)

add_library(LLVMProfiling-static static
	${SOURCES}
	)

set_target_properties(LLVMProfiling-static
	PROPERTIES
	OUTPUT_NAME "LLVMProfiling" )

add_library(LLVMProfiling-shared shared
	${SOURCE}
	)

set_target_properties(LLVMProfiling-shared
	PROPERTIES
	OUTPUT_NAME "LLVMProfiling" )
