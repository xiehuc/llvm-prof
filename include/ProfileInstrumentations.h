#ifndef PROFILE_INSTRUMENTATIONS_H_H
#define PROFILE_INSTRUMENTATIONS_H_H

#include "llvm/ADT/StringRef.h"

namespace llvm
{
class ModulePass;

//===--------------------------------------------------------------------===//
//
// createProfileLoaderPass - This pass loads information from a profile dump
// file.
//
ModulePass *createProfileLoaderPass();
extern char &ProfileLoaderPassID;

//===--------------------------------------------------------------------===//
//
// createProfileMetadataLoaderPass - This pass loads information from a
// profile dump file and sets branch weight metadata.
//
ModulePass *createProfileMetadataLoaderPass();
extern char &ProfileMetadataLoaderPassID;

//===--------------------------------------------------------------------===//
//
// createNoProfileInfoPass - This pass implements the default "no profile".
//
ImmutablePass *createNoProfileInfoPass();

//===--------------------------------------------------------------------===//
//
// createProfileEstimatorPass - This pass estimates profiling information
// instead of loading it from a previous run.
//
FunctionPass *createProfileEstimatorPass();
extern char &ProfileEstimatorPassID;

//===--------------------------------------------------------------------===//
//
// createProfileVerifierPass - This pass verifies profiling information.
//
FunctionPass *createProfileVerifierPass();

//===--------------------------------------------------------------------===//
//
// createPathProfileLoaderPass - This pass loads information from a path
// profile dump file.
//
ModulePass *createPathProfileLoaderPass();
extern char &PathProfileLoaderPassID;

//===--------------------------------------------------------------------===//
//
// createNoPathProfileInfoPass - This pass implements the default
// "no path profile".
//
ImmutablePass *createNoPathProfileInfoPass();

//===--------------------------------------------------------------------===//
//
// createPathProfileVerifierPass - This pass verifies path profiling
// information.
//
ModulePass *createPathProfileVerifierPass();

// Insert edge profiling instrumentation
ModulePass *createEdgeProfilerPass();

// Insert optimal edge profiling instrumentation
ModulePass *createOptimalEdgeProfilerPass();

// Insert path profiling instrumentation
ModulePass *createPathProfilerPass();

#if 0
// Insert GCOV profiling instrumentation
struct GCOVOptions {
  static GCOVOptions getDefault();

  // Specify whether to emit .gcno files.
  bool EmitNotes;

  // Specify whether to modify the program to emit .gcda files when run.
  bool EmitData;

  // A four-byte version string. The meaning of a version string is described in
  // gcc's gcov-io.h
  char Version[4];

  // Emit a "cfg checksum" that follows the "line number checksum" of a
  // function. This affects both .gcno and .gcda files.
  bool UseCfgChecksum;

  // Add the 'noredzone' attribute to added runtime library calls.
  bool NoRedZone;

  // Emit the name of the function in the .gcda files. This is redundant, as
  // the function identifier can be used to find the name from the .gcno file.
  bool FunctionNamesInData;
};
ModulePass *createGCOVProfilerPass(const GCOVOptions &Options =
                                   GCOVOptions::getDefault());
#endif
}
#endif
