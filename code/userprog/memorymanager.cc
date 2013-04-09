// memorymanager.cc
//	A memory manager helper class. Used by address space to make nachos use contigous virtual memory. 

#include "memorymanager.h"
#include "machine.h"

//----------------------------------------------------------------------
// MemoryManager::MemoryManager
// 	Initialize a memory manager with a bitmap of "NumPhysPages"
//----------------------------------------------------------------------

MemoryManager::MemoryManager():bitMap(NumPhysPages){

}

//----------------------------------------------------------------------
// MemoryManager::~MemoryManager
//----------------------------------------------------------------------

MemoryManager::~MemoryManager() {
}

//----------------------------------------------------------------------
// MemoryManager::GetPage
//----------------------------------------------------------------------

int MemoryManager::GetPage() {
	return bitMap.Find();
}

//----------------------------------------------------------------------
// MemoryManager::ClearPage
// 	Take the index "i" of a page and clears this bit.
//----------------------------------------------------------------------

void MemoryManager::ClearPage(int i) {
	bitMap.Clear(i);
}

//----------------------------------------------------------------------
// MemoryManager::GetClearPages
// 	Return the number of clear pages
//----------------------------------------------------------------------

int MemoryManager::GetClearPages() {
	return bitMap.NumClear();
}
