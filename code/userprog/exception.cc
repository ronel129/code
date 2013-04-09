// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//TODO
const int FILE_NAME_SIZE = 128;

//----------------------------------------------------------------------
//Need comments
//----------------------------------------------------------------------

char* StringClone(char* old) {
	char * newString = new char[FILE_NAME_SIZE];
	for (int i = 0; i < FILE_NAME_SIZE; i++) {
		newString[i] = old[i];
		if (old[i] == NULL)
			break;
	}
}

//----------------------------------------------------------------------
// AdjustPCRegs
//	Changes register values after each system call. Moves the program counter to 
// the next ( pc + 4)
//----------------------------------------------------------------------

void AdjustPCRegs() {
	int pc = machine->ReadRegister(PCReg);
	machine->WriteRegister(PrevPCReg, pc);
	pc = machine->ReadRegister(NextPCReg);
	machine->WriteRegister(PCReg, pc);
	machine->WriteRegister(NextPCReg, pc + 4);
}

//----------------------------------------------------------------------
//	Read file from memory to a string. Used for Exec Sytem call
//----------------------------------------------------------------------

void ReadFile(char *fileName) {
	int pos = 0;
	int value;
	int arg1 = machine->ReadRegister(4);
	while (value != NULL) {
		machine->ReadMem(arg1, 1, &value);
		fileName[pos] = (char) value;
		pos++;
		arg1++;
	}
}

//----------------------------------------------------------------------
// HaltHandler
//	Halt current thread.
//----------------------------------------------------------------------

void HaltHandler() {
	printf("System Call: %d invoked Halt\n", currentThread->space->pcb->pid);
	interrupt->Halt();
}

//----------------------------------------------------------------------
// ForkHelper
// 	Used by ForkHandler to copy back the machine registers,
//	PC and return registers saved from the yield.
//----------------------------------------------------------------------

void ForkHelper(int funcAddr) {
	int* state = (int*)funcAddr;
	for(int i =0; i < NumTotalRegs;i++)
		 machine->WriteRegister(i,state[i]);
		delete[] state;
	currentThread->space->RestoreState(); 
	machine->Run(); 
	ASSERT(FALSE); 
}

//----------------------------------------------------------------------
// ForkHandler
//The Fork(func) system call creates a new user-level (child) process, 
//whose address space starts out as an exact copy of that of the caller (the parent), 
//but immediately the child abandons the program of the parent and starts executing the 
//function supplied by the single argument. 
//Fork should return pid of child process (SpaceId). 
//Notice this definition is slightly different from the one in the syscall.h file in Nachos. 
//Also, the semantics is not the same as Unix fork(). 
//After forked function func finishes, the control should go back to the 
//instruction after the initial system call Fork.
//----------------------------------------------------------------------

SpaceId ForkHandler(int virtualSpace) {
	int currentPid = currentThread->space->pcb->pid;
	printf("System Call: %d invoked Fork\n", currentPid);
	AddrSpace *space = currentThread->space->Copy();
	int spaceId;
	if(space==NULL){
		printf("Process %d was not able to fork a new process\n",currentPid);
		return -1;//IS this correct?
	}

	PCB *pcb = new PCB();
	Thread* thread = new Thread("Fork");
	//set PCB
	pcb->thread = thread;
	pcb->pid = pm->GetPID();
	pcb->status = PRUNNING;
	ASSERT(pcb->pid!=-1);
	pcb->parentPid = currentThread->space->pcb->pid; //parent ID
	space->pcb = pcb;
	thread->space = space;
	pm->AddProcessAt(pcb, pcb->pid);
	space->SaveState();
	int* currentState = new int[NumTotalRegs];
	for(int i =0; i < NumTotalRegs;i++)
		currentState[i] = machine->ReadRegister(i);
	currentState[PCReg] = virtualSpace;
	currentState[NextPCReg] = virtualSpace+4;
	spaceId = pcb->pid;
	printf("Process %d Fork %d: start at address 0x%X with %d pages memory\n",currentPid,spaceId,virtualSpace,space->GetPages());
	thread->Fork(ForkHelper, (int)currentState);
	currentThread->Yield();
	return spaceId;
}

//----------------------------------------------------------------------
// ExitHandler
//The Exit(int) call takes a single argument, which is an integer status value as in Unix. 
//The currently executing process is terminated. For now, you can just ignore the status value. 
//You need to supply this value to parent process if and when it does a Join().
//----------------------------------------------------------------------

