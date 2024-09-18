#include <iostream>
#include "event.h"

using namespace std;

/**
 * @brief Shortcut of operator "<<" (for printing event details)
 */
std::ostream& operator<<(std::ostream& os, const event& evt)
{
    if( evt.m_type==0 ) os << "Event: arrival / ";	
    else os << "Event: transmission / ";	

    os << "dest= " << evt.m_dest.m_no << " / time = " << evt.m_time << endl;;  // Print data of this event (transmission or arrival) and time this event occur
    return os;
};

// Constructor
/**
 * @brief Construct a new event::event object
 * 
 * @param type Arrival or Transmission
 * @param dest Correspond destination that event pointed
 * @param time Time that event occurs
 */
event::event(int type, destination dest, double time)
{
	m_type	= type;
	m_dest	= dest;
	m_time	= time;
};

/**
 * @brief Destroy the event::event object
 * 
 */
event::~event() {};

/**
 * @brief Print details of an event
 * 
 */
void event::print() 
{
	if (m_type == 0) cout<<"Arrival event / "; else cout<<"Transmission event / ";
	cout<<"dest = "<<m_dest.m_no<<" /time = "<<m_time<<endl;
};