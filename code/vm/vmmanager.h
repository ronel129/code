/*
 * VMManager.h
 *
 *  Created on: Dec 28, 2009
 *      Author: qi
 */

#ifndef VMMANAGER_H_
#define VMMANAGER_H_
#include "bitmap.h"
#include "addrspace.h"
#include "list.h"

#define SWAP_SIZE 512

class PhysicalPageInfo{
public :
	//time
	AddrSpace * space;
	int pageTableIndex;
};

class DiskPageInfo{
public :
	DiskPageInfo();
	void Add(TranslationEntry *);
	void MarkValid(bool);
	void MarkUse(bool);
	void MarkDirty(bool);
	void Remove(TranslationEntry *);
	TranslationEntry* GetAt(int index);
	void SetMemPage(int page);
private:
	int index;    //index in the swap file
	List * list;
};


class VirtualMemoryManager{
public:
	VirtualMemoryManager();
	~VirtualMemoryManager();
	int GetStoreLocation();
	PhysicalPageInfo* GetPageInfo(int index);
	void ClearSpace(AddrSpace * space);
	void BackStore(char * from,int size,int location);
	void LoadToMemory(char * to,int size,int location);
	void ReplacePage(int vitualAddr);
	void LoadPage(int vitualAddr);
	void IncreasePosition();
	DiskPageInfo *GetDiskPageInfo(int);
	void ChangeRepresentee(AddrSpace* space,int virAddr);
private:
	TranslationEntry* GetTranslationEntry(PhysicalPageInfo *);
private :
	BitMap bitMap;
	OpenFile *swapFile;   //the backing store
	PhysicalPageInfo * pageInfos;
	DiskPageInfo * shareInfos;
	int position;
};

#endif /* VMMANAGER_H_ */
