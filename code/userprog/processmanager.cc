// processmanager.cc
// Process Manager class that helps facility PCB operations. It keeps necessary information about a process.
// Includes PID, parent PID, status, etc. 
#include "processmanager.h"

//----------------------------------------------------------------------
// PCB::PCB
// 	Initialize a PCB with initialize a bitmap with "MAX_FILE_OPEN"
//	bits, and set the bits in a bitmap
//----------------------------------------------------------------------

PCB::PCB():bitMap(MAX_FILE_OPEN){
bitMap.Mark(0);
bitMap.Mark(1);
}

//----------------------------------------------------------------------
// PCB::~PCB
//	De-allocate a PCB
//----------------------------------------------------------------------

PCB::~PCB(){

}

//----------------------------------------------------------------------
// PCB::Add
//	Add a UserOpenFile in PCB
//----------------------------------------------------------------------

int PCB::Add(UserOpenFile uoFile){
	int i = bitMap.Find();
	if(i < 0)
		return i;
	this->userOpenFileTable[i] = uoFile;
	return i;
}

//----------------------------------------------------------------------
// PCB::Remove
//	Remove a UserOpenFile by file id
//----------------------------------------------------------------------

bool PCB::Remove(int fileId){
	bitMap.Clear(fileId);
	return true;
}

//----------------------------------------------------------------------
// PCB::Get
//	Return the UserOpenFile by file id
//----------------------------------------------------------------------

UserOpenFile* PCB::Get(int fileId){
if(bitMap.Test(fileId))
	return userOpenFileTable+fileId;
return NULL;
}

//----------------------------------------------------------------------
// ProcessManager::ProcessManager
// Initialize a process manager with initialize a bitmap with "MAX_PROCESS"
//	bits,and set the PCBs array, conditions array and lock array
//----------------------------------------------------------------------

ProcessManager::ProcessManager():bitMap(MAX_PROCESS){
	pcbs = new PCB*[MAX_PROCESS];
	conditions = new Condition*[MAX_PROCESS];
	locks = new Lock*[MAX_PROCESS];
}

//----------------------------------------------------------------------
// ProcessManager::~ProcessManager
//	De-allocate a process manager
//----------------------------------------------------------------------

ProcessManager::~ProcessManager(){
	delete pcbs;
	delete conditions;
	delete locks;
}

//----------------------------------------------------------------------
// ProcessManager::GetPID
// 	Return an unused process id
//----------------------------------------------------------------------

int ProcessManager::GetPID(){
	int result = bitMap.Find();
	this->joinProNum[result] = 0;
	this->joinProNum[result] ++;
	return result;
}

//----------------------------------------------------------------------
// ProcessManager::GetPID
// 	Clear a process id respectively
//
//	"pid" is the pid number of the process need to be cleared
//----------------------------------------------------------------------

void ProcessManager::ClearPID(int pid){
	this->joinProNum[pid]--;
	if(this->joinProNum[pid]==0)
	bitMap.Clear(pid);
}

//----------------------------------------------------------------------
// ProcessManager::AddProcessAt
// 	Add a process with exact pid
//
//	"*pcb" is the pointer to the PCB to be added
//	"pid" is the pid number of the process
//----------------------------------------------------------------------

void ProcessManager::AddProcessAt(PCB *pcb,int pid){
	pcbs[pid] = pcb;
}

//----------------------------------------------------------------------
// ProcessManager::Join
//	The PCB manager keep track of who is waiting for who using a condition
//	variable for each PCB.
//
//	"pid" is the pid number of the process
//----------------------------------------------------------------------

void ProcessManager::Join(int pid){
	//ASSERT(bitMap.Test(pid));
	Lock * lock = locks[pid];
	if(lock == NULL){
		lock = new Lock("");
		locks[pid] = lock;
	}
	Condition* condition = conditions[pid];
	if(condition == NULL){
		condition = new Condition("");
		conditions[pid] = condition;
	}
	lock->Acquire();
	this->joinProNum[pid]++;
	condition->Wait(lock);
	this->joinProNum[pid]--;
	if(this->joinProNum[pid]==0)
		bitMap.Clear(pid);
	lock->Release();
}

//----------------------------------------------------------------------
// ProcessManager::Broadcast
//	Broadcast the "pid" process in Exit
//
//	"pid" is the pid number of the process
//----------------------------------------------------------------------

void ProcessManager::Broadcast(int pid){
	Lock * lock = locks[pid];
	Condition* condition = conditions[pid];
	pcbStatus[pid] = pcbs[pid]->status;
	if(condition != NULL){
		lock->Acquire();
		condition->Broadcast(lock);
		lock->Release();
	}

}

//----------------------------------------------------------------------
// ProcessManager::GetStatus
// 	Return process's status by pid
//
//	"pid" is the pid number of the process to get
//----------------------------------------------------------------------

int ProcessManager::GetStatus(int pid){
	if(bitMap.Test(pid) == 0)
		return -1;//process already done
	return pcbStatus[pid];
}
