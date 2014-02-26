
add_definitions(${LLVM_DEFINITIONS})
include_directories(../include)

llvm_map_components_to_libraries(LLVM_LINK_COMPONENTS bitreader )

add_executable(llvm-prof
  llvm-prof.cpp
  )

target_link_libraries(llvm-prof
	${LLVM_LIBRARIES}
	${LLVM_DYNAMIC_LIBRARY}
	LLVMProfiling-shared
	)

install(TARGETS llvm-prof
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
	)
