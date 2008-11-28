#ifndef MEMORY_H_
#define MEMORY_H_

#define USEMEMORYMANAGER 1
#include "../Utility/MemoryManager/MemoryTemplates.h"

#ifdef USEMEMORYMANAGER
#include "../Utility/MemoryManager/mmgr.h"
#else
#include "../Utility/MemoryManager/nommgr.h"
#endif

#endif