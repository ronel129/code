// processmanager.h
// Process Manager class that helps facility PCB operations. It keeps necessary information about a process.
// Includes PID, parent PID, status, etc. 

#ifndef PROCESSMANAGER_H_
#define PROCESSMANAGER_H_
#define MAX_PROCESS PageSize
#define MAX_FILE_OPEN 25
#include "bitmap.h"
#include "thread.h"
#include "synch.h"
#include "useropenfile.h"

// Definitions thread status
#define PRUNNING 2;
#define	PBLOCKED 3;
#define GOOD 0;
#define BAD 1;

class PCB {
public:
	int pid;	
	int parentPid;	
	Thread* thread;	
	int status;	
	PCB();	
	~PCB();	
	int Add(UserOpenFile uoFile);	
	bool Remove(int fileId);	
	UserOpenFile* Get(int fileId);	
private:
	UserOpenFile userOpenFileTable[MAX_FILE_OPEN];	
	BitMap bitMap;	// a BitMap for PCB methods
};


class ProcessManager {
public:
	ProcessManager();	
	~ProcessManager();	
	int GetPID();	
	void ClearPID(int pid);	
	void AddProcessAt(PCB *pcb,int pid);	
	void Join(int pid);		
	void Broadcast(int pid);	// Used by Exit to broadcast the PID. 
	int GetStatus(int pid); 
private:
	BitMap bitMap;	
	PCB** pcbs;	
	Condition **conditions; //condition for PCBs join and exit
	Lock ** locks;	// Lock for join and exit
	int pcbStatus[MAX_PROCESS]; 
	int joinProNum[MAX_PROCESS];	
};
#endif /* PROCESSMANAGER_H_ */