void ExitHandler(int status) {
	printf("System Call: %d invoked Exit\n", currentThread->space->pcb->pid);
	printf("Process %d exits with %d\n", currentThread->space->pcb->pid,status);
	int pid = currentThread->space->pcb->pid;
	currentThread->space->pcb->status = status;
	pm->Broadcast(pid);
	delete currentThread->space;
	currentThread->space = NULL;
	pm->ClearPID(pid);
	currentThread->Finish();
}

//----------------------------------------------------------------------
// YieldHandler
//The Yield() call is used by a process executing in user mode to 
//temporarily relinquish the CPU to another process. 
//The return value is undefined.
//----------------------------------------------------------------------

void YieldHandler() {
	printf("System Call: %d invoked Yield\n", currentThread->space->pcb->pid);
	currentThread->Yield();
}


//----------------------------------------------------------------------
// KillHandler
//The Kill(SpaceId) kills a process with supplied SpaceId. It returns 0 if successful 
//and -1 if not (for example SpaceId is not valid).
//----------------------------------------------------------------------
int KillHandler(int kpid) {
	int pid = currentThread->space->pcb->pid;
	int status = currentThread->space->pcb->status;
	int success = -1;
	
	printf("System Call: %d invoked Kill\n", pid, kpid);
	
	if ( pid == kpid )
	{
		Exit(status);
		success = 0;
	}
	else
	{

		Thread* threadToBeKilled = scheduler->RemoveThreadByPid(kpid);
		if ( threadToBeKilled != NULL  ) 
		{
			//threadToBeKilled->space->pcb->status = status;
			pm->Broadcast(kpid);
			delete threadToBeKilled->space;
			threadToBeKilled->space = NULL;
			pm->ClearPID(kpid);
			//threadToBeKilled->Finish();
			success = 0;
		} 		
	}
	//return id;
	if (success == 0) 
	{
			printf("Process %d killed process %d\n", pid,kpid);
	
	} else {
	
			printf("Process %d cannot kill process %d\n", pid, kpid);
	
	}
	
	return success;
}

//----------------------------------------------------------------------
// ExecHandler
//The Exec(filename) system replaces the current process state with a 
//new process executing program from file.
//You can think as if the current process stops executing and the new program is 
//loaded in its place. The new program uses the object code from the Nachos 
//object file which name is supplied as an argument to the call.
//It should return -1 to the parent if not successful. 
//If successful, the parent process is replaced with the new program running from its beginning. 
//The way to execute a new process without destroying the current one is to first Fork
//and then Exec.
//----------------------------------------------------------------------

int ExecHandler(char * filename) 
{
	Semaphore* mutex = new Semaphore("execLock",1);
	mutex->P();
	printf("System Call: %d invoked Exec\n", currentThread->space->pcb->pid);
	printf("Exec Program: %d loading %s\n", currentThread->space->pcb->pid,filename);
	int pid = currentThread->space->pcb->pid;
	int status = currentThread->space->pcb->status;
			
	OpenFile *executable = fileSystem->Open(filename);
	if (executable == NULL) {
		printf("Unable to open file %s\n", filename);
		mutex->V();

		return -1;
	}

	PCB* pcb;
	pcb = currentThread->space->pcb;
	

	AddrSpace *space;
	space = new AddrSpace(executable);

	//Return when the address space is not valid
	if(!space->Valid()){
		delete space;
		mutex->V();
		return -1;
	}
		
	space->pcb = pcb;
	Thread *t = new Thread("exec");
	t->space = space;
	pcb->thread = t;
 	currentThread->space = space;

	delete executable;

	currentThread->space->InitRegisters();
	currentThread->space->RestoreState(); // load page table register
	
	//Release
	mutex->V();

	return 1;
}

//----------------------------------------------------------------------
// JoinHandler
//The Join(SpaceId) call waits and returns only after a process with the 
//specified SpaceID (supplied as an argument to the call) has finished. 
//The return value of the Join call is the exit status of the process for 
//which it was waiting or -1 in the case of error (for example, invalid SpaceId).
//----------------------------------------------------------------------

int JoinHandler(int id) {
	printf("System Call: %d invoked Join\n", currentThread->space->pcb->pid);
	currentThread->space->pcb->status = PBLOCKED;
	if(pm->GetStatus(id)<0)
		return pm->GetStatus(id);
	pm->Join(id);
	currentThread->space->pcb->status = PRUNNING;
	return pm->GetStatus(id);
}


