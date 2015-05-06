/*===-- ProfileDataTypes.h - Profiling info shared constants --------------===*\
|*
|*                     The LLVM Compiler Infrastructure
|*
|* This file is distributed under the University of Illinois Open Source
|* License. See LICENSE.TXT for details.
|*
|*===----------------------------------------------------------------------===*|
|*
|* This file defines constants shared by the various different profiling
|* runtime libraries and the LLVM C++ profile metadata loader. It must be a
|* C header because, at present, the profiling runtimes are written in C.
|*
\*===----------------------------------------------------------------------===*/

#ifndef LLVM_ANALYSIS_PROFILEDATATYPES_H
#define LLVM_ANALYSIS_PROFILEDATATYPES_H

/* Included by libprofile. */
#if defined(__cplusplus)
extern "C" {
#endif

/* TODO: Strip out unused entries once ProfileInfo etc has been removed. */
enum ProfilingType {
	ArgumentInfo = 1,   /* The command line argument block */
	FunctionInfo = 2,   /* Function profiling information  */
	BlockInfo    = 3,   /* Block profiling information     */
	EdgeInfo     = 4,   /* Edge profiling information      */
	PathInfo     = 5,   /* Path profiling information      */
	BBTraceInfo  = 6,   /* Basic block trace information   */
	OptEdgeInfo  = 7,   /* Edge profiling information, optimal version */
	ValueInfo    = 100, /* Value profiling information     */
   SLGInfo      = 101, /* StoreLoadGlobal profiling info  */
   MPInfo       = 102, /* MPI Inst profiling info         */
   MPIFullInfo  = 103, /* an expand MPI Inst profiling info contain datatype */
   BlockInfo64  = 104, /* Block profiling information with 64bit */
   EdgeInfo64   = 105, /* Edge Profiling information with 64bit */
};

// special flags used in value profiling
enum ProfilingFlags {
	CONSTANT_COMPRESS = 1<<0,
	RUN_LENGTH_COMPRESS = 1<<1
};

#define FORTRAN_DATATYPE_MAP_SIZE 128

#if defined(__cplusplus)
}
#endif

#endif /* LLVM_ANALYSIS_PROFILEDATATYPES_H */
