/*
 * VMManager.cc
 *
 *  Created on: Dec 28, 2009
 *      Author: qi
 *      Modified by Hyper: Dec 29 2009
 */

#include "system.h"
#include "vmmanager.h"
VirtualMemoryManager::VirtualMemoryManager() :
	bitMap(SWAP_SIZE) {
	fileSystem->Create("SWAP", PageSize * SWAP_SIZE);
	swapFile = fileSystem->Open("SWAP");
	pageInfos = new PhysicalPageInfo[NumPhysPages];
	shareInfos = new DiskPageInfo[SWAP_SIZE];
	position = 0;//the clock pointer starting position
}

VirtualMemoryManager::~VirtualMemoryManager() {
	fileSystem->Remove("SWAP");
	delete swapFile;
	delete pageInfos;
	delete shareInfos;
}

int VirtualMemoryManager::GetStoreLocation() {
	return bitMap.Find() * PageSize;
}

void VirtualMemoryManager::BackStore(char * from, int size, int location) {
	swapFile->WriteAt(from, size, location);
}

void VirtualMemoryManager::ClearSpace(AddrSpace * space) {
	int emptyIndex = NumPhysPages;
	for (int i = 0; i < NumPhysPages; i++) {
		if (pageInfos[i].space == space) {
			TranslationEntry* entry = space->GetTranslationEntry(pageInfos[i].pageTableIndex);
			this->ChangeRepresentee(space,pageInfos[i].pageTableIndex*PageSize);
		}
		if(pageInfos[i].space == NULL)
			emptyIndex=min(i,emptyIndex);

	}
	for (int i = 0; i < NumPhysPages; i++) {
		if (pageInfos[i].space != NULL && i > emptyIndex) {
			pageInfos[emptyIndex] = pageInfos[i];
			pageInfos[i].space = NULL;
			emptyIndex++;
		}
	}
	for (int i = 0; i < space->GetNumPages(); i++) {
		TranslationEntry* entry = space->GetTranslationEntry(i);
			DiskPageInfo* shareInfo = shareInfos+entry->diskLoc/PageSize;
			shareInfo->Remove(entry);
		if(entry->readOnly == FALSE){
			if (entry->valid == TRUE){
				memManager->ClearPage(entry->physicalPage);
				printf("E %d: %d\n",entry->space->pcb->pid,entry->virtualPage);
			}
			bitMap.Clear(entry->diskLoc / PageSize);
		}
	}
}

void VirtualMemoryManager::LoadToMemory(char * to, int size, int location) {
	swapFile->ReadAt(to, size, location);
}

TranslationEntry* VirtualMemoryManager::GetTranslationEntry(
		PhysicalPageInfo * info) {
	return info->space->GetTranslationEntry(info->pageTableIndex);
}
/**
 * Choose a page and put it to the disk, and load the demanding page to the physical memory.
 */
void VirtualMemoryManager::ReplacePage(int vitualAddr) {
	TranslationEntry* entry;
	while (true) {
		PhysicalPageInfo& info = pageInfos[position];//The space group occupying the page
		if (info.space == NULL) {
			info.space = currentThread->space;
			info.pageTableIndex = vitualAddr / PageSize;
			entry = GetTranslationEntry(&pageInfos[position]);
			entry->physicalPage = memManager->GetPage();
			break;
		}
		entry = GetTranslationEntry(&info);//The represent entry
		DiskPageInfo * shareInfo = &shareInfos[entry->diskLoc / PageSize];
		if (entry->use == FALSE) {
			if (entry->dirty == TRUE){
				printf("S %d: %d\n",entry->space->pcb->pid,entry->virtualPage);
				this->BackStore(machine->mainMemory + entry->physicalPage
						* PageSize, PageSize, entry->diskLoc);
			}
			else
				printf("E %d: %d\n",entry->space->pcb->pid,entry->virtualPage);
			shareInfo->MarkValid(FALSE);
			shareInfo->MarkUse(FALSE);
			info.space = currentThread->space;
			info.pageTableIndex = vitualAddr / PageSize;
			GetTranslationEntry(&info)->physicalPage = entry->physicalPage;
			break;
		} else {
			shareInfo->MarkUse(FALSE);
			IncreasePosition();
		}
	}
	IncreasePosition();
	LoadPage(vitualAddr);
}

void VirtualMemoryManager::IncreasePosition() {
	position = (position + 1) % NumPhysPages;
}

void VirtualMemoryManager::LoadPage(int virtualAddr) {
	TranslationEntry* entry = currentThread->space->GetTranslationEntry(
			virtualAddr / PageSize);
	this->LoadToMemory(machine->mainMemory + entry->physicalPage * PageSize,
			PageSize, entry->diskLoc);
	DiskPageInfo * shareInfo = &shareInfos[entry->diskLoc / PageSize];
	shareInfo->MarkValid(TRUE);
	shareInfo->SetMemPage(entry->physicalPage);
}

void VirtualMemoryManager::ChangeRepresentee(AddrSpace* space,int virAddr){
	TranslationEntry* entry = space->GetTranslationEntry(virAddr/PageSize);
	DiskPageInfo* shareInfo = shareInfos+entry->diskLoc/PageSize;
	shareInfo->Remove(entry);
	PhysicalPageInfo* pageInfo = NULL;
	for(int i=0;i<NumPhysPages;i++){
		pageInfo = pageInfos+i;
		if(pageInfo->space == space&&pageInfo->pageTableIndex == virAddr/PageSize)
			break;
		pageInfo = NULL;
	}
	if(pageInfo == NULL)
		return;
	TranslationEntry* newRep = shareInfo->GetAt(0);
	if(newRep == NULL)
		pageInfo->space = NULL;
		else{
	pageInfo->space = newRep->space;
	pageInfo->pageTableIndex = newRep->space->GetTransEntryIndex(newRep);
		}
}
DiskPageInfo * VirtualMemoryManager::GetDiskPageInfo(int index) {
	return &shareInfos[index];
}

DiskPageInfo::DiskPageInfo() {
	list = new List();
}

void DiskPageInfo::Add(TranslationEntry *entry) {
	list->Append(entry);
}

void DiskPageInfo::MarkValid(bool flag) {
	TranslationEntry *entry;
	for (int i = 0; i < list->Size(); i++) {
		entry = (TranslationEntry *) (list->GetAt(i));
		entry->valid = flag;
	}
}

void DiskPageInfo::MarkUse(bool flag) {
	TranslationEntry *entry;
	for (int i = 0; i < list->Size(); i++) {
		entry = (TranslationEntry *) (list->GetAt(i));
		entry->use = flag;
	}
}


void DiskPageInfo::Remove(TranslationEntry *entry){
	list->Remove(entry);
	if(list->Size() == 1)
		((TranslationEntry *) list->Top())->readOnly = FALSE;
}

TranslationEntry* DiskPageInfo::GetAt(int index){
	return (TranslationEntry*)this->list->GetAt(index);
}

void DiskPageInfo::SetMemPage(int page){
	TranslationEntry *entry;
		for (int i = 0; i < list->Size(); i++) {
			entry = (TranslationEntry *) (list->GetAt(i));
			entry->physicalPage=page;
		}
}

void DiskPageInfo::MarkDirty(bool flag){
	TranslationEntry *entry;
		for (int i = 0; i < list->Size(); i++) {
			entry = (TranslationEntry *) (list->GetAt(i));
			entry->dirty = flag;
		}
}
