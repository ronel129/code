// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"

#define UserStackSize		1024 	// increase this as necessary!

class PCB;
class AddrSpace {
  public:
    	AddrSpace(OpenFile *executable);	// Create an address space,
	AddrSpace();
					// initializing it with the program
					// stored in the file "executable"
    	~AddrSpace();			// De-allocate an address space

    	void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    	void SaveState();			// Save/restore address space-specific
    	void RestoreState();		// info on a context switch 
	bool Convert(int virtAddr, int* physAddr); //converts a virtual address to a physical address
	
	int ReadFile(int virtAddr, OpenFile* file, int size,
	int fileAddr);	// Load the code and data segments into the Convertd memory
	bool Valid();	
	AddrSpace* Copy();	// Create a new address space, an exact copy of the original
	int GetPages();	

public :
	PCB *pcb;	// store information of the process control block (PCB)


  private:
    TranslationEntry *pageTable;	
    unsigned int numPages;		
};

#endif // ADDRSPACE_H
