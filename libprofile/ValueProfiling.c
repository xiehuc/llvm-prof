#include "Profiling.h"
#include <sys/queue.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define malloc0(sz) memset(malloc(sz),0,sz)

static unsigned *ArrayStart;
static unsigned NumElements;

typedef struct ValueItem{
	int value;
	SLIST_ENTRY(ValueItem) next;
}ValueItem;

typedef SLIST_HEAD(ValueLink, ValueItem) ValueLink;

static ValueLink* ValueHead = NULL;

void ValueProfAtExitHandler(void)
{
	write_profiling_data(ValueInfo, ArrayStart, NumElements);
	int* buffer = NULL;

	int OutFile = getOutFile();
	int i=0;
	for(i=0;i<NumElements;i++){
		ValueItem* item = NULL;
		size_t len = sizeof(int)*ArrayStart[i];
		buffer = NULL;
		if(SLIST_EMPTY(&ValueHead[i])){
			len = sizeof(int);
			buffer = malloc0(len);
			*buffer = -1;
		}else{
			buffer = malloc0(len);
			int* w = buffer+ArrayStart[i];
			SLIST_FOREACH(item, &ValueHead[i], next){
				*--w = item->value;
			}
		}
		if(write(OutFile,buffer,len)<0){
			fprintf(stderr,"error: unable to write to output file.");
			exit(0);
		}
		free(buffer);
	}
}

void llvm_profiling_trap_value(int index,int value,int isConstant)
{
	++ArrayStart[index];
	if(isConstant) return;
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
  atexit(ValueProfAtExitHandler);
  return Ret;
}
