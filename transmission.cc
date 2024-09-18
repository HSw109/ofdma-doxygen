#include <iostream>
#include <set>
#include <list>
#include <vector>
#include <algorithm>
#include <random>
#include "transmission.h"

using namespace std;

//IMPORTANT: All the times in this file are given in micro seconds (mu seconds)

/* Discipline
 * 0: FIFO 
 * 1: FIFO with aggregation
 * 2: FIFO with OFDMA
 * 3: FIFO with hybrid aggregation and OFDMA
 *
 */ 

//1 spatial stream and GI=800microsec
//Line: MCS (from 0 to 11) 
//7 Column: RU-26 RU-52 RU-106 RU-242(20MHz) RU-484 (40MHz) RU-996 (80MHz) RU-2x996 (160MHz)
/**
 * @brief List of physical rate with special Resource Units
 * 
 */
double phyRate[12][7] = {
			{0.9,	1.8,	3.8,	8.6,	17.2,	36.0,	72.1},
			{1.8,	3.5,	7.5,	17.2,	34.4,	72.1,	144.1},
			{2.6,	5.3,	11.3,	25.8,	51.6,	108.1,	216.2},
			{3.5,	7.1,	15.0,	34.4,	68.8,	144.1,	288.2},
			{5.3,	10.6,	22.5,	51.6,	103.2,	216.2,	432.4},
			{7.1,	14.1,	30.0,	68.8,	137.6,	288.2,	576.5},
			{7.9,	15.9,	33.8,	77.4,	154.9,	324.3,	648.5},
			{8.8,	17.6,	37.5,	86.0,	172.1,	360.3,	720.6},
			{10.6,	21.2,	45.0,	103.2,	206.5,	432.4,	864.7},
			{11.8,	23.5,	50.0,	114.7, 	229.4, 	480.4, 	960.7},
			{-1.0,	-1.0,	-1.0,	129.0,	258.1, 	540.4,	1080.9},
			{-1.0,	-1.0,	-1.0,	143.4,	286.8,	600.4,	1201.0}
			};

/* Global variables */
/**
 * @brief Only 20Hz, maximum tones of resource unit = 242, max number of clients = 9
 *        4 resource unit avaiable: 26, 52, 106, 242
 * 
 */
double accessTimeAndAck=132.0; //Time are given in the google sheets 
int bandwidth = 20, maxTones = 242, maxClient = 9, RuType[4] = {26, 52, 106, 242};      // Ru is resources unit
//int bandwidth = 40, maxTones = 484, maxClient = 18, maxRuType = 5;
//int bandwidth = 80, maxTones = 996, maxClient = 37, maxRuType = 6;
//int bandwidth = 160, maxTones = 1992, maxClient = 73, maxRuType = 7;

/* functions */
/**
 * @brief Get the Mac Transmission Time of a packet (Layer 2: Data link layer)
 * 
 * @param aPacket 
 * @param nbOfBytesMac  
 * @param ru 
 * @return double 
 */
double getMacTransmissionTime(packet aPacket, int nbOfBytesMac, int ru)//ru: ressource unit  (number of tones)
{
   switch(ru) 
   {
	   case 26:   return (8* (aPacket.m_size+nbOfBytesMac) ) / (phyRate[aPacket.m_destination.m_mcs][0]*1000000); break;
	   case 52:   return (8* (aPacket.m_size+nbOfBytesMac) ) / (phyRate[aPacket.m_destination.m_mcs][1]*1000000); break;
	   case 106:  return (8* (aPacket.m_size+nbOfBytesMac) ) / (phyRate[aPacket.m_destination.m_mcs][2]*1000000); break;
	   case 242:  return (8* (aPacket.m_size+nbOfBytesMac) ) / (phyRate[aPacket.m_destination.m_mcs][3]*1000000); break;
	   case 484:  return (8* (aPacket.m_size+nbOfBytesMac) ) / (phyRate[aPacket.m_destination.m_mcs][4]*1000000); break;
	   case 996:  return (8* (aPacket.m_size+nbOfBytesMac) ) / (phyRate[aPacket.m_destination.m_mcs][5]*1000000); break;
	   case 1992: return (8* (aPacket.m_size+nbOfBytesMac) ) / (phyRate[aPacket.m_destination.m_mcs][6]*1000000); break;
       default: 
   	    cerr << "Error getTransmissionTimePayload: Number of tones is not supported (ru=" << ru << endl;
   	    exit(1);
   }
};
/**
 * @brief Checking if it is a new destination founded or not
 * 
 * @param aDest 
 * @param destList 
 * @return int (its index if old dest, -1 if new dest)
 */
