// synchlist.cc
//	Routines for synchronized access to a list.
//
//	Implemented by surrounding the List abstraction
//	with synchronization routines.
//
// 	Implemented in "monitor"-style -- surround each procedure with a
// 	lock acquire and release pair, using condition signal and wait for
// 	synchronization.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synchlist.h"

//----------------------------------------------------------------------
// SynchList::SynchList
//	Allocate and initialize the data structures needed for a 
//	synchronized list, empty to start with.
//	Elements can now be added to the list.
//----------------------------------------------------------------------

SynchList::SynchList()
{
    list = new List();
    lock = new Lock("list lock"); 
    listEmpty = new Condition("list empty cond");
}

//----------------------------------------------------------------------
// SynchList::~SynchList
//	De-allocate the data structures created for synchronizing a list. 
//----------------------------------------------------------------------

SynchList::~SynchList()
{ 
    delete list; 
    delete lock;
    delete listEmpty;
}

//----------------------------------------------------------------------
// SynchList::Append
//      Append an "item" to the end of the list.  Wake up anyone
//	waiting for an element to be appended.
//
//	"item" is the thing to put on the list, it can be a pointer to 
//		anything.
//----------------------------------------------------------------------

void
SynchList::Append(void *item)
{
    lock->Acquire();		// enforce mutual exclusive access to the list 
    list->Append(item);
    listEmpty->Signal(lock);	// wake up a waiter, if any
    lock->Release();
}

//----------------------------------------------------------------------
// SynchList::Remove
//      Remove an "item" from the beginning of the list.  Wait if
//	the list is empty.
// Returns:
//	The removed item. 
//----------------------------------------------------------------------

void *
SynchList::Remove()
{
    void *item;

    lock->Acquire();			// enforce mutual exclusion
    while (list->IsEmpty())
	listEmpty->Wait(lock);		// wait until list isn't empty
    item = list->Remove();
    ASSERT(item != NULL);
    lock->Release();
    return item;
}

//----------------------------------------------------------------------
// SynchList::Mapcar
//      Apply function to every item on the list.  Obey mutual exclusion
//	constraints.
//
//	"func" is the procedure to be applied.
//----------------------------------------------------------------------

void
SynchList::Mapcar(VoidFunctionPtr func)
{ 
    lock->Acquire(); 
    list->Mapcar(func);
    lock->Release(); 
}

//#if defined(THREADS) && defined(CHANGED)
bool SynchList::IsEmpty(){
bool isEmpty;
lock->Acquire();
isEmpty = list->IsEmpty();
lock->Release();
return isEmpty;
}

//----------------------------------------------------------------------
// SynchList::SortedInsert
//      Insert an "item" into a synchronized list, so that the list elements are
//	sorted in increasing order by "sortKey".
//      
//	"item" is the thing to put on the list, it can be a pointer to 
//		anything.
//	"sortKey" is the priority of the item.
//----------------------------------------------------------------------
 void SynchList::SortedInsert(void *item, int sortKey){
	 lock->Acquire();	// enforce mutual exclusive access to the list 
	 list->SortedInsert(item,sortKey);
	 listEmpty->Signal(lock);
	 lock->Release();
 }
//----------------------------------------------------------------------
// SynchList::SortedRemove
//      Remove the first "item" from the front of a sorted synchronized list.
// 
// Returns:
//	Pointer to removed item, NULL if nothing on the list.
//	Sets *keyPtr to the priority value of the removed item
//	(this is needed by interrupt.cc, for instance).
//
//	"keyPtr" is a pointer to the location in which to store the 
//		priority of the removed item.
//----------------------------------------------------------------------
  void* SynchList::SortedRemove(int *keyPtr){
	  void *item;
	     lock->Acquire();			// enforce mutual exclusion
	     while (list->IsEmpty())
	 	listEmpty->Wait(lock);		// wait until list isn't empty
	     item = list->SortedRemove(keyPtr);
	     ASSERT(item != NULL);
	     lock->Release();
	     return item;
  }
//----------------------------------------------------------------------
// List::Top
//	 Show the first "item" from the front of a sorted synchronized list.
//
// Returns:
//	Pointer to the first item, NULL if nothing on the list.
//----------------------------------------------------------------------
    void* SynchList::Top(){
    	void *item;
    		     lock->Acquire();			// enforce mutual exclusion
    		     while (list->IsEmpty())
    		 	listEmpty->Wait(lock);		// wait until list isn't empty
    		     item = list->Top();
    		     ASSERT(item != NULL);
    		     lock->Release();
    		     return item;
    }
//#endif

