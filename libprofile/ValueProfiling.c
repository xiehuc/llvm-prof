#include "Profiling.h"
#include <sys/queue.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#define malloc0(sz) memset(malloc(sz),0,sz)
#define trunk_size 100

static unsigned *ArrayStart;
static unsigned NumElements;
//record true write value content count.
static unsigned *WriteCount;

typedef struct ValueItem{
	SLIST_ENTRY(ValueItem) next;
	int value[trunk_size];
}ValueItem;
typedef SLIST_HEAD(ValueEntry,ValueItem) ValueEntry;

typedef struct ValueHead{
	int flags;
	int pos;
	ValueEntry entry;
}ValueHead;

static ValueHead* ValueLink = NULL;

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
		ValueEntry* entry = &ValueLink[i].entry;
		int* w = buffer+WriteCount[i];
		SLIST_FOREACH(item, entry, next){
			if(item == SLIST_FIRST(entry)){
				int size = ValueLink[i].pos;
				memcpy(w-size,item->value,sizeof(int)*size);
				w-=size;
			}else{
				memcpy(w-trunk_size,item->value,sizeof(int)*trunk_size);
				w-=trunk_size;
			}
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
	int pos = ValueLink[index].pos;
	ValueEntry* entry = &ValueLink[index].entry;
	if(pos >= trunk_size || SLIST_EMPTY(entry)){
		ValueItem* ins = malloc0(sizeof(ValueItem));
		SLIST_INSERT_HEAD(entry, ins, next);
		pos = ValueLink[index].pos = 0;
	}
	ValueItem* item = SLIST_FIRST(entry);
	item->value[pos] = value;
	++ValueLink[index].pos;
}

int llvm_start_value_profiling(int argc, const char **argv,
                              unsigned *arrayStart, unsigned numElements) {
  int Ret = save_arguments(argc, argv);
  ArrayStart = arrayStart;
  NumElements = numElements;
  ValueLink = malloc0(sizeof(*ValueLink)*NumElements);
  WriteCount = malloc0(sizeof(unsigned)*NumElements);
  atexit(ValueProfAtExitHandler);
  return Ret;
}