int newDest(destination aDest, std::list<destination> destList) 
{
  
  int i=0;
  for(auto it=destList.begin();it!=destList.end(); it++)
  {
    if( it->m_no == aDest.m_no ) return(i); // if aDest match any destination same with destList, return its index
    i ++;
  }
  return(-1);
}               // check if the new destination is founded in the destList, if occur => return -1

/**
 * @brief Time for transmit a packet
 * 
 * @param pkt 
 * @return double 
 */
double alpha_i(packet pkt)
{
	return (pkt.m_size*8.0)/(pkt.m_destination.m_phyRate*1000000);
};      // time

/**
 * @brief Get the Transmission Time Ofdma Frame object
 * 
 * @param packetsList List of packets
 * @param ruList      List of Resource Unit that correspond to each packet
 * @return <double>   Return the max transmition time packet's (cause all packet paralel)
 */
double getTransmissionTimeOfdmaFrame(std::list<packet> packetsList, std::vector<int> ruList)
{
  double AIFS=0.000043; //musec
  double SIFS=0.000016; //musec
  double backoff=0.0000675; // Slot_time=9musec * 7.5 (backoff in [0-15])
  double phyHeader=0.000044; 

  //Algorithm: we have a single function for OFDMA, Aggreg and OFDMA+Aggreg
  //So, we may have different packets for the same destination. In this case they are sent in one MPDU (MAC Protocol Data Unit) one after the other.
  std::list<double> macTime;
  std::vector<int>::iterator ruListIt;    // iterator point to each packet object, represent the type of resource unit

  ruListIt = ruList.begin(); //
  for(auto it=packetsList.begin(); it!=packetsList.end(); it++) // each iterator is the packet object
  {
//  cout<<"Test ruList value = "<<*ruListIt<<", tx time = "<<getMacTransmissionTime(*it,30+32,RuType[*ruListIt])<<endl;
    macTime.push_back(getMacTransmissionTime(*it,30+32,RuType[*ruListIt]));   // value of each object
    ruListIt++;                                                               // ruList is a iterator, so *ruListIt reference the index in RuList
  }
  
  double transmissionTime;

  //Two phyHeader: one for the frame and one for the BACK
  transmissionTime=AIFS+backoff+2*phyHeader+SIFS;
  transmissionTime+=*max_element(macTime.begin(), macTime.end()); //return the iterator on the max elt  
        // reference to the max element of iterator => time need to finish all packet is the longest time transmit a packet (cause the parallel)
  return transmissionTime; 
}


