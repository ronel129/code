// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------


static void SwapHeader(NoffHeader *noffH) {

	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable) {

	Semaphore* addrLock = new Semaphore("addrLock",1);	
	NoffHeader noffH;
	unsigned int i, size;

	executable->ReadAt((char *) &noffH, sizeof(noffH), 0);
	if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic)
			== NOFFMAGIC))
		SwapHeader(&noffH);
	ASSERT(noffH.noffMagic == NOFFMAGIC);
	// how big is address space?
	size = noffH.code.size + noffH.initData.size + noffH.uninitData.size
			+ UserStackSize; // we need to increase the size
	// to leave room for the stack
	numPages = divRoundUp(size, PageSize);
	size = numPages * PageSize;

	addrLock->P();
	if(numPages > memManager->GetClearPages()) {// check we're not trying
		printf("Not enough memory.\n");
		numPages = -1;
		addrLock->V();
		return;
	}
	// to run anything too big --
	// at least until we have
	// virtual memory

	DEBUG('a', "Initializing address space, num pages %d, size %d\n", numPages,
			size);
	// first, set up the translation
	pageTable = new TranslationEntry[numPages];
	for (i = 0; i < numPages; i++) {
		pageTable[i].virtualPage = i;
		//use Memory maanager class.
		//set pages allocated by memory manager for the physical page index
		pageTable[i].physicalPage = memManager->GetPage();
		// zero out the address space of physical page
		// and the stack segment
		bzero(machine->mainMemory + pageTable[i].physicalPage * PageSize,
				PageSize);
		pageTable[i].valid = TRUE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE; // if the code segment was entirely on
		// a separate page, we could set its
		// pages to be read-only
	}
	addrLock->V();
	// then, copy in the code and data segments into memory
	if (noffH.code.size > 0) {
		DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
				noffH.code.virtualAddr, noffH.code.size);
		ReadFile(noffH.code.virtualAddr, executable, noffH.code.size,
				noffH.code.inFileAddr);
	}
	if (noffH.initData.size > 0) {
		DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
				noffH.initData.virtualAddr, noffH.initData.size);
		ReadFile(noffH.initData.virtualAddr, executable, noffH.initData.size,
				noffH.initData.inFileAddr);
	
	}
	printf("Loaded Program: %d code | %d data | %d bss\n",noffH.code.size,noffH.initData.size,noffH.uninitData.size);
}

AddrSpace::AddrSpace() {
}//Do nothing

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace() {
	if(this->Valid()){
	for (int i = 0; i < numPages; i++) {
		memManager->ClearPage(pageTable[i].physicalPage);
	}

	delete pageTable;
	delete pcb;}
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------
void AddrSpace::InitRegisters() {

	int i;

	for (i = 0; i < NumTotalRegs; i++)
		machine->WriteRegister(i, 0);

	// Initial program counter -- must be location of "Start"
	machine->WriteRegister(PCReg, 0);

	// Need to also tell MIPS where next instruction is, because
	// of branch delay possibility
	machine->WriteRegister(NextPCReg, 4);

	// Set the stack register to the end of the address space, where we
	// allocated the stack; but subtract off a bit, to make sure we don't
	// accidentally reference off the end!
	machine->WriteRegister(StackReg, numPages * PageSize - 16);
	DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() {

}


//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() {
	machine->pageTable = pageTable;
	machine->pageTableSize = numPages;
}

//----------------------------------------------------------------------
// AddrSpace::Convert
// 	Convert a virtual address to a physical address. 
//	Return a boolean TRUE or FALSE depending on whether or not the virtual
//	address was valid.
//----------------------------------------------------------------------

bool AddrSpace::Convert(int virtualAddr, int* physAddr) {
	int i;
	
	unsigned int pageFrame;
	unsigned int vpn, offset;
	TranslationEntry entry;
	ASSERT(pageTable != NULL);

	// calculate the virtual page number, and offset within the page
	vpn = (unsigned) virtualAddr / PageSize;
	offset = (unsigned) virtualAddr % PageSize;
	if (vpn >= numPages) {
		//Page is too large, return.
		return false;
	} else if (!pageTable[vpn].valid) {
		//Not a valid page
		return false;
	}
	entry = pageTable[vpn];
	pageFrame = entry.physicalPage;

	if (pageFrame >= NumPhysPages) {
		// if the pageFrame is larger than physical pages need to exit.
		return false;
	}

	//store the physical address
	*physAddr = pageFrame * PageSize + offset;
	ASSERT((*physAddr >= 0));
	return true;
}

//----------------------------------------------------------------------
// AddrSpace::ReadFile
// 	Load the code and data segments into the Converted memory.
//----------------------------------------------------------------------

int AddrSpace::ReadFile(int virtualAddr, OpenFile *file, int size, int fileAddr) {
	
	int bytesCopied=0;
	int sizeAvailable, readSize;
	char diskBuffer[size];	
	int physAddr;	

	int returnSize = file->ReadAt(diskBuffer, size, fileAddr);
	size = returnSize;

	//Reads the bytes from the file.	

	while (size > 0 ) {
		bool valid = Convert(virtualAddr, &physAddr);
		ASSERT(valid==true);
		sizeAvailable = PageSize - (physAddr) % PageSize;
		readSize = min(size,sizeAvailable);
		bcopy(diskBuffer+bytesCopied, &machine->mainMemory[physAddr], readSize);
		
		size -= readSize;
		bytesCopied+= readSize;
		virtualAddr += readSize;
	}
	return returnSize;
}

//----------------------------------------------------------------------
// AddrSpace::Copy
//	Create a new address space that is copy of the original.
//----------------------------------------------------------------------

AddrSpace* AddrSpace::Copy() {

	if(numPages > memManager->GetClearPages()){
		return NULL;
	}
	AddrSpace* Copy = new AddrSpace();
	Copy->numPages = this->numPages;
	Copy->pageTable = new TranslationEntry[numPages];
	for (int i = 0; i < numPages; i++) {
		Copy->pageTable[i].virtualPage = i;
		Copy->pageTable[i].physicalPage = memManager->GetPage();
		Copy->pageTable[i].valid = this->pageTable[i].valid;
		Copy->pageTable[i].use = this->pageTable[i].use;
		Copy->pageTable[i].dirty = this->pageTable[i].dirty;

		Copy->pageTable[i].readOnly = this->pageTable[i].readOnly;
		bcopy(machine->mainMemory + this->pageTable[i].physicalPage * PageSize,
				machine->mainMemory + Copy->pageTable[i].physicalPage
						* PageSize, PageSize);
	}
	return Copy;
}

//----------------------------------------------------------------------
// AddrSpace::Valid
// 	Return true if address space has pages
//----------------------------------------------------------------------

bool AddrSpace::Valid(){
	return numPages!=-1;
}

//----------------------------------------------------------------------
// AddrSpace::GetPages
// 	Return numPages
//----------------------------------------------------------------------

int AddrSpace::GetPages(){
	return numPages;
}