//Project 3 System calls below.
//----------------------------------------------------------------------
// Gets the fileName from user space then 
// use fileSystem->Create(fileName,0)to create a new instance of an OpenFile object.
// Until a user opens the file for IO it is not necessary to do anything further.
//----------------------------------------------------------------------

void CreateHandler(char *fileName) {
	printf("System Call: %d invoked Create\n", currentThread->space->pcb->pid);
	bool success = fileSystem->Create(fileName, 0);
	ASSERT(success);
}

//----------------------------------------------------------------------
//Function will use an OpenFile object created previously by fileSystem->Open (fileName). 
//Once you have this object, check to see if it is already open by some other process 
//in the global SysOpenFile table. 
//If so, incriment the userOpens count. 
//If not, create a new SysOpenFile and store it's pointer
//----------------------------------------------------------------------
OpenFileId OpenHandler(char *fileName) {
	printf("System Call: %d invoked Open\n", currentThread->space->pcb->pid);
	int index  = 0;
	SysOpenFile * sysFile = fileManager->Get(fileName, index);
	
	if (sysFile == NULL) {
		OpenFile * openFile = fileSystem->Open(fileName);
		if (openFile == NULL) {
			printf("Unable to open file %s\n", fileName);
			ASSERT(FALSE);
			return -1;
		}
		SysOpenFile sysFile;
		sysFile.file = openFile;
		sysFile.userOpens = 1;
		sysFile.fileName = StringClone(fileName);
		index = fileManager->Add(sysFile);
	}
	else{
		sysFile->userOpens++;
	}
	UserOpenFile userFile;
	userFile.fileIndex = index;
	userFile.offset = 0;
	OpenFileId openFileId = currentThread->space->pcb->Add(userFile);
	return openFileId;
}

//----------------------------------------------------------------------
// Gets the userFile using the Id. 
// Uses the userFile fileindex to create get a system file and closes it.
//----------------------------------------------------------------------
void CloseHandler(OpenFileId id) {
	printf("System Call: %d invoked Close\n", currentThread->space->pcb->pid);
	UserOpenFile* userFile =  currentThread->space->pcb->Get(id);
	if(userFile == NULL){
		return ;
	}
	SysOpenFile * sysFile=fileManager->Get(userFile->fileIndex);
	sysFile->closeFile();
	currentThread->space->pcb->Remove(id);
}

///----------------------------------------------------------------------
//Get the arguments from the user in the same way you did for Write(). 
//If the OpenFileID == ConsoleInput (syscall.h), use a routine to read into a 
//buffer of size 'size+1' one character at a time using getChar(). 
//Otherwise, grab a handle to the OpenFile object in the same way you did for Write() 
//and use OpenFileObject->ReadAt(myOwnBuffer,size,pos) to put n characters into your buffer. 
//pos is the position listed in the UserOpenFile object that 
//represents the place in the current file you are writing to. With this method, 
//you must explictly put a null byte after the last character read. The number read is returned from ReadAt(). 
//----------------------------------------------------------------------
int ReadHandler(int bufferAddr, int size, OpenFileId id) {
	printf("System Call: %d invoked Read\n", currentThread->space->pcb->pid);
	char *buffer = new char[size + 1];
	int actualSize = size;
	if (id == ConsoleInput) {
		int count = 0;
		while (count < size) {
			buffer[count]=getchar();
			count++;
		}
	} else {
		UserOpenFile* userFile =  currentThread->space->pcb->Get(id);
		if(userFile == NULL)
			return 0;
		SysOpenFile * sysFile=fileManager->Get(userFile->fileIndex);
		actualSize = sysFile->file->ReadAt(buffer,size,userFile->offset);
		userFile->offset+=actualSize;
	}
	
	userReadWrite(bufferAddr,buffer,actualSize,USER_READ);
	delete[] buffer;
	return actualSize;
}