int * ruAllocation(std::list<packet> pkt_set) 
{
	int *ru = new int[pkt_set.size()];      // return the new array pointer with number of packet element
	double sum = 0;
	
	for (std::list<packet>::iterator it = pkt_set.begin(); it!=pkt_set.end(); it++) sum += alpha_i(*it); // calculate total time for entire packet list
	
	int i = 0;      // global iterator
	double tonesfloat[pkt_set.size()];    // new array has the size of packet element
	
	for (std::list<packet>::iterator it = pkt_set.begin(); it!=pkt_set.end(); it++)
	{

    /* we only use the case of 20Hz, => maxTones is 242
    =  242 * time of transmission each packet / total transmission time of all packet
    => Then we got a float corressponding to each packet, that a number have direct proportion with the transmission time of packet (or size of packet)
    */

		tonesfloat[i] = maxTones * alpha_i(*it) / sum;    // each "i" corresponding to appropriate packet in packetList
		int distance[2] = {maxTones,0};     // distance[0] = 242, distance[1] = 0
		double tmp = 0;
		for (int j=0; j< sizeof(RuType)/sizeof(RuType[0]); j++)  // iterate from 0 to 3
		{
      /*
      tonesfloat[] = [0;242]
      tmp = [0;242]
      j = [0;3]
      */
			tmp = abs(tonesfloat[i] - RuType[j]);   // Getting value is the gap of packet's tonefloat with each tone (26, 52, 106, 242)

      /*
          242 > tmp => distance = tmp; 
          distance[1] has range [0;3] , represent 26 52 104 242
      */

     // if the previous iterate value of "tmp" is greater than the current "tmp"
     // => take the value of current tmp => finding the closet gap
     // => Then after the loop, we got the array distance is the combined of the closet RuTone and its index .

			if (distance[0] > tmp) { distance[0] = tmp; distance[1] = j; }
		}
		ru[i] = distance[1];    // current closet index of tones
		cout<<tonesfloat[i]<<" "<<ru[i]<<" "<<RuType[ru[i]]<<endl; // print the tonesfloat and appropriate RuTones index
		i++;  // do it for every packet
	}
	
	int totalTones = 0;
	i = 0;
	for (int j=0; j < pkt_set.size(); j++) totalTones += RuType[ru[j]]; // counting the total of all appropriate tones for all packet
	cout<<"[INFO] RU Allocation component, after computing the floattones, total tones = "<<totalTones<<" number of ru size = "<<sizeof(ru)/sizeof(ru[0])<<endl;

  // while the total tones is greater than 242, then we need to reduce tones by minus 1 from all RuTones index (exclusive the first index 0)
	while (totalTones > maxTones)
	{
		if (ru[i]>0) ru[i]--; // if index of tones > 0, then reduce 1 from this index
    // example [0, 2, 1, 3, ..] => [0, 1, 0, 2,..]

		totalTones = 0;
		for (int k=0; k < pkt_set.size(); k++) totalTones += RuType[ru[k]];
		i++;
		cout<<"[INFO] RU Allocation component, total tones too large, reduce tones, total tones = "<<totalTones<<endl;
	}
	return ru;    // => return the array store most appropriate index of RuTones for packets
};

/**
 * @brief Algorithm for generate all possile combination RU list for packet list
 * @param ru blank vector at beginning
 * @param noSta number of packet - 1
 * @param ruList list of vector of value of Ru index (A reference to a list where all possible combinations of RU allocations will be stored.)
 */
void ruBruteForce(std::vector<int> ru, int noSta, std::list<std::vector<int>> &ruList)
{
  for (int i=0; i<sizeof(RuType)/sizeof(RuType[0]); i++) // i: [0;3]
  {
    std::vector<int> str;
    str = ru;       
    str.push_back(i);       
    if (noSta >0)                     // Number of packet - 1
    {
      ruBruteForce(str, noSta-1, ruList);
    } else {
      ruList.push_back(str);
    }
  }
};

// return a vector combines of RuTypes that optimal for packetList
/**
 * @brief Return a vector combines of RuTypes that optimal for packetList
 * 
 * @param packetsList given list of packets
 * @return std::vector<int> 
 */
