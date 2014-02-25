
add_definitions(${LLVM_DEFINITIONS})
include_directories(../include)

llvm_map_components_to_libraries(LLVM_LINK_COMPONENTS bitreader analysis)

add_executable(llvm-prof
  llvm-prof.cpp
  )

target_link_libraries(llvm-prof
	${LLVM_LINK_COMPONENTS}
	${LLVM_LIBRARIES}
	LLVMProfiling-shared
	)
