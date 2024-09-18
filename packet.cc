#include <iostream>
#include "packet.h"

using namespace std;

/**
 * @brief Construct a new packet::packet object
 * 
 * @param size Size of a packet (bytes)
 * @param arrival Time that packet arrived
 * @param dest Destination of packet is sending to
 */
packet::packet (int size, double arrival, destination dest) 
{
	m_size			= size; 
	m_arrival		= arrival; 
	m_destination	= dest;
};

/**
 * @brief Comparision
 * 
 * @param aPacket 
 * @return true 
 * @return false 
 */

bool packet::operator==(packet &aPacket) 
{
        if ((this->m_size == aPacket.m_size)&&(this->m_arrival==aPacket.m_arrival)&&(this->m_destination==aPacket.m_destination))
        { 
          return true;
        } else {
          return false;
        }
};

/**
 * @brief Destroy the packet::packet object
 * 
 */
packet::~packet() {};

