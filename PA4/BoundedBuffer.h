#ifndef BoundedBuffer_h
#define BoundedBuffer_h

#include <stdio.h>
#include <queue>
#include <string>
#include "Semaphore.h"

using namespace std;

class BoundedBuffer
{
private:
	queue<vector<char> > q;	/* the queue of items in the buffer. Note
	that each item a sequence of characters that is best represented by a vector<char> because: 
	1. An STL std::string cannot keep binary/non-printables
	2. The other alternative is keeping a char* for the sequence and an integer length (i.e., the items can be of variable length), which is more complicated.*/

	// add necessary synchronization variables (e.g., sempahores, mutexes) and variables
	Semaphore emptySlots;
	Semaphore fullSlots;
	Semaphore mut;

public:
	BoundedBuffer(int _cap):emptySlots(_cap), fullSlots(0), mut(1){

	}

	~BoundedBuffer(){

	}

	void push(vector<char> data){
		emptySlots.P();
		mut.P();
		q.push(data);
		mut.V();
		fullSlots.V();
	
		// follow the class lecture pseudocode
				
		//1. Perform necessary waiting (by calling wait on the right semaphores and mutexes),
		//2. Push the data onto the queue
		//3. Do necessary unlocking and notification
		
		
	}

	vector<char> pop(){
		fullSlots.P();
		mut.P();
		vector<char> data = q.front();
		q.pop();

		mut.V();
		emptySlots.V();
		return data;
		//1. Wait using the correct sync variables 
		//2. Pop the front item of the queue. 
		//3. Unlock and notify using the right sync variables
		//4. Return the popped vector
	}
};

#endif /* BoundedBuffer_ */
