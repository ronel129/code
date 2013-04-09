#include "utilities.h"

//----------------------------------------------------------------------
//Compares two buffer pointers, returns true when they are same.
//----------------------------------------------------------------------

bool strcmp(char* first,char * second){

	while((*first)!=0)
	{
		if(*first != *second)
			return false;
		first++;
		second++;
	}
	return *second == 0;
}
