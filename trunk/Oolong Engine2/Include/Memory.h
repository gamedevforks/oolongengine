#ifndef MEMORY_H_
#define MEMORY_H_

#define USEMEMORYMANAGER
#include "../Utility/MemoryManager/MemoryTemplates.h"
//#include "../Utility/MemoryManager/HeapFactory.h"

#ifdef USEMEMORYMANAGER
#include "../Utility/MemoryManager/mmgr.h"
#else
#include "../Utility/MemoryManager/nommgr.h"
#endif

#endif