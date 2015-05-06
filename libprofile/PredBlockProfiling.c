#include "Profiling.h"
#include <stdlib.h>

static uint64_t *ArrayStart;
static uint64_t NumElements;

static void PredBlockProfAtExitHandler(void) {
  write_profiling_data_long(BlockInfo64, ArrayStart, NumElements);
}

int llvm_start_pred_block_profiling(int argc, const char** argv,
                                    uint64_t* arrayStart, uint64_t numElements)
{
  int Ret = save_arguments(argc, argv);
  ArrayStart = arrayStart;
  NumElements = numElements;
  atexit(PredBlockProfAtExitHandler);
  return Ret;
}
