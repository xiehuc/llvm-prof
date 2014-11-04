#some platform need disable rtti to void 
#undefined reference `typeinfo for xxx`
add_definitions(-fno-rtti)
add_definitions(${LLVM_DEFINITIONS} -Wall)
if(LLVM_VERSION VERSION_GREATER "3.4")
add_definitions(-std=c++11)
endif()
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
   printer.cpp
   passes.cpp
	)

target_link_libraries(llvm-prof
	${LLVM_LIBRARIES}
   ${LLVM_PROF_LIBRARY}
	LLVMProfiling-shared
	)

install(TARGETS llvm-prof
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
	)
