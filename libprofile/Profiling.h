/*===-- Profiling.h - Profiling support library support routines ----------===*\
|*
|*                     The LLVM Compiler Infrastructure
|*
|* This file is distributed under the University of Illinois Open Source
|* License. See LICENSE.TXT for details.
|*
|*===----------------------------------------------------------------------===*|
|*
|* This file defines functions shared by the various different profiling
|* implementations.
|*
\*===----------------------------------------------------------------------===*/

#ifndef PROFILING_H
#define PROFILING_H

#include "ProfileDataTypes.h" /* for enum ProfilingType */
#include <stdint.h>

/* save_arguments - Save argc and argv as passed into the program for the file
 * we output.
 */
int save_arguments(int argc, const char **argv);

/*
 * Retrieves the file descriptor for the profile file.
 */
int getOutFile();

/* write_profiling_data - Write out a typed packet of profiling data to the
 * current output file.
 */
void write_profiling_data(enum ProfilingType PT, unsigned* Start,
                          unsigned NumElements);
void write_profiling_data_long(enum ProfilingType PT, uint64_t* Start,
                               uint64_t NumElements);

#endif
