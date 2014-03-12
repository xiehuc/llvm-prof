#some platform need disable rtti to void 
#undefined reference `typeinfo for xxx`
add_definitions(-fno-rtti)
add_definitions(${LLVM_DEFINITIONS} -Wall)
include_directories(
    ${LLVM_INCLUDE_DIRS}
    ../include 
    )

link_directories(${LLVM_LIBRARY_DIRS})

if(DYNAMIC_LINK)
	set(LLVM_PROF_LIBRARY ${LLVM_DYNAMIC_LIBRARY})
else()
	llvm_map_components_to_libraries(LLVM_PROF_LIBRARY bitreader analysis )
endif()

add_executable(llvm-prof
	llvm-prof.cpp
	)

target_link_libraries(llvm-prof
	${LLVM_LIBRARIES}
	LLVMProfiling-shared
	#${LLVM_PROF_LIBRARY}
	)

install(TARGETS llvm-prof
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
	)
