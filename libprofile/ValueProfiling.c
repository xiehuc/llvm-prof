#include "Profiling.h"
#include <sys/queue.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#define malloc0(sz) memset(malloc(sz),0,sz)

static unsigned *ArrayStart;
static unsigned NumElements;
//record true write value content count.
static unsigned *WriteCount;

typedef struct ValueItem{
	int value;
	SLIST_ENTRY(ValueItem) next;
}ValueItem;

typedef SLIST_HEAD(ValueLink, ValueItem) ValueLink;

static ValueLink* ValueHead = NULL;

void ValueProfAtExitHandler(void)
{
#define EXIT_ON_ERROR {\
	fprintf(stderr,"error: unable to write to output file.");\
	exit(0); }

	int* buffer = NULL;
	int OutFile = getOutFile();
	write_profiling_data(ValueInfo, ArrayStart, NumElements);
	write_profiling_data(ValueContent, WriteCount, NumElements);
	int i=0;
	for(i=0;i<NumElements;i++){
		size_t len = sizeof(int)*WriteCount[i];
		if(len==0) continue;
		buffer = malloc0(len);
		ValueItem* item = NULL;
		int* w = buffer+WriteCount[i];
		SLIST_FOREACH(item, &ValueHead[i], next){
			*--w = item->value;
		}
		if(write(OutFile,buffer,len)<0)
			EXIT_ON_ERROR;
		free(buffer);
	}
	close(OutFile);
#undef EXIT_ON_ERROR
}

void llvm_profiling_trap_value(int index,int value,int isConstant)
{
	++ArrayStart[index];
	if(isConstant) return;
	++WriteCount[index];
	ValueItem* item = malloc0(sizeof(*item));
	item->value = value;
	SLIST_INSERT_HEAD(&ValueHead[index], item, next);
}

int llvm_start_value_profiling(int argc, const char **argv,
                              unsigned *arrayStart, unsigned numElements) {
  int Ret = save_arguments(argc, argv);
  ArrayStart = arrayStart;
  NumElements = numElements;
  ValueHead = malloc0(sizeof(ValueLink)*NumElements);
  WriteCount = malloc0(sizeof(unsigned)*NumElements);
  atexit(ValueProfAtExitHandler);
  return Ret;
}
