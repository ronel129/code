// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"
#include "synchlist.h"


// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

int SharedVariable;
int threadsRequired;
int noThreads;
Semaphore *mutex = new Semaphore("mutex", 1);
Semaphore *barrier = new Semaphore("barrier", 0);
Lock *mutexLock = new Lock("mutexLock");
Lock *barrierLock = new Lock("barrierLock");
void
SimpleThread(int which)
{
    	int num, val;	
    	for(num = 0; num < 5; num++) 
    	{	
		
		#if defined(CHANGED) && defined(HW1_SEMAPHORES)
			mutex->P();	
		#endif
		
		#if defined(CHANGED) && defined(HW1_LOCKS)
			mutexLock->Acquire();
		#endif
		
		val = SharedVariable;
	      printf("*** thread %d sees value %d\n", which, val); 
		
		currentThread->Yield(); 		
		SharedVariable = val+1;
		
		#if defined(CHANGED) && defined(HW1_SEMAPHORES)
			mutex->V();	
		#endif	

		#if defined(CHANGED) && defined(HW1_LOCKS)
			mutexLock->Release();
		#endif
	
	       currentThread->Yield();
			
    	}
	#if defined(CHANGED) && defined(HW1_SEMAPHORES)	
		mutex->P();
		noThreads ++;
		if ( noThreads == threadsRequired )
		{
			barrier->V();		
		}
		mutex->V();

		barrier->P();
		barrier->V();	
	#endif

	#if defined(CHANGED) && defined(HW1_LOCKS)
		mutexLock->Acquire(); 
		noThreads ++;
		mutexLock->Release(); 
		while ( noThreads != threadsRequired )
		{
			currentThread->Yield();		
		}

	#endif
	
    	val = SharedVariable;		
    	printf("Thread %d sees final value %d\n", which, val); 
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------
void
ThreadTest(int n)
{
    #if defined(CHANGED) && !defined(HW1_ELEVATOR)
    int i;
    threadsRequired = n;	
   	
    for ( i = 0; i < n; i ++ )
    {
		
   		Thread *t = new Thread("forked thread");  
		t->Fork(SimpleThread, i);
		
    }  
	
    #else
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    default:
	printf("No test specified.\n");
	break;
    }
    #endif
}