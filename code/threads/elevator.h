#ifndef ELEVATOR_H
#define ELEVATOR_H

#include "copyright.h"
#include "system.h"
#include "synch.h"
#include "synchlist.h"

//----------------------------------------------------------------------
// ElevatorClass
//----------------------------------------------------------------------

class ElevatorClass {
public:
	ElevatorClass(int numFloor);
	virtual ~ElevatorClass();
	void RequestAndWait(int atFloor);
	void Enter(int toFloor);
	void Exit();	
	void Run();	
	int GetOccupancy();
	void IncrementOccupancy();
	void DecrementOccupancy();
	
private:
	int elevatorOccupancy;
	void AddToQueue(int floor);	
	int currentFloor;	
	SynchList* path; 
	SynchList* nextPath;
	int* personCount;
	Lock* lock;	
	Condition* elevatorCondition;
	Condition* personCondition;
};

#endif // ELEVATOR_H
