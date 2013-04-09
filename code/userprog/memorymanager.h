// memorymanager.h
//	Helper methos for defining in Memory Manager that will be used to make nachos use contiguous virtual memory.

#ifndef MEMORYMANAGER_H_
#define MEMORYMANAGER_H_
#include "bitmap.h"


class MemoryManager {
private:
	BitMap bitMap;	// a BitMap for memory manager methods
public:
	MemoryManager();	
	~MemoryManager();	
	int GetPage();		
	void ClearPage(int i);	
	int GetClearPages();
};

#endif /* MEMORYMANAGER_H_ */
