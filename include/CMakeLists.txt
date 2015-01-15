set(INCLUDES
   ValueUtils.h
	ValueProfiling.h
	InitializeProfilerPass.h
	PathNumbering.h
	PathProfileInfo.h
	PathV2.h
	ProfileDataLoader.h
	ProfileDataTypes.h
	ProfileInfo.h
	ProfileInfoLoader.h
	ProfileInfoTypes.h
	ProfileInstrumentations.h
	ProfileInfoWriter.h
	ProfileInfoMerge.h
   TimingSource.h
   ProfilingUtils.h
   Profiling.h # for libprofile
	)

install(FILES ${INCLUDES}
	DESTINATION ${CMAKE_INSTALL_PREFIX}/include/llvm-prof)
