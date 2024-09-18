#ifndef EVENT_H
#define EVENT_H

#include "destination.h"

class event {
	public:
		int m_type; 		//0: arrival 1: transmission	  		
		destination m_dest;   //for an arrival: the corresponding destination 
		double m_time;		//time of this event (reach the destination)
		friend std::ostream& operator<<(std::ostream& os, const event& evt);  //This is the syntax for overloading the << operator. The operator is used to insert data into an output stream (like std::cout).
	
		event(int type, destination dest, double time);
		~event();
		void print();
}; 

#endif