//----------------------------------------------------------------------
//If the OpenFileID is == ConsoleOutput (syscall.h), then simply printf(buffer). 
//Otherwise, grab a handle to the OpenFile object from the user's openfile list pointing 
//to the global file list.
//Once the OpenFile object is taken, fill up a buffer 
//of size 'size+1' using your userReadWrite() function. 
//Then call OpenFileObject->Write(myOwnBuffer, size);
//----------------------------------------------------------------------
void WriteHandler(int bufferAddr, int size, OpenFileId id) {
	
	char* buffer = new char[size+1];

	if (id == ConsoleOutput) {
		userReadWrite(bufferAddr,buffer,size,USER_WRITE);
		buffer[size]=0;
		printf("%s\n",buffer);
	} else {
		printf("System Call: %d invoked Write\n", currentThread->space->pcb->pid);
	
		buffer = new char[size];
		int writeSize = userReadWrite(bufferAddr,buffer,size,USER_WRITE);
		UserOpenFile* userFile =  currentThread->space->pcb->Get(id);
		if(userFile == NULL)
			return ;
		SysOpenFile * sysFile=fileManager->Get(userFile->fileIndex);
		int bytes = sysFile->file->WriteAt(buffer,size,userFile->offset);
		userFile->offset+=bytes;
	}		
	delete[] buffer;
}

//Project4-
void PageFaultHandler() {
	int faultAddr = machine->ReadRegister(BadVAddrReg);
	vmmanager->ReplacePage(faultAddr);
	printf("L %d: %d -> %d\n",currentThread->space->pcb->pid,faultAddr/PageSize,currentThread->space->GetTranslationEntry(faultAddr/PageSize)->physicalPage);
}

void ReadOnlyHandler(){
	int faultAddr = machine->ReadRegister(BadVAddrReg);
	vmmanager->ChangeRepresentee(currentThread->space,faultAddr);
	TranslationEntry* entry = currentThread->space->GetTranslationEntry(faultAddr/PageSize);
	printf("D %d: %d\n%",currentThread->space->pcb->pid,entry->diskLoc/PageSize);
	entry->diskLoc = vmmanager->GetStoreLocation();
	int oriPhyPage = entry->physicalPage;
	vmmanager->BackStore(machine->mainMemory+entry->physicalPage*PageSize,PageSize,entry->diskLoc);
	vmmanager->GetDiskPageInfo(entry->diskLoc/PageSize)->Add(entry);
	entry->dirty = FALSE;
	entry->readOnly = FALSE;
	entry->valid = FALSE;
	vmmanager->ReplacePage(faultAddr);
}

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
	int arg1, arg2, arg3, result;
	char* fileName = new char[FILE_NAME_SIZE];// file name
	int pos = 0;
	if (which == SyscallException) {
		switch (type) {
		case SC_Halt:
			DEBUG('a', "Shutdown, initiated by user program.\n");
			HaltHandler();
			break;
		case SC_Fork: //Fork Handler
			arg1 = machine->ReadRegister(4);
			result = ForkHandler(arg1);
			machine->WriteRegister(2, result);
			break;
		case SC_Exit: //Exit Handler
			arg1 = machine->ReadRegister(4);
			ExitHandler(arg1);
			break;
		case SC_Exec: //Exec handler
			ReadFile(fileName);
			result = ExecHandler(fileName);
			machine->WriteRegister(2, result);
			break;
		case SC_Yield: //Yield Handler
			YieldHandler();
			break;
		case SC_Join: //Join handler
			arg1 = machine->ReadRegister(4);
			result = JoinHandler(arg1);
			machine->WriteRegister(2, result);
			break;
		case SC_Kill: //Kill handler
			arg1 = machine->ReadRegister(4);
			result = KillHandler(arg1);
			machine->WriteRegister(2, result);
			break;
		//Project 3 system calls.
		case SC_Create:
			ReadFile(fileName);
			CreateHandler(fileName);
			break;
		case SC_Open:
			ReadFile(fileName);
			StringClone(fileName);
			result = OpenHandler(fileName);
			machine->WriteRegister(2, result);
			break;
		case SC_Close:
			arg1 = machine->ReadRegister(4);
			CloseHandler(arg1);
			break;
		case SC_Read:
			arg1 = machine->ReadRegister(4);
			arg2 = machine->ReadRegister(5);
			arg3 = machine->ReadRegister(6);
			result = ReadHandler(arg1, arg2, arg3);
			machine->WriteRegister(2, result);
			break;
		case SC_Write:
			arg1 = machine->ReadRegister(4);
			arg2 = machine->ReadRegister(5);
			arg3 = machine->ReadRegister(6);
			WriteHandler(arg1, arg2, arg3);
			//machine->WriteRegister(2, result);
			break;
		default:
			printf("System call spefied not supported.\n");
		}
		AdjustPCRegs();
    } 
	else if (which == PageFaultException) {
		PageFaultHandler();
	} else if(which== ReadOnlyException){
		ReadOnlyHandler();
	else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
