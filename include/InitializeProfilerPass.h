#ifndef INITIALIZE_PROFILER_PASS_H_H
#define INITIALIZE_PROFILER_PASS_H_H
namespace llvm{
void initializeEdgeProfilerPass(PassRegistry&);
void initializePathProfilerPass(PassRegistry&);
void initializeGCOVProfilerPass(PassRegistry&);

void initializeProfileMetadataLoaderPassPass(PassRegistry&);
void initializePathProfileLoaderPassPass(PassRegistry&);

void initializeNoProfileInfoPass(PassRegistry&);
void initializeNoPathProfileInfoPass(PassRegistry&);

void initializeOptimalEdgeProfilerPass(PassRegistry&);

void initializeProfileEstimatorPassPass(PassRegistry&);

void initializeProfileInfoAnalysisGroup(PassRegistry&);
void initializePathProfileInfoAnalysisGroup(PassRegistry&);
void initializePathProfileVerifierPass(PassRegistry&);
void initializeProfileVerifierPassPass(PassRegistry&);


}
#endif
