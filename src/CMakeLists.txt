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

set(SELF ${CMAKE_CURRENT_SOURCE_DIR})
set(CLANG "clang-${LLVM_RECOMMAND_VERSION}")
add_custom_command(OUTPUT inst-timing
   COMMAND ${CLANG} -O0 -DTIMING_${TIMING} ${SELF}/inst-timing.c -emit-llvm -c -o /tmp/inst-timing.bc
   COMMAND ${LLVM_OPT} -load ${PROJECT_BINARY_DIR}/lib/libLLVMProfiling.so -InstTemplate /tmp/inst-timing.bc -o /tmp/inst-timing.1.bc
   COMMAND ${CLANG} -O0 /tmp/inst-timing.1.bc -o inst-timing -lm
   DEPENDS ${SELF}/inst-timing.c ${SELF}/libtiming.c ${SELF}/../lib/InstTemplate.cpp
   )
add_custom_target(InstTiming ALL DEPENDS inst-timing)

install(TARGETS llvm-prof
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
	)