std::vector<int> ruAllocationOptimal(std::list<packet> packetsList)
{
  std::vector<double> ofValueList; //a vector of objectiveFunction computed values
  std::vector<int>::iterator ofValueIt; 
  
  std::vector<int> optimalValue; //return ru optimal vector

  int noSta = packetsList.size() - 1; //depth of BruteForce recursive layers

  std::vector<int> ru;
  std::vector<int>::iterator ruIt;
  std::list<std::vector<int>> ruList;
  std::list<std::vector<int>>::iterator ruListIt;

  ruBruteForce(ru, noSta, ruList);    // store all possible RU combination with correspond noSta in ruList

  for (ruListIt=ruList.begin(); ruListIt!=ruList.end(); ruListIt++)  // iterate each vector
  {
    int totalTones = 0;
    for (ruIt=ruListIt->begin(); ruIt!=ruListIt->end(); ruIt++)   // iterate each element in vector
    {
      totalTones += RuType[*ruIt]; //compute total tones given by this combination
    }
    if (totalTones > maxTones) 
    {
      ofValueList.push_back(0.0);  //excess max no of tones, simply put 0 to the ofValueList
    } else {
      ofValueList.push_back(objectiveFunction(packetsList, *ruListIt));  //otherwise, compute objectiveFunction value for each vector, then add to ofValueList
                                                                        
    }
  }
// After above code, we have a vector contain all objectiveFunction value for each vector combination

  ruListIt = ruList.begin();
  // find the maximum iterator element in ofValueList, then minus the begin iterator to find the number of positions
  // "advance" used to move given iterator with specific number of position to move
  // shifts ruListIt to point to the RU allocation combination in ruList that corresponds to the maximum value found in ofValueList.
  std::advance(ruListIt, max_element(ofValueList.begin(), ofValueList.end()) - ofValueList.begin()); //find the index of best combination in ofValueList, then shift ruListIt according to that index
  optimalValue = *ruListIt;       // assign value of optimalValue is the value ruListIt pointing to
  return optimalValue;  // vector for optimal RuTones 
};

// calculate objective Function for given packet List and corresponding tones
/**
 * @brief Calculate objective Function for given packet List and corresponding tones
 * 
 * @param packetsList List of packets
 * @param ruList      List of Resource Unit that optimize for packet list
 * @return <double>     Return the Objective Physical Rate for a packet list
 */
double objectiveFunction(std::list<packet> packetsList, std::vector<int> ruList)
{
  std::list<packet>::iterator it;
  double size = 0.0;
  
  for (it = packetsList.begin(); it!= packetsList.end(); it++)
  {
    size += it->m_size;   // total size of packet list
  }

  return size/getTransmissionTimeOfdmaFrame(packetsList, ruList);
  // each packet list have the number that represent rate in OFDMA Frame
};

/**
 * @brief Get the Transmission Time Payload without byteMac (another layer) for a packet
 * 
 * @param aPacket given packet
 * @param ru      resource unit
 * @return <double> 
 */

double getTransmissionTimePayload(packet aPacket, int ru)//ru: ressource unit (number of tones)
{
   switch(ru) 
   {
	   case 26:   return (8*aPacket.m_size) / (phyRate[aPacket.m_destination.m_mcs][0]*1000000); break;
	   case 52:   return (8*aPacket.m_size) / (phyRate[aPacket.m_destination.m_mcs][1]*1000000); break;
	   case 106:  return (8*aPacket.m_size) / (phyRate[aPacket.m_destination.m_mcs][2]*1000000); break;
	   case 242:  return (8*aPacket.m_size) / (phyRate[aPacket.m_destination.m_mcs][3]*1000000); break;
	   case 484:  return (8*aPacket.m_size) / (phyRate[aPacket.m_destination.m_mcs][4]*1000000); break;
	   case 996:  return (8*aPacket.m_size) / (phyRate[aPacket.m_destination.m_mcs][5]*1000000); break;
	   case 1992: return (8*aPacket.m_size) / (phyRate[aPacket.m_destination.m_mcs][6]*1000000); break;
       default: 
   	    cerr << "Error getTransmissionTimePayload: Number of tones is not supported (ru=" << ru << endl;
   	    exit(1);
   }
};

/**
 * @brief Calculate transmission time for FIFO
 * 
 * @param buffer 
 * @param currentTime 
 * @return double 
 */
