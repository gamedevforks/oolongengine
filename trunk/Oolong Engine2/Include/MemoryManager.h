#ifndef MEMORY_H_
#define MEMORY_H_

#ifdef __APPLE__
#include <TargetConditionals.h>

#if((TARGET_IPHONE_SIMULATOR == 0) && (TARGET_OS_IPHONE == 1) && (!_NOMEMORYMANAGER))
#include "../Utility/MemoryManager/mmgr.h"
#else
#include "../Utility/MemoryManager/nommgr.h"
#endif

#include "../Utility/MemoryManager/MemoryTemplates.h"

#endif
#endif