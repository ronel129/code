#ifndef SYSOPENFILE_H_
#define SYSOPENFILE_H_
#define SYSOPENFILETABLE_SIZE 200
#include "filesys.h"
#include "bitmap.h"

class SysOpenFile{
public:
	char * fileName;
	OpenFile *file;	// pointer to the file system's OpenFile object
	int userOpens;	// Number of user processes accessing
	void closeFile();	//close a SysOpenFile
	void closeAll();		//close all SysOpenFile
};


class SysOpenFileManager{
public:
	SysOpenFileManager();			
	~SysOpenFileManager();			
	int Add(SysOpenFile newSysOpenFile); 	//Add a SysOpenFile object to the array
	SysOpenFile* Get(char* fileName,int& index); // Get SysOpenFile object by fileName
	SysOpenFile* Get(int index);		// Get SysOpenFile object by index of the array
	void closeFile(int index);			// Close a SysOpenFile object by index

private:
	SysOpenFile sysOpenFileObjects[SYSOPENFILETABLE_SIZE]; // array for all SysOpenFile objects
	BitMap bitMap;				// a BitMap for SysOpenFile manager methods
};

#endif /* SYSOPENFILE_H_ */