double fifo(list<packet> &buffer, double currentTime)
{
  double transmissionTime;
  double AIFS=0.000043; //musec
  double SIFS=0.000016; //musec
  double backoff=0.0000675; // Slot_time=9musec * 7.5 (backoff in [0-15])
  double phyHeader=0.000044; 
  //std::list<packet>::iterator it;
  //cout<<"[INFO]: FIFO component, before pop out packet, buffer size = "<<buffer.size()<<endl;
  packet pckt = buffer.front();
  buffer.pop_front();
  //cout<<"[INFO]: FIFO component, after pop out packet, buffer size = "<<buffer.size()<<endl;
  //cout<<"[INFO]: FIFO component, destination no  = "<<pckt.m_destination.m_no<<endl;
  //cout<<"[INFO]: FIFO component, packet size = "<<pckt.m_size<<endl;
  cout<<"[INFO]: FIFO component, packet sent with destination = "<<pckt.m_destination.m_no
      <<", phyRate = "<<pckt.m_destination.m_phyRate
      <<", packetSize = "<<pckt.m_size
      <<", arrivalTime = "<<pckt.m_arrival
      <<", transmissionTime = "<<currentTime 
      <<endl;
  transmissionTime = AIFS+backoff+2*phyHeader+SIFS;
  transmissionTime+= (8*pckt.m_size)/(pckt.m_destination.m_phyRate*1000000);
  return transmissionTime;
};

/**
 * @brief Calculate transmission time for FIFO Aggregation, FIFO represents the process that you take the first packet in the packet list, check if any packets that
 * have the same destination with the selected packet, aggregate all of thems into a frame called "Jumbo frame", then sending this jumbo to the corresponding destination, simultaneously
 * erase the packet in the initial buffer. Do this till the buffer empty and calculate the total time for transmit them. CAREFUL: if aggregation happens then no need to increase iterator since after erase() the iterator 
 * automatically point to the next item.
 * 
 * @param buffer List of packets
 * @param currentTime 
 * @param txOP Transmission Opportunity: Max time for a transmission occurs
 * @param aggregation_frame_size Default max size for an aggregated frame
 * @param frame_header_size Size of aggregated frame header
 * @param no_aggregated_pkt Counter for aggregated packet
 * @param aggregation Flag represents aggregation (yes or no)
 * @return <double> Transmission time 
 */
double fifoAggregation(std::list<packet> &buffer, double currentTime) 
{
	double txOP = 10.5;	//TxOP in miliseconds  Transmission Opportunity
	double aggregation_frame_size = 11454;	//Max aggregation frame size in Bytes
	double max_frame_size, current_frame_size=0.0;
  double frame_header_size = 14.0;
	int no_aggregated_pkt = 0;
	bool aggregation = 0;
	
  std::cout<<"[TRANSMISSION] fifoAggregation is processing..."<<std::endl;
	cout<<"[INFO]: FIFO_Aggregation component, buffer size before pop out the first packet = "<<buffer.size()<<endl;

	// If there is one, verify the frame size then aggregate it into the first packet, 
	// pop out the aggregated packet out of the buffer, count the number of packet to be aggregated,
	// if aggregation happens then no need to increase iterator since after erase() the iterator 
	// automatically point to the next item.
	std::list<packet>::iterator it;
  std::list<packet>::iterator packetIt;
  std::vector<int> destList;

  int numDest = 0;
  double totalTime = 0;

  for (it = buffer.begin(); it != buffer.end(); ++it) {
    destList.push_back(it->m_destination.m_no);  // e.g., [1,1,1,1,5,5,5,8,8]
  }

  for (auto it = destList.begin(); it != destList.end(); ++it) {
      bool isUnique = true;
      for (auto checkIt = destList.begin(); checkIt != it; ++checkIt) {
          if (*checkIt == *it) {
              isUnique = false;
              break;
          }
      }
      if (isUnique) {
          numDest++;
      }
  }

  std::cout << "Number of unique destinations: " << numDest << std::endl;

while (numDest > 0)
  {
    packet pckt = buffer.front();     // store first packet of packet list
	  buffer.pop_front();               // remove
    cout<< "\n" << "[Begin of FIFO Aggregation to destination " << pckt.m_destination.m_no << "]\n" << endl;
	  cout<<"   FIFO_Aggregation component, buffer size after pop out the first packet = "<<buffer.size()<<endl;
    cout<<"   Max size for a transmission = "<< pckt.m_destination.m_phyRate*txOP*1000/8<<endl;
	  max_frame_size = min(aggregation_frame_size, pckt.m_destination.m_phyRate*txOP*1000/8);
    current_frame_size = pckt.m_size;
	  cout<<"   Max frame size for this jumbo frame = "<<max_frame_size <<"\n   Current frame size = "
        <<current_frame_size<<endl;
    packetIt = buffer.begin();                // second packet of given list
    int loop = buffer.size();         // number of packet = 8

  while (loop > 0)     // query packet list till all packet is processed
    {
      cout<<"   Processing a packet with destination = "<<packetIt->m_destination.m_no
        <<", buffer size = "<<buffer.size()<<endl;
      aggregation = 0;

      // if the current packet has the same destination with the first packet
      if (packetIt->m_destination.m_no == pckt.m_destination.m_no)
      {
        // if the frame size =  first packet + same destination packet  <= max frame size
        if (current_frame_size + packetIt->m_size <= max_frame_size)
        {
          // added this packet to frame
          current_frame_size += packetIt->m_size;

          // count the number of aggregated packet
          no_aggregated_pkt += 1;    

          // erase the packet that used for aggregation FIFO in buffer
          packetIt = buffer.erase(packetIt); // both erase and update
          // set bool to 1
          aggregation = 1;
          cout<<"   [AGGREGATED] Aggregated a packet, current frame size = "<<current_frame_size
            <<", number of aggregated packet = "<<no_aggregated_pkt<<", now buffer size = "<<buffer.size()<<endl;
        } 
      }

      // if not the same destination
      if (aggregation == 0) 
      {
        cout<<"   [NOT AGGREGATED]: Aggregation not happen, buffer size = "<<buffer.size()<<endl;
        // do with the next packet
        packetIt++;
      }
      // - 1 packet, restart the loop
      loop--;
      cout<< "   Jumbo frame size of packet = " << current_frame_size <<"\n" <<endl;
    }
  double transmisstion_time=(8*current_frame_size+8*frame_header_size)/(pckt.m_destination.m_phyRate*1000000);
  totalTime+=transmisstion_time;
  cout<< "   Time for transmisstion jumbo frame to destination " << pckt.m_destination.m_no<< " = " <<transmisstion_time<<endl;
  numDest--;
  }
  cout<<endl<< "\nFinal totalTime for transmit = "<<totalTime<<endl;
  return(totalTime);
};

