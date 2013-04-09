#include "sysopenfile.h"
#include "utilities.h"
  
//----------------------------------------------------------------------
// SysOpenFile::closeFile
//	close a SysOpenFile and accessing processes
//----------------------------------------------------------------------

void SysOpenFile::closeFile() {
	if (userOpens <= 0)
		return;
	userOpens--;
	if (userOpens == 0) {
		delete fileName;
		delete file;
	}
}

//----------------------------------------------------------------------
// SysOpenFile::closeAll
//	close all SysOpenFile and accessing processes
//----------------------------------------------------------------------

void SysOpenFile::closeAll() {
	if (userOpens <= 0)
		return;
	userOpens = 0;
	delete fileName;
	delete file;
}

//----------------------------------------------------------------------
// SysOpenFileManager::SysOpenFileManager
//----------------------------------------------------------------------

SysOpenFileManager::SysOpenFileManager() :
	bitMap(SYSOPENFILETABLE_SIZE) {
}

SysOpenFileManager::~SysOpenFileManager() {

}

//----------------------------------------------------------------------
// SysOpenFileManager::Add
//	Add a SysOpenFile to the array of all SysOpenFile.
//----------------------------------------------------------------------

int SysOpenFileManager::Add(SysOpenFile file) {
	int i = bitMap.Find();
	if (i < 0)
		return i;	//full
	sysOpenFileObjects[i] = file;
	return i;	//index of the added SysOpenFile
}

//----------------------------------------------------------------------
// SysOpenFileManager::Get
//	Get SysOpenFile by fileName and stored its index.
//----------------------------------------------------------------------

SysOpenFile* SysOpenFileManager::Get(char* fileName, int& index) 
{
	for (int i = 2; i < SYSOPENFILETABLE_SIZE; i++){
		
		if (bitMap.Test(i) && strcmp(fileName, sysOpenFileObjects[i].fileName)) {
			index = i;
			return &sysOpenFileObjects[i];  //Return the added SysOpenFile
		}
	}
	return NULL;
}

//----------------------------------------------------------------------
// SysOpenFileManager::Get(index)
//	Get SysOpenFile by index of the array
//----------------------------------------------------------------------

SysOpenFile* SysOpenFileManager::Get(int index) {
	if (bitMap.Test(index)) {
		return sysOpenFileObjects + index;  //Return the got SysOpenFile
	}
	return NULL;
}

//----------------------------------------------------------------------
// SysOpenFileManager::closeFile
//	Close a SysOpenFile by index of the array
//----------------------------------------------------------------------

void SysOpenFileManager::closeFile(int index) {
	ASSERT(bitMap.Test(index));
	sysOpenFileObjects[index].closeFile();
	if (sysOpenFileObjects[index].userOpens == 0)
		bitMap.Clear(index);
}
