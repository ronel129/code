//#if defined(THREADS) && defined(CHANGED) &&defined(HW1_LOCKS) &&defined(HW1_ELEVATOR)

#include "copyright.h"
#include "elevator.h"
#include "system.h"
#include "synch.h"
#include "synchlist.h"


ElevatorClass* elevator;	
int num=0;
int out = 0;

//----------------------------------------------------------------------
// StartElevator
//----------------------------------------------------------------------

void StartElevator(int){
elevator->Run();
}

//----------------------------------------------------------------------
// Elevator
//----------------------------------------------------------------------

void Elevator(int numFloors) {
	if ( numFloors < 1 ){
		printf("Number of floor specified %d, is not supported.\n", numFloors);
		elevator = NULL;		
	}
	else{
		elevator = new ElevatorClass(numFloors);
		Thread* t  = new Thread("Elevator");
		t->Fork(StartElevator,0);
	}
}

//----------------------------------------------------------------------
// SimulatePerson
//----------------------------------------------------------------------

void SimulatePerson(int infoReq) {

		if (elevator->GetOccupancy() == 5) { return; } //elevator full.
		
		int* info = (int *)infoReq;
		printf("Person %d wants to go to floor %d from floor %d. \n", info[0], info[2], info[1]);		
		elevator->RequestAndWait(info[1]);		
		printf("Person %d got into the elevator. \n", info[0]);			
		elevator->Enter(info[2]);
		printf("Person %d got out of the elevator. \n", info[0]);
		elevator->Exit();
		delete info;
}

//----------------------------------------------------------------------
// ArrivingGoingFromTo
//----------------------------------------------------------------------

void ArrivingGoingFromTo(int atFloor, int toFloor) {
	if (elevator == NULL) { return; }
	num++;
	int* infoReq = new int[3];
	infoReq[0] = num;
	infoReq[1] = atFloor;
	infoReq[2] = toFloor;
	Thread* t = new Thread("Person");
	t->Fork(SimulatePerson, (int)infoReq);
}

//----------------------------------------------------------------------
// ElevatorClass::ElevatorClass
//----------------------------------------------------------------------

ElevatorClass::ElevatorClass(int numFloor) {
	currentFloor = 1;
	elevatorOccupancy = 0;
	personCount = new int[numFloor + 1];	
	for (int i = 1; i <= numFloor; i++) {
		personCount[i] = 0;
	}
	lock = new Lock("");
	path = new SynchList();
	nextPath = new SynchList();
	elevatorCondition = new Condition("1");
	personCondition = new Condition("2");
}

//----------------------------------------------------------------------
// ElevatorClass::~ElevatorClass()
//----------------------------------------------------------------------

ElevatorClass::~ElevatorClass() {
	delete personCount;
	delete lock;
	delete elevatorCondition;
	delete personCondition;
	delete path;
	delete nextPath;
}

int ElevatorClass::GetOccupancy(){
		return elevatorOccupancy;
	}
	
void ElevatorClass::IncrementOccupancy() {
		elevatorOccupancy++;
	}

void ElevatorClass::DecrementOccupancy() {
		elevatorOccupancy--;
	}

//----------------------------------------------------------------------
// ElevatorClass::RequestAndWait
//----------------------------------------------------------------------

void ElevatorClass::RequestAndWait(int atFloor) {
	lock->Acquire();
	if (currentFloor == atFloor && personCount[0] == 0) {
		personCount[atFloor]++;
		lock->Release();
		return;
	}
	AddToQueue(atFloor);
	while (currentFloor != atFloor || personCount[0] == 1) {
		personCondition->Wait(lock);
	}
	lock->Release();
}

//----------------------------------------------------------------------
// ElevatorClass::Enter
//----------------------------------------------------------------------

void ElevatorClass::Enter(int toFloor) {
	
	lock->Acquire();
	elevator->IncrementOccupancy();
	AddToQueue(toFloor);
	personCount[currentFloor]--;
	if (personCount[currentFloor] == 0){
		elevatorCondition->Signal(lock);
	}
	while (currentFloor != toFloor || personCount[0] == 1)
		personCondition->Wait(lock);						    
	lock->Release();
}

//----------------------------------------------------------------------
// ElevatorClass::Exit
//----------------------------------------------------------------------

void ElevatorClass::Exit() {
	lock->Acquire();
	elevator->DecrementOccupancy();
	personCount[currentFloor]--;
	if (personCount[currentFloor] == 0)
		elevatorCondition->Signal(lock);

	lock->Release();
}

//----------------------------------------------------------------------
// ElevatorClass::AddToQueue
//----------------------------------------------------------------------

void ElevatorClass::AddToQueue(int floor) {
	if(!(lock->isHeldByCurrentThread()))
		lock->Acquire();
	int* floorPtr = new int(floor);
	if (path->IsEmpty()) {
		path->SortedInsert(floorPtr, floor);
		elevatorCondition->Signal(lock);
	} else {
		int * nextFloor = (int *) path->Top();
		if(personCount[floor] == 0)								
		if (*nextFloor < currentFloor) {
			if (floor >= currentFloor) {
				nextPath->SortedInsert(floorPtr, floor);
			} else {
				path->SortedInsert(floorPtr, -floor);
			}
		} else {	
			if (floor <= currentFloor) {
				nextPath->SortedInsert(floorPtr, -floor);
			} else {
				path->SortedInsert(floorPtr, floor);
			}
		}
	}
	personCount[floor]++;
	if(!(lock->isHeldByCurrentThread()))
		lock->Release();
}

//----------------------------------------------------------------------
// ElevatorClass::Run
//----------------------------------------------------------------------

void ElevatorClass::Run() {
	while (true) {
		lock->Acquire();
		if (path->IsEmpty()) {
			elevatorCondition->Wait(lock);
			personCount[0] = 1;
		}
		lock->Release();
   		while (!path->IsEmpty()) {
			//Wait for elevator to move from floor to floor
			for (int i = 0; i < 50; i++) {
				currentThread->Yield();
			}
			lock->Acquire();
			int nextFloor = *((int *) path->Top());
			if (currentFloor > nextFloor) {
				currentFloor--;
			} else if (currentFloor < nextFloor) {
				currentFloor++;
			}
			printf("Elevator arrives on floor %d.\n",currentFloor);
			if(currentFloor == nextFloor)
			{	personCount[0] = 0;
				personCondition->Broadcast(lock);
				elevatorCondition->Wait(lock);
				int* temp_ptr = (int*)path->Remove();
				delete temp_ptr;
				personCount[0] = 1;
			}
			lock->Release();

		}
		lock->Acquire();
		SynchList * temp = path;
		path = nextPath;
		nextPath = temp;
		lock->Release();
	}
}

//#endif