/*===-- EdgeProfiling.c - Support library for edge profiling --------------===*\
|*
|*                     The LLVM Compiler Infrastructure
|*
|* This file is distributed under the University of Illinois Open Source      
|* License. See LICENSE.TXT for details.                                      
|* 
|*===----------------------------------------------------------------------===*|
|* 
|* This file implements the call back routines for the edge profiling
|* instrumentation pass.  This should be used with the -insert-edge-profiling
|* LLVM pass.
|*
\*===----------------------------------------------------------------------===*/

#include "Profiling.h"
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>

static unsigned *ArrayStart;
static unsigned NumElements;

/* EdgeProfAtExitHandler - When the program exits, just write out the profiling
 * data.
 */
static void MPIProfAtExitHandler(void) {
  /* Note that if this were doing something more intelligent with the
   * instrumentation, we could do some computation here to expand what we
   * collected into simple edge profiles.  Since we directly count each edge, we
   * just write out all of the counters directly.
   */
  unsigned* MapTable = ArrayStart + NumElements;
  unsigned* VisitTable = MapTable + FORTRAN_DATATYPE_MAP_SIZE;
  unsigned i;
  for(i=0;i<FORTRAN_DATATYPE_MAP_SIZE;++i){
    if(*VisitTable++ == 1 && *MapTable++ == 0)
      fprintf(stderr, "WARNNING: doesn't consider MPI Fortran Type %d\n", i);
  }
  write_profiling_data(MPIFullInfo, ArrayStart, NumElements);
}

static int init_datatype_map(uint32_t* DT)
{
   memset(DT, 0, sizeof(uint32_t) * FORTRAN_DATATYPE_MAP_SIZE * 2);
#include "datatype.h"
   return 0;
}


/* llvm_start_edge_profiling - This is the main entry point of the edge
 * profiling library.  It is responsible for setting up the atexit handler.
 */
int llvm_start_mpi_profiling(int argc, const char **argv,
                              unsigned *arrayStart, unsigned numElements) {
  int Ret = save_arguments(argc, argv);
  ArrayStart = arrayStart;
  NumElements = numElements - FORTRAN_DATATYPE_MAP_SIZE * 2;
  init_datatype_map(ArrayStart + NumElements);
  atexit(MPIProfAtExitHandler);
  return Ret;
}