/**
 * @brief Calculate transmission time for FIFO OFDMA Optimal, which sends all packet has different destination in the buffer (cause the paralel, so can not send 2 or more packets to the same destination in the same time) 
 * . The combination of sent packet is depend on the Objective Function value of every combination packet in the buffer. FIFO here is the prior process taking the first packet1:
 * Example: [[packet 1], [packet1, packet2], [packet1, packet2, packet3], ...] (each of this packet has different destination)
 * => The packets combination will be bruteforce with each combination of Resource Unit to find the Optimal Resource Unit combination correspond to packet combination.
 * Do the same sequence with every combination => Result the most optimal packet combination (biggest Objective Function). Then calculate time for transfer this combination and delete it from initial buffer.
 * @param buffer Current list of packet queueing
 * @param currentTime 
 * @param destList Store the destination of "new destination" packet
 * @param tmpList Temporary list for storing "new destination" packet object
 * @param newDestList List of List of packets: [[packet 1], [packet1, packet2], [packet1, packet2, packet3], ...]
 * @return <double> Transmission time  
 */
double fifoOFDMAOptimal(std::list<packet> &buffer, double currentTime)
{
  double txTime;
  std::vector<double> ofValueList;
  std::list<destination> destList;    // store all destination of packets
  std::list<packet> tmpList;
  std::list<packet>::iterator packetIt,bufferIt;
  std::list<std::list<packet>> newDestList;
  std::list<std::list<packet>>::iterator newDestListIt;

  std::cout<<"[TRANSMISSION] fifoOFDMAOptimal is processing..."<<std::endl;
  std::cout<<" buffersize = "<<buffer.size()<<std::endl; // initial number of packets

  packetIt = buffer.begin();  // get the first packet
  int index = 0;              // iterator of clients
  int i = 1;                  // index of fifoOFDMAOptimal combination
  while ((packetIt != buffer.end()) && (index < maxClient))
  {
    // modify destList will affect all this function
    if (newDest(packetIt->m_destination.m_no, destList) == -1) //found packet with new destination
    {
      tmpList.push_back(*packetIt); // added the packet object that iterator point to end of temporary list

      /*
      ruAllocationOptimal return vector for optimal combinations of Ru Tones for all packet in tmpList
      ofValueList store all rate of optimal combination RuTones for packetList
      */
      ofValueList.push_back(objectiveFunction(tmpList, ruAllocationOptimal(tmpList))); // This also calculate the oF of [[packet 1], [packet1, packet2], ...] 
      destList.push_back(packetIt->m_destination.m_no); // store list of address of destination of iterated packet
      newDestList.push_back(tmpList);   // LIST of LIST packet
      // example: [[packet 1], [packet1, packet2], ...]  but all of this packet has new destination
      index++;
      packetIt++;
    } else {
      packetIt++; //destination already in the list, iterator point to the next packet
    }
  }

  //update
  for (newDestListIt = newDestList.begin(); newDestListIt != newDestList.end(); newDestListIt++ ){
            std::vector<int> optimalRU = ruAllocationOptimal(*newDestListIt);
            for (auto it = newDestListIt->begin(); it!= newDestListIt->end(); it++){
              std::cout<< " phyRate = " << it->m_destination.m_phyRate
                  << " packetSize = " << it->m_size
                  << " arrivalTime = " << it->m_arrival
                  << " Transsmisstion = " << currentTime
                  <<endl;
            }
            std::cout<< " Ru optimal combination for this packetList ";
            for (int j = 0; j < optimalRU.size(); j++){
            std::cout<< RuType[optimalRU[j]] << " ";
            }
            std::cout<<endl<<" Objective Function value = "<<objectiveFunction(*newDestListIt, ruAllocationOptimal(*newDestListIt))<<std::endl;
            std::cout<< " End of combination = " << i <<endl << " " <<endl;
            i++;
      }

  //point iterator to the right list of packet in the new destination list, then pop them out from the buffer
  newDestListIt = newDestList.begin();
  // newDestListIt point to the max Objective Function of packet's set have new destination
  std::advance(newDestListIt, max_element(ofValueList.begin(),ofValueList.end())-ofValueList.begin());
  tmpList.clear();

  // Now, newDestListIt is pointed to the OPTIMALEST COMBINATION PACKETS
  // Iterate each combination of packets in the OPTIMAL
  for (packetIt = newDestListIt->begin(); packetIt != newDestListIt->end(); packetIt++)
  {
    tmpList.push_back(*packetIt); // add all element of packet's combination to the tmpList
    for (bufferIt = buffer.begin(); bufferIt != buffer.end(); bufferIt++) // iterate all packet in initial packet List
    {
      if (*packetIt == *bufferIt) {     // if the packet is founded in initial buffer, clear and print its details to screen
        bufferIt = buffer.erase(bufferIt); bufferIt--;  // remove the FIFO OFDMA packet
        cout<<"[INFO]: fifoOFDMAOptimal component, packet sent with destination = "<<packetIt->m_destination.m_no
            <<", phyRate = "<<packetIt->m_destination.m_phyRate
            <<", packetSize = "<<packetIt->m_size
            <<", arrivalTime = "<<packetIt->m_arrival
            <<", transmissionTime = "<<currentTime 
            <<endl;
      }
    }
  }
  
  // calculate time for this combination

  txTime = getTransmissionTimeOfdmaFrame(tmpList, ruAllocationOptimal(tmpList));
  cout<<" Transmission time = " << txTime <<endl;
  return txTime;
};

//Return the transmission time (time to access the medium + payload + queue + SIFS + BACK)
double transmitNextPackets(int discipline, list<packet> &buffer, double currentTime)
{
  double txTime=0.0;
  switch(discipline)
  {
    case 0: txTime = fifo(buffer, currentTime); break;
    case 1: txTime = fifoOFDMAOptimal(buffer, currentTime); break;
    case 2: txTime = fifoAggregation(buffer,currentTime); break;
    default: 
	    cerr << "Error transmitNextPackets: the discipline is not supported (disc=" << discipline << endl;
	    exit(1);
  }
  return txTime;
};
