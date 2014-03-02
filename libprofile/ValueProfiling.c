#include "Profiling.h"
#include <sys/queue.h>
#include <stdlib.h>

static unsigned *ArrayStart;
static unsigned NumElements;

typedef struct ValueItem{
	long value;
	SLIST_ENTRY(ValueItem) next;
}ValueItem;

typedef SLIST_HEAD(ValueLink, ValueItem) ValueLink;

ValueLink* ValueHead = NULL;

void llvm_profiling_trap_value(int index,long value)
{
	ValueItem* item = memset(malloc(sizeof(*item)),0,sizeof(*item));
	item->value = value;
	SLIST_INSERT_AFTER(ValueHead[index], item, next);
}

int llvm_start_value_profiling(int argc, const char **argv,
                              unsigned *arrayStart, unsigned numElements) {
  int Ret = save_arguments(argc, argv);
  ArrayStart = arrayStart;
  NumElements = numElements;
  ValueHead = memset(calloc(NumElements,  sizeof(ValueLink)), 0,
		  sizeof(ValueLink)*NumElements);
  atexit(EdgeProfAtExitHandler);
  return Ret;
}
