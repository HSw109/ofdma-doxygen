#include <iostream>
#include <list>
#include <set>
#include <vector>
#include <random>
#include "packet.h"
#include "destination.h"
#include "event.h"
#include "transmission.h"

using namespace std;

int main(int argc, char* argv[])
{
  //Set destination 
  // all mcs index is 2, 4 packet/s
  // Poisson distribution

  // destination list
  class destination dest1(1,2,2.0,1), dest2(2,2,4.0,1), dest3(3,2,4.0,1), dest4(4,2,4.0,1), dest5(5,2,4.0,1), dest6(6,2,4.0,1), dest7(7,2,4.0,1), dest8(8,2,4.0,1), dest9(9,2,4.0,1); 
  std::list<destination> dest_list;
  dest_list.push_back(dest1);
  dest_list.push_back(dest2);
  dest_list.push_back(dest3);
  dest_list.push_back(dest4);  
  dest_list.push_back(dest5);
  dest_list.push_back(dest6);
  dest_list.push_back(dest7);
  dest_list.push_back(dest8);
  dest_list.push_back(dest9);

  // 5000 bytes per packet
  // time packet arrival
  // 9 packet to 9 destination
  class packet pkt1(5000,2,dest1), pkt2(5000,1,dest2), pkt3(5000,1,dest3), pkt4(5000,1,dest4), pkt5(5000,1,dest5), pkt6(5000,1,dest6), pkt7(5000,1,dest7), pkt8(5000,1,dest8), pkt9(5000,1,dest9);
  std::list<packet> buffer;
  buffer.push_back(pkt1);
  buffer.push_back(pkt2);
  buffer.push_back(pkt3);
  buffer.push_back(pkt4);
  buffer.push_back(pkt5);
  buffer.push_back(pkt6);
  buffer.push_back(pkt7);
  buffer.push_back(pkt8);
  buffer.push_back(pkt9);

  class packet pktt1(500,2,dest1), pktt2(500,1,dest1), pktt3(100,1,dest2), pktt4(100,2,dest2), pktt5(50,1,dest5), pktt6(50,2,dest1), pktt7(50,3,dest5), pktt8(50,1,dest8), pktt9(50,2,dest2);
  std::list<packet> buffer1;
  buffer1.push_back(pktt1);
  buffer1.push_back(pktt2);
  buffer1.push_back(pktt3);
  buffer1.push_back(pktt4);
  buffer1.push_back(pktt5);
  buffer1.push_back(pktt6);
  buffer1.push_back(pktt7);
  buffer1.push_back(pktt8);
  buffer1.push_back(pktt9);


// initial blank RuTones
  std::vector<int> ruList;
  ruList.push_back(0); ruList.push_back(0);  ruList.push_back(0);  ruList.push_back(0);  ruList.push_back(0);  ruList.push_back(0);  ruList.push_back(0);
  ruList.push_back(0);  ruList.push_back(0);

  cout<<"OFDMA Tx Time = "<<getTransmissionTimeOfdmaFrame(buffer, ruList)<<endl;
  cout<<"ObjectiveFunction = "<<objectiveFunction(buffer, ruList)<<endl;

//   cout<<"buffer size before call fifoOFDMA = "<<buffer.size()<<endl;
//   cout<<"txTime for OFDMA = "<<fifoOFDMAOptimal(buffer, 0)<<endl;
//   cout<<"buffer size after call fifoOFDMA = "<<buffer.size()<<endl;

  cout<<"buffer size before call fifoOFDMA = "<<buffer1.size()<<endl;
  cout<<"txTime for OFDMA = "<<fifoAggregation(buffer1, 0)<<endl;
  cout<<"buffer size after call fifoOFDMA = "<<buffer1.size()<<endl;

  return(0);
}
