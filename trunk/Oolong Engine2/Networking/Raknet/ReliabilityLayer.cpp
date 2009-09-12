/// \file
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "ReliabilityLayer.h"
#include "GetTime.h"
#include "SocketLayer.h"
#include "PluginInterface2.h"
#include "RakAssert.h"
#include "Rand.h"
#include "MessageIdentifiers.h"
#include <math.h>

// Can't figure out which library has this function on the PS3
double Ceil(double d) {if (((double)((int)d))==d) return d; return (int) (d+1.0);}

#if defined(new)
#pragma push_macro("new")
#undef new
#define RELIABILITY_LAYER_NEW_UNDEF_ALLOCATING_QUEUE
#endif


//#define _DEBUG_LOGGER

static const int DEFAULT_HAS_RECEIVED_PACKET_QUEUE_SIZE=512;
static const RakNetTimeUS MAX_TIME_BETWEEN_PACKETS= 350000; // 350 milliseconds
static const RakNetTimeUS STARTING_TIME_BETWEEN_PACKETS=MAX_TIME_BETWEEN_PACKETS;
static const RakNetTimeUS HISTOGRAM_RESTART_CYCLE=10000000; // Every 10 seconds reset the histogram
static const long double TIME_BETWEEN_PACKETS_INCREASE_MULTIPLIER_DEFAULT=.02;
static const long double TIME_BETWEEN_PACKETS_DECREASE_MULTIPLIER_DEFAULT=1.0 / 9.0;

typedef uint32_t BitstreamLengthEncoding;

#ifdef _MSC_VER
#pragma warning( push )
#endif

struct DatagramHeaderFormat
{
	// Time must be first, see SendToThread.cpp
	RakNetTimeUS sourceSystemTime;
	DatagramSequenceNumberType datagramNumber;
	//BytesPerMicrosecond B;
	//BytesPerMicrosecond AS;
	// Use floats to save bandwidth
	float B;
	float AS;
	// Not endian safe
	/*
	uint8_t isACK:1;
	uint8_t isNAK:1;
	uint8_t isPacketPair:1;
	uint8_t hasBAndAS:1;
	*/
	bool isACK;
	bool isNAK;
	bool isPacketPair;
	bool hasBAndAS;
	bool isContinuousSend;
	bool isValid; // To differentiate between what I serialized, and offline data

	static BitSize_t GetDataHeaderBitLength()
	{
		return BYTES_TO_BITS(GetDataHeaderByteLength());
	}

	static BitSize_t GetDataHeaderByteLength()
	{
		//return 2 + sizeof(DatagramSequenceNumberType) + sizeof(RakNetTimeMS) + sizeof(float)*2;
		return 2 + 3 + sizeof(RakNetTimeMS) + sizeof(float)*2;
	}

#ifdef USE_THREADED_SEND
	void Serialize(SendToThread::SendToThreadBlock *b)
	{
		memcpy(b->data+b->dataWriteOffset, this, sizeof(DatagramHeaderFormat));
		b->dataWriteOffset+=sizeof(DatagramHeaderFormat);
	}
#endif

	void Serialize(RakNet::BitStream *b)
	{
		// Not endian safe
		//		RakAssert(GetDataHeaderByteLength()==sizeof(DatagramHeaderFormat));
		//		b->WriteAlignedBytes((const unsigned char*) this, sizeof(DatagramHeaderFormat));
		//		return;

		b->Write(true); // IsValid
		if (isACK)
		{
			b->Write(true);
			b->Write(hasBAndAS);
			b->AlignWriteToByteBoundary();
			RakNetTimeMS timeMSLow=(RakNetTimeMS) sourceSystemTime&0xFFFFFFFF; b->Write(timeMSLow);
			if (hasBAndAS)
			{
				b->Write(B);
				b->Write(AS);
			}
		}
		else if (isNAK)
		{
			b->Write(false);
			b->Write(true);
		}
		else
		{
			b->Write(false);
			b->Write(false);
			b->Write(isPacketPair);
			b->Write(isContinuousSend);
			b->AlignWriteToByteBoundary();
			RakNetTimeMS timeMSLow=(RakNetTimeMS) sourceSystemTime&0xFFFFFFFF; b->Write(timeMSLow);
			b->Write(datagramNumber);
		}
	}
	void Deserialize(RakNet::BitStream *b)
	{
		// Not endian safe
		//		b->ReadAlignedBytes((unsigned char*) this, sizeof(DatagramHeaderFormat));
		//		return;

		b->Read(isValid);
		b->Read(isACK);
		if (isACK)
		{
			isNAK=false;
			isPacketPair=false;
			b->Read(hasBAndAS);
			b->AlignReadToByteBoundary();
			RakNetTimeMS timeMS; b->Read(timeMS); sourceSystemTime=(RakNetTimeUS) timeMS;
			if (hasBAndAS)
			{
				b->Read(B);
				b->Read(AS);
			}
		}
		else
		{
			b->Read(isNAK);
			if (isNAK)
			{
				isPacketPair=false;
			}
			else
			{
				b->Read(isPacketPair);
				b->Read(isContinuousSend);
				b->AlignReadToByteBoundary();
				RakNetTimeMS timeMS; b->Read(timeMS); sourceSystemTime=(RakNetTimeUS) timeMS;
				b->Read(datagramNumber);
			}
		}
	}
};

#pragma warning(disable:4702)   // unreachable code

#ifdef _WIN32
//#define _DEBUG_LOGGER
#ifdef _DEBUG_LOGGER
#include "WindowsIncludes.h"
#endif
#endif

//#define DEBUG_SPLIT_PACKET_PROBLEMS
#if defined (DEBUG_SPLIT_PACKET_PROBLEMS)
static int waitFlag=-1;
#endif

using namespace RakNet;

int SplitPacketChannelComp( SplitPacketIdType const &key, SplitPacketChannel* const &data )
{
	if (key < data->splitPacketList[0]->splitPacketId)
		return -1;
	if (key == data->splitPacketList[0]->splitPacketId)
		return 0;
	return 1;
}


// DEFINE_MULTILIST_PTR_TO_MEMBER_COMPARISONS( InternalPacket, SplitPacketIndexType, splitPacketIndex )

bool operator<( const DataStructures::MLKeyRef<SplitPacketIndexType> &inputKey, const InternalPacket *cls )
{
	return inputKey.Get() < cls->splitPacketIndex;
}
bool operator>( const DataStructures::MLKeyRef<SplitPacketIndexType> &inputKey, const InternalPacket *cls )
{
	return inputKey.Get() > cls->splitPacketIndex;
}
bool operator==( const DataStructures::MLKeyRef<SplitPacketIndexType> &inputKey, const InternalPacket *cls )
{
	return inputKey.Get() == cls->splitPacketIndex;
}
/// Semi-hack: This is necessary to call Sort()
bool operator<( const DataStructures::MLKeyRef<InternalPacket *> &inputKey, const InternalPacket *cls )
{
	return inputKey.Get()->splitPacketIndex < cls->splitPacketIndex;
}
bool operator>( const DataStructures::MLKeyRef<InternalPacket *> &inputKey, const InternalPacket *cls )
{
	return inputKey.Get()->splitPacketIndex > cls->splitPacketIndex;
}
bool operator==( const DataStructures::MLKeyRef<InternalPacket *> &inputKey, const InternalPacket *cls )
{
	return inputKey.Get()->splitPacketIndex == cls->splitPacketIndex;
}
/*
int SplitPacketIndexComp( SplitPacketIndexType const &key, InternalPacket* const &data )
{
	if (key < data->splitPacketIndex)
		return -1;
	if (key == data->splitPacketIndex)
		return 0;
	return 1;
}
*/

//-------------------------------------------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------------------------------------------
// Add 21 to the default MTU so if we encrypt it can hold potentially 21 more bytes of extra data + padding.
ReliabilityLayer::ReliabilityLayer() :
updateBitStream( MAXIMUM_MTU_SIZE + 21 )   // preallocate the update bitstream so we can avoid a lot of reallocs at runtime
{
	freeThreadedMemoryOnNextUpdate = false;
#ifdef _DEBUG
	// Wait longer to disconnect in debug so I don't get disconnected while tracing
	timeoutTime=30000;
#else
	timeoutTime=10000;
#endif

#ifndef _RELEASE
	minExtraPing=extraPingVariance=0;
	maxSendBPS=(double) minExtraPing;	
#endif

	InitializeVariables(MAXIMUM_MTU_SIZE);
}

//-------------------------------------------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------------------------------------------
ReliabilityLayer::~ReliabilityLayer()
{
	FreeMemory( true ); // Free all memory immediately
}
//-------------------------------------------------------------------------------------------------------
// Resets the layer for reuse
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::Reset( bool resetVariables, int MTUSize )
{
	FreeMemory( true ); // true because making a memory reset pending in the update cycle causes resets after reconnects.  Instead, just call Reset from a single thread
	if (resetVariables)
	{
		InitializeVariables(MTUSize);

		if ( encryptor.IsKeySet() )
			congestionManager.Init(RakNet::GetTimeUS(), MTUSize - UDP_HEADER_SIZE - 16 - DatagramHeaderFormat::GetDataHeaderByteLength());
		else
			congestionManager.Init(RakNet::GetTimeUS(), MTUSize - UDP_HEADER_SIZE - DatagramHeaderFormat::GetDataHeaderByteLength());
	}
}

//-------------------------------------------------------------------------------------------------------
// Sets up encryption
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SetEncryptionKey( const unsigned char* key )
{
	if ( key )
		encryptor.SetKey( key );
	else
		encryptor.UnsetKey();
}

//-------------------------------------------------------------------------------------------------------
// Set the time, in MS, to use before considering ourselves disconnected after not being able to deliver a reliable packet
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SetTimeoutTime( RakNetTime time )
{
	timeoutTime=time;
}

//-------------------------------------------------------------------------------------------------------
// Returns the value passed to SetTimeoutTime. or the default if it was never called
//-------------------------------------------------------------------------------------------------------
RakNetTime ReliabilityLayer::GetTimeoutTime(void)
{
	return timeoutTime;
}

//-------------------------------------------------------------------------------------------------------
// Initialize the variables
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::InitializeVariables( int MTUSize )
{
	(void) MTUSize;

	memset( waitingForOrderedPacketReadIndex, 0, NUMBER_OF_ORDERED_STREAMS * sizeof(OrderingIndexType));
	memset( waitingForSequencedPacketReadIndex, 0, NUMBER_OF_ORDERED_STREAMS * sizeof(OrderingIndexType) );
	memset( waitingForOrderedPacketWriteIndex, 0, NUMBER_OF_ORDERED_STREAMS * sizeof(OrderingIndexType) );
	memset( waitingForSequencedPacketWriteIndex, 0, NUMBER_OF_ORDERED_STREAMS * sizeof(OrderingIndexType) );
	memset( &statistics, 0, sizeof( statistics ) );
	statistics.connectionStartTime = RakNet::GetTime();
	splitPacketId = 0;
	elapsedTimeSinceLastUpdate=0;
	throughputCapCountdown=0;
	sendReliableMessageNumberIndex = 0;
	internalOrderIndex=0;
	lastUpdateTime= RakNet::GetTimeNS();
//	lastTimeBetweenPacketsIncrease=lastTimeBetweenPacketsDecrease=0;
	bandwidthExceededStatistic=false;
	remoteSystemTime=0;
	unreliableTimeout=0;
	countdownToNextPacketPair=0;
	// lastPacketSendTime=retransmittedFrames=sentPackets=sentFrames=receivedPacketsCount=bytesSent=bytesReceived=0;

	nextAllowedThroughputSample=0;
	deadConnection = cheater = false;
	timeOfLastContinualSend=0;

	timeResendQueueNonEmpty = 0;
//	packetlossThisSample=false;
//	backoffThisSample=0;
//	packetlossThisSampleResendCount=0;
//	lastPacketlossTime=0;
	numPacketsOnResendBuffer=0;

	receivedPacketsBaseIndex=0;
	resetReceivedPackets=true;
	receivePacketCount=0; 

	//	SetPing( 1000 );

	timeBetweenPackets=STARTING_TIME_BETWEEN_PACKETS;

	ackPingIndex=0;
	ackPingSum=(RakNetTimeUS)0;

	nextSendTime=lastUpdateTime;
	//nextLowestPingReset=(RakNetTimeUS)0;
//	continuousSend=false;

//	histogramStart=(RakNetTimeUS)0;
//	histogramBitsSent=0;
	unacknowledgedBytes=0;
	resendLinkedListHead=0;
	totalUserDataBytesAcked=0;

	resetHistogramTime=0;
	bytesReceivedThisHistogram=0;
	bytesSentThisHistogram=0;
	bytesReceivedLastHistogram=0;
	bytesSentLastHistogram=0;

}

//-------------------------------------------------------------------------------------------------------
// Frees all allocated memory
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::FreeMemory( bool freeAllImmediately )
{
	if ( freeAllImmediately )
	{
		FreeThreadedMemory();
		FreeThreadSafeMemory();
	}
	else
	{
		FreeThreadSafeMemory();
		freeThreadedMemoryOnNextUpdate = true;
	}
}

void ReliabilityLayer::FreeThreadedMemory( void )
{
}

void ReliabilityLayer::FreeThreadSafeMemory( void )
{
	unsigned i,j;
	InternalPacket *internalPacket;

	ClearPacketsAndDatagrams();

	for (i=0; i < splitPacketChannelList.Size(); i++)
	{
		for (j=0; j < splitPacketChannelList[i]->splitPacketList.Size(); j++)
		{
			rakFree_Ex(splitPacketChannelList[i]->splitPacketList[j]->data, __FILE__, __LINE__ );
			ReleaseToInternalPacketPool( splitPacketChannelList[i]->splitPacketList[j] );
		}
		RakNet::OP_DELETE(splitPacketChannelList[i], __FILE__, __LINE__);
	}
	splitPacketChannelList.Clear();

	while ( outputQueue.Size() > 0 )
	{
		internalPacket = outputQueue.Pop();
		rakFree_Ex(internalPacket->data, __FILE__, __LINE__ );
		ReleaseToInternalPacketPool( internalPacket );
	}

	outputQueue.ClearAndForceAllocation( 32 );

	for ( i = 0; i < orderingList.Size(); i++ )
	{
		if ( orderingList[ i ] )
		{
			DataStructures::LinkedList<InternalPacket*>* theList = orderingList[ i ];

			if ( theList )
			{
				while ( theList->Size() )
				{
					internalPacket = orderingList[ i ]->Pop();
					rakFree_Ex(internalPacket->data, __FILE__, __LINE__ );
					ReleaseToInternalPacketPool( internalPacket );
				}

				RakNet::OP_DELETE(theList, __FILE__, __LINE__);
			}
		}
	}

	orderingList.Clear();

	//resendList.ForEachData(DeleteInternalPacket);
//	resendTree.Clear();
	memset(resendBuffer, 0, sizeof(resendBuffer));
	numPacketsOnResendBuffer=0;

	if (resendLinkedListHead)
	{
		InternalPacket *prev;
		InternalPacket *iter = resendLinkedListHead;
		while (1)
		{
			rakFree_Ex(iter->data, __FILE__, __LINE__ );
			prev=iter;
			iter=iter->next;
			if (iter==resendLinkedListHead)
			{
				ReleaseToInternalPacketPool(prev);
				break;
			}
			ReleaseToInternalPacketPool(prev);
		}
		resendLinkedListHead=0;
	}
	unacknowledgedBytes=0;

	//	acknowlegements.Clear();
	for ( i = 0; i < NUMBER_OF_PRIORITIES; i++ )
	{
		j = 0;
		for ( ; j < sendPacketSet[ i ].Size(); j++ )
		{
			rakFree_Ex(( sendPacketSet[ i ] ) [ j ]->data, __FILE__, __LINE__ );
			ReleaseToInternalPacketPool( ( sendPacketSet[ i ] ) [ j ] );
		}

		sendPacketSet[ i ].ClearAndForceAllocation( 32 ); // Preallocate the send lists so we don't do a bunch of reallocations unnecessarily
	}

#ifndef _RELEASE
	for (unsigned i = 0; i < delayList.Size(); i++ )
		RakNet::OP_DELETE(delayList[ i ], __FILE__, __LINE__);
	delayList.Clear();
#endif

	packetsToSendThisUpdate.Clear();
	packetsToSendThisUpdate.Preallocate(512);
	packetsToDeallocThisUpdate.Clear();
	packetsToDeallocThisUpdate.Preallocate(512);
	packetsToSendThisUpdateDatagramBoundaries.Clear();
	packetsToSendThisUpdateDatagramBoundaries.Preallocate(128);
	datagramSizesInBytes.Clear();
	datagramSizesInBytes.Preallocate(128);

	internalPacketPool.Clear();

	/*
	DataStructures::Page<DatagramSequenceNumberType, DatagramMessageIDList*, RESEND_TREE_ORDER> *cur = datagramMessageIDTree.GetListHead();
	while (cur)
	{
		int treeIndex;
		for (treeIndex=0; treeIndex < cur->size; treeIndex++)
			ReleaseToDatagramMessageIDPool(cur->data[treeIndex]);
		cur=cur->next;
	}
	datagramMessageIDTree.Clear();
	datagramMessageIDPool.Clear();
	*/

	for (int i=0; i < DATAGRAM_MESSAGE_ID_ARRAY_LENGTH; i++)
		datagramMessageIDArray[i].messages.Clear();

	acknowlegements.Clear();
	NAKs.Clear();
}

//-------------------------------------------------------------------------------------------------------
// Packets are read directly from the socket layer and skip the reliability
//layer  because unconnected players do not use the reliability layer
// This function takes packet data after a player has been confirmed as
//connected.  The game should not use that data directly
// because some data is used internally, such as packet acknowledgment and
//split packets
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::HandleSocketReceiveFromConnectedPlayer(
	const char *buffer, unsigned int length, SystemAddress systemAddress, DataStructures::List<PluginInterface2*> &messageHandlerList, int MTUSize,
	SOCKET s, RakNetRandom *rnr, unsigned short remotePortRakNetWasStartedOn_PS3, RakNetTimeUS timeRead)
{
#ifdef _DEBUG
	RakAssert( !( buffer == 0 ) );
#endif

	(void) MTUSize;

	if ( length <= 2 || buffer == 0 )   // Length of 1 is a connection request resend that we just ignore
		return true;

	static int timesRun=0;
	timesRun++;


	bytesReceivedThisHistogram+=length;

//	RakNetTimeUS time;
	bool indexFound;
	int count, size;
	DatagramSequenceNumberType holeCount;
	unsigned i,j;
//	bool hasAcks=false;

	UpdateThreadedMemory();

	// decode this whole chunk if the decoder is defined.
	if ( encryptor.IsKeySet() )
	{
		if ( encryptor.Decrypt( ( unsigned char* ) buffer, length, ( unsigned char* ) buffer, &length ) == false )
		{
			statistics.bitsWithBadCRCReceived += length * 8;
			statistics.packetsWithBadCRCReceived++;
			return false;
		}
	}

	statistics.bitsReceived += length * 8;
	statistics.packetsReceived++;

	RakNet::BitStream socketData( (unsigned char*) buffer, length, false ); // Convert the incoming data to a bitstream for easy parsing
//	time = RakNet::GetTimeNS();

	// Set to the current time if it is not zero, and we get incoming data
	if (timeResendQueueNonEmpty!=0)
		timeResendQueueNonEmpty=timeRead;

	DatagramHeaderFormat dhf;
	dhf.Deserialize(&socketData);
	if (dhf.isValid==false)
		return true;
	if (dhf.isACK)
	{
		DatagramSequenceNumberType datagramNumber;
		// datagramNumber=dhf.datagramNumber;
		RakNetTimeMS timeMSLow=(RakNetTimeMS) timeRead&0xFFFFFFFF;
		RakNetTimeUS rtt = timeMSLow-dhf.sourceSystemTime;
		if (rtt > 10000000)
		{
			// Sanity check. This could happen due to type overflow, especially since I only send the low 4 bytes to reduce bandwidth
			rtt=(RakNetTimeUS) congestionManager.GetRTT();
		}
	//	RakAssert(rtt < 500000);
	//	printf("%i ", (RakNetTimeMS)(rtt/1000));
		ackPing=rtt;
#ifdef _DEBUG
		if (dhf.hasBAndAS==false)
		{
			dhf.B=0;
			dhf.AS=0;
		}
#endif
		congestionManager.OnAck(timeRead, rtt, dhf.hasBAndAS, dhf.B, dhf.AS, totalUserDataBytesAcked );
		

		DataStructures::RangeList<DatagramSequenceNumberType> incomingAcks;
		if (incomingAcks.Deserialize(&socketData)==false)
			return false;
		for (i=0; i<incomingAcks.ranges.Size();i++)
		{
			if (incomingAcks.ranges[i].minIndex>incomingAcks.ranges[i].maxIndex)
			{
				RakAssert(incomingAcks.ranges[i].minIndex<=incomingAcks.ranges[i].maxIndex);
				return false;
			}
			for (datagramNumber=incomingAcks.ranges[i].minIndex; datagramNumber >= incomingAcks.ranges[i].minIndex && datagramNumber <= incomingAcks.ranges[i].maxIndex; datagramNumber++)
			{
				const DatagramMessageIDList &datagramMessageIDList = datagramMessageIDArray[datagramNumber & (uint32_t) DATAGRAM_MESSAGE_ID_ARRAY_MASK];
//				if (datagramMessageIDTree.Delete(messageNumber, datagramMessageIDList))
//				{
					for (j=0; j < datagramMessageIDList.messages.Size(); j++)
					{
						RemovePacketFromResendListAndDeleteOlderReliableSequenced( datagramMessageIDList.messages[j], timeRead, messageHandlerList, systemAddress );
					}
//					ReleaseToDatagramMessageIDPool(datagramMessageIDList);
					//				}
			}
		}
	}
	else if (dhf.isNAK)
	{
		DatagramSequenceNumberType messageNumber;
		DataStructures::RangeList<DatagramSequenceNumberType> incomingNAKs;
		if (incomingNAKs.Deserialize(&socketData)==false)
			return false;
		for (i=0; i<incomingNAKs.ranges.Size();i++)
		{
			if (incomingNAKs.ranges[i].minIndex>incomingNAKs.ranges[i].maxIndex)
			{
				RakAssert(incomingNAKs.ranges[i].minIndex<=incomingNAKs.ranges[i].maxIndex);
				return false;
			}
			// Sanity check
			RakAssert(incomingNAKs.ranges[i].maxIndex.val-incomingNAKs.ranges[i].minIndex.val<1000);
			for (messageNumber=incomingNAKs.ranges[i].minIndex; messageNumber >= incomingNAKs.ranges[i].minIndex && messageNumber <= incomingNAKs.ranges[i].maxIndex; messageNumber++)
			{
		//		DatagramMessageIDList *datagramMessageIDList;
				congestionManager.OnNAK(timeRead, messageNumber);
				const DatagramMessageIDList &datagramMessageIDList = datagramMessageIDArray[messageNumber & (uint32_t) DATAGRAM_MESSAGE_ID_ARRAY_MASK];
//				if (datagramMessageIDTree.Get(reliableMessageNumber, datagramMessageIDList))
//				{
					unsigned int i;
					InternalPacket *internalPacket;
					for (i=0; i < datagramMessageIDList.messages.Size(); i++)
					{
						// Update timers so resends occur immediately
						//if (resendTree.Get(datagramMessageIDList.messages[i], internalPacket))
						internalPacket = resendBuffer[datagramMessageIDList.messages[i] & (uint32_t) RESEND_BUFFER_ARRAY_MASK];
						if (internalPacket)
						{
							if (internalPacket->nextActionTime!=0)
							{
								internalPacket->nextActionTime=timeRead;

								// Removeme
//								printf("NAK:%i ", internalPacket->reliableMessageNumber);

								// This is not needed, they are already in order to resend. For a group of NAK, this actually sends them in reverse order
								// Put at head of list
								// MoveToListHead(internalPacket);
							}
						}				
					}
//				}
			}
		}
	}
	else
	{
		uint32_t skippedMessageCount;
		congestionManager.OnGotPacket(dhf.datagramNumber, dhf.isContinuousSend, timeRead, length, &skippedMessageCount);
		if (dhf.isPacketPair)
			congestionManager.OnGotPacketPair(dhf.datagramNumber, length, timeRead);

		DatagramHeaderFormat dhfNAK;
		dhfNAK.isNAK=true;
		uint32_t skippedMessageOffset;
		for (skippedMessageOffset=skippedMessageCount; skippedMessageOffset > 0; skippedMessageOffset--)
		{
			NAKs.Insert(dhf.datagramNumber-skippedMessageOffset);

			/*
			dhfNAK.datagramNumber=dhf.datagramNumber-skippedMessageOffset;
#if defined(USE_THREADED_SEND)
			SendToThread::SendToThreadBlock *sttb = AllocateSendToThreadBlock(s,systemAddress.binaryAddress,systemAddress.port,remotePortRakNetWasStartedOn_PS3);
			dhfNAK.Serialize(sttb);
			SendThreaded(sttb);
#else
			RakNet::BitStream skippedMessageBs;
			dhfNAK.Serialize(&skippedMessageBs);
			SendBitStream( s, systemAddress, &updateBitStream, rnr, remotePortRakNetWasStartedOn_PS3 );
#endif
			*/
		}

		// Ack dhf.datagramNumber
		// Ack even unreliable messages for congestion control, just don't resend them on no ack
		SendAcknowledgementPacket( dhf.datagramNumber, dhf.sourceSystemTime);

		InternalPacket* internalPacket = CreateInternalPacketFromBitStream( &socketData, timeRead );
		if (internalPacket==0)
			return true;


		while ( internalPacket )
		{
			for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
				messageHandlerList[messageHandlerIndex]->OnInternalPacket(internalPacket, receivePacketCount, systemAddress, (RakNetTime)(timeRead/(RakNetTimeUS)1000), false);

			{

				// resetReceivedPackets is set from a non-threadsafe function.
				// We do the actual reset in this function so the data is not modified by multiple threads
				if (resetReceivedPackets)
				{
					hasReceivedPacketQueue.ClearAndForceAllocation(DEFAULT_HAS_RECEIVED_PACKET_QUEUE_SIZE);
					receivedPacketsBaseIndex=0;
					resetReceivedPackets=false;
				}

				// 8/12/09 was previously not checking if the message was reliable. However, on packetloss this would mean you'd eventually exceed the
				// hole count because unreliable messages were never resent, and you'd stop getting messages
				if (internalPacket->reliability == RELIABLE || internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_ORDERED )
				{
					// If the following conditional is true then this either a duplicate packet
					// or an older out of order packet
					// The subtraction unsigned overflow is intentional
					holeCount = (DatagramSequenceNumberType)(internalPacket->reliableMessageNumber-receivedPacketsBaseIndex);
					const DatagramSequenceNumberType typeRange = (DatagramSequenceNumberType)(const uint32_t)-1;

					if (holeCount==(DatagramSequenceNumberType) 0)
					{
						// Got what we were expecting
						if (hasReceivedPacketQueue.Size())
							hasReceivedPacketQueue.Pop();
						++receivedPacketsBaseIndex;
					}
					else if (holeCount > typeRange/(DatagramSequenceNumberType) 2)
					{
						// Underflow - got a packet we have already counted past
						statistics.duplicateMessagesReceived++;

		//				static int dupeCount2=0;
			//			dupeCount2++;
					//	printf("Dupe2 %i\n", dupeCount2);

						// Duplicate packet
						rakFree_Ex(internalPacket->data, __FILE__, __LINE__ );
						ReleaseToInternalPacketPool( internalPacket );
						goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
					}
					else if ((unsigned int) holeCount<hasReceivedPacketQueue.Size())
					{

						// Got a higher count out of order packet that was missing in the sequence or we already got
						if (hasReceivedPacketQueue[holeCount]!=false) // non-zero means this is a hole
						{
							// Fill in the hole
							hasReceivedPacketQueue[holeCount]=false; // We got the packet at holeCount
						}
						else
						{
						//	static int dupeCount=0;
						//	dupeCount++;
						//	printf("Dupe %i\n", dupeCount);

							// Not a hole - just a duplicate packet
							statistics.duplicateMessagesReceived++;

							// Duplicate packet
							rakFree_Ex(internalPacket->data, __FILE__, __LINE__ );
							ReleaseToInternalPacketPool( internalPacket );
							goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
						}
					}
					else // holeCount>=receivedPackets.Size()
					{
						if (holeCount > (DatagramSequenceNumberType) 1000000)
						{
							RakAssert("Hole count too high. See ReliabilityLayer.h" && 0);

							// Would crash due to out of memory!
							rakFree_Ex(internalPacket->data, __FILE__, __LINE__ );
							ReleaseToInternalPacketPool( internalPacket );
							goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
						}


						// Fix - sending on a higher priority gives us a very very high received packets base index if we formerly had pre-split a lot of messages and
						// used that as the message number.  Because of this, a lot of time is spent in this linear loop and the timeout time expires because not
						// all of the message is sent in time.
						// Fixed by late assigning message IDs on the sender

						// Add 0 times to the queue until (reliableMessageNumber - baseIndex) < queue size.
						while ((unsigned int)(holeCount) > hasReceivedPacketQueue.Size())
							hasReceivedPacketQueue.Push(true); // time+(RakNetTimeUS)60 * (RakNetTimeUS)1000 * (RakNetTimeUS)1000); // Didn't get this packet - set the time to give up waiting
						hasReceivedPacketQueue.Push(false); // Got the packet
#ifdef _DEBUG
						// If this assert hits then DatagramSequenceNumberType has overflowed
						RakAssert(hasReceivedPacketQueue.Size() < (unsigned int)((DatagramSequenceNumberType)(const uint32_t)(-1)));
#endif
					}

					while ( hasReceivedPacketQueue.Size()>0 && hasReceivedPacketQueue.Peek()==false )
					{
						hasReceivedPacketQueue.Pop();
						++receivedPacketsBaseIndex;
					}
				}

				statistics.messagesReceived++;

				// If the allocated buffer is > DEFAULT_HAS_RECEIVED_PACKET_QUEUE_SIZE and it is 3x greater than the number of elements actually being used
				if (hasReceivedPacketQueue.AllocationSize() > (unsigned int) DEFAULT_HAS_RECEIVED_PACKET_QUEUE_SIZE && hasReceivedPacketQueue.AllocationSize() > hasReceivedPacketQueue.Size() * 3)
					hasReceivedPacketQueue.Compress();

				if ( internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == UNRELIABLE_SEQUENCED )
				{
#ifdef _DEBUG
					RakAssert( internalPacket->orderingChannel < NUMBER_OF_ORDERED_STREAMS );
#endif

					if ( internalPacket->orderingChannel >= NUMBER_OF_ORDERED_STREAMS )
					{

						rakFree_Ex(internalPacket->data, __FILE__, __LINE__ );
						ReleaseToInternalPacketPool( internalPacket );
						goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
					}

					if ( IsOlderOrderedPacket( internalPacket->orderingIndex, waitingForSequencedPacketReadIndex[ internalPacket->orderingChannel ] ) == false )
					{
						statistics.sequencedMessagesInOrder++;


						// Is this a split packet?
						if ( internalPacket->splitPacketCount > 0 )
						{
							// Generate the split
							// Verify some parameters to make sure we don't get junk data


							// Check for a rebuilt packet
							InsertIntoSplitPacketList( internalPacket, timeRead );

							// Sequenced
							internalPacket = BuildPacketFromSplitPacketList( internalPacket->splitPacketId, timeRead,
								s, systemAddress, rnr, remotePortRakNetWasStartedOn_PS3);

							if ( internalPacket )
							{
								// Update our index to the newest packet
								waitingForSequencedPacketReadIndex[ internalPacket->orderingChannel ] = internalPacket->orderingIndex + (OrderingIndexType)1;

								// If there is a rebuilt packet, add it to the output queue
								outputQueue.Push( internalPacket );
								internalPacket = 0;
							}

							// else don't have all the parts yet
						}

						else
						{
							// Update our index to the newest packet
							waitingForSequencedPacketReadIndex[ internalPacket->orderingChannel ] = internalPacket->orderingIndex + (OrderingIndexType)1;

							// Not a split packet. Add the packet to the output queue
							outputQueue.Push( internalPacket );
							internalPacket = 0;
						}
					}
					else
					{
						statistics.sequencedMessagesOutOfOrder++;

						// Older sequenced packet. Discard it
						rakFree_Ex(internalPacket->data, __FILE__, __LINE__ );
						ReleaseToInternalPacketPool( internalPacket );
					}

					goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
				}

				// Is this an unsequenced split packet?
				if ( internalPacket->splitPacketCount > 0 )
				{
					// An unsequenced split packet.  May be ordered though.

					// Check for a rebuilt packet
					if ( internalPacket->reliability != RELIABLE_ORDERED )
						internalPacket->orderingChannel = 255; // Use 255 to designate not sequenced and not ordered

					InsertIntoSplitPacketList( internalPacket, timeRead );

					internalPacket = BuildPacketFromSplitPacketList( internalPacket->splitPacketId, timeRead,
						s, systemAddress, rnr, remotePortRakNetWasStartedOn_PS3);

					if ( internalPacket == 0 )
					{

						// Don't have all the parts yet
						goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
					}

					// else continue down to handle RELIABLE_ORDERED

				}

				if ( internalPacket->reliability == RELIABLE_ORDERED )
				{
#ifdef _DEBUG
					RakAssert( internalPacket->orderingChannel < NUMBER_OF_ORDERED_STREAMS );
#endif

					if ( internalPacket->orderingChannel >= NUMBER_OF_ORDERED_STREAMS )
					{
						// Invalid packet
						rakFree_Ex(internalPacket->data, __FILE__, __LINE__ );
						ReleaseToInternalPacketPool( internalPacket );
						goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
					}

					if ( waitingForOrderedPacketReadIndex[ internalPacket->orderingChannel ] == internalPacket->orderingIndex )
					{
						// Get the list to hold ordered packets for this stream
						DataStructures::LinkedList<InternalPacket*> *orderingListAtOrderingStream;
						unsigned char orderingChannelCopy = internalPacket->orderingChannel;

						statistics.orderedMessagesInOrder++;

						// Push the packet for the user to read
						outputQueue.Push( internalPacket );
						internalPacket = 0; // Don't reference this any longer since other threads access it

						// Wait for the next ordered packet in sequence
						waitingForOrderedPacketReadIndex[ orderingChannelCopy ] ++; // This wraps

						orderingListAtOrderingStream = GetOrderingListAtOrderingStream( orderingChannelCopy );

						if ( orderingListAtOrderingStream != 0)
						{
							while ( orderingListAtOrderingStream->Size() > 0 )
							{
								// Cycle through the list until nothing is found
								orderingListAtOrderingStream->Beginning();
								indexFound=false;
								size=orderingListAtOrderingStream->Size();
								count=0;

								while (count++ < size)
								{
									if ( orderingListAtOrderingStream->Peek()->orderingIndex == waitingForOrderedPacketReadIndex[ orderingChannelCopy ] )
									{
										outputQueue.Push( orderingListAtOrderingStream->Pop() );
										waitingForOrderedPacketReadIndex[ orderingChannelCopy ]++;
										indexFound=true;
									}
									else
										(*orderingListAtOrderingStream)++;
								}

								if (indexFound==false)
									break;
							}
						}

						internalPacket = 0;
					}
					else
					{
						//	RakAssert(waitingForOrderedPacketReadIndex[ internalPacket->orderingChannel ] < internalPacket->orderingIndex);
						statistics.orderedMessagesOutOfOrder++;

						// This is a newer ordered packet than we are waiting for. Store it for future use
						AddToOrderingList( internalPacket );
					}

					goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
				}

				// Nothing special about this packet.  Add it to the output queue
				outputQueue.Push( internalPacket );

				internalPacket = 0;
			}

			// Used for a goto to jump to the next packet immediately

CONTINUE_SOCKET_DATA_PARSE_LOOP:
			// Parse the bitstream to create an internal packet
			internalPacket = CreateInternalPacketFromBitStream( &socketData, timeRead );
		}

	}


	receivePacketCount++;

	return true;
}

//-------------------------------------------------------------------------------------------------------
// This gets an end-user packet already parsed out. Returns number of BITS put into the buffer
//-------------------------------------------------------------------------------------------------------
BitSize_t ReliabilityLayer::Receive( unsigned char **data )
{
	// Wait until the clear occurs
	if (freeThreadedMemoryOnNextUpdate)
		return 0;

	InternalPacket * internalPacket;

	if ( outputQueue.Size() > 0 )
	{
		//  #ifdef _DEBUG
		//  RakAssert(bitStream->GetNumberOfBitsUsed()==0);
		//  #endif
		internalPacket = outputQueue.Pop();

		BitSize_t bitLength;
		*data = internalPacket->data;
		bitLength = internalPacket->dataBitLength;
		ReleaseToInternalPacketPool( internalPacket );
		return bitLength;
	}

	else
	{
		return 0;
	}

}

//-------------------------------------------------------------------------------------------------------
// Puts data on the send queue
// bitStream contains the data to send
// priority is what priority to send the data at
// reliability is what reliability to use
// ordering channel is from 0 to 255 and specifies what stream to use
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::Send( char *data, BitSize_t numberOfBitsToSend, PacketPriority priority, PacketReliability reliability, unsigned char orderingChannel, bool makeDataCopy, int MTUSize, RakNetTimeUS currentTime )
{
#ifdef _DEBUG
	RakAssert( !( reliability > RELIABLE_SEQUENCED || reliability < 0 ) );
	RakAssert( !( priority > NUMBER_OF_PRIORITIES || priority < 0 ) );
	RakAssert( !( orderingChannel >= NUMBER_OF_ORDERED_STREAMS ) );
	RakAssert( numberOfBitsToSend > 0 );
#endif

	(void) MTUSize;

	//	int a = BITS_TO_BYTES(numberOfBitsToSend);

	// Fix any bad parameters
	if ( reliability > RELIABLE_SEQUENCED || reliability < 0 )
		reliability = RELIABLE;

	if ( priority > NUMBER_OF_PRIORITIES || priority < 0 )
		priority = HIGH_PRIORITY;

	if ( orderingChannel >= NUMBER_OF_ORDERED_STREAMS )
		orderingChannel = 0;

	unsigned int numberOfBytesToSend=(unsigned int) BITS_TO_BYTES(numberOfBitsToSend);
	if ( numberOfBitsToSend == 0 )
	{
		return false;
	}
	InternalPacket * internalPacket = AllocateFromInternalPacketPool();
	if (internalPacket==0)
	{
		notifyOutOfMemory(__FILE__, __LINE__);
		return false; // Out of memory
	}

	//InternalPacket * internalPacket = sendPacketSet[priority].WriteLock();
#ifdef _DEBUG
	// Remove accessing undefined memory warning
	memset( internalPacket, 255, sizeof( InternalPacket ) );
#endif

	internalPacket->creationTime = currentTime;

	if ( makeDataCopy )
	{
		internalPacket->data = (unsigned char*) rakMalloc_Ex( numberOfBytesToSend, __FILE__, __LINE__ );
		memcpy( internalPacket->data, data, numberOfBytesToSend );
	}
	else
	{
		// Allocated the data elsewhere, delete it in here
		internalPacket->data = ( unsigned char* ) data;
	}

	internalPacket->dataBitLength = numberOfBitsToSend;
	internalPacket->nextActionTime = 0;

	internalPacket->reliableMessageNumber = (MessageNumberType) (const uint32_t)-1;
	internalPacket->messageNumberAssigned=false;

	internalPacket->messageInternalOrder = internalOrderIndex++;

	internalPacket->priority = priority;
	internalPacket->reliability = reliability;
	internalPacket->splitPacketCount = 0;

	// Calculate if I need to split the packet
//	int headerLength = BITS_TO_BYTES( GetBitStreamHeaderLength( internalPacket, true ) );

	unsigned int maxDataSize = congestionManager.GetMaxDatagramPayload();

	bool splitPacket = numberOfBytesToSend > maxDataSize;

	// If a split packet, we might have to upgrade the reliability
	if ( splitPacket )
		statistics.numberOfSplitMessages++;
	else
		statistics.numberOfUnsplitMessages++;

	//	++sendMessageNumberIndex;

	if ( internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == UNRELIABLE_SEQUENCED )
	{
		// Assign the sequence stream and index
		internalPacket->orderingChannel = orderingChannel;
		internalPacket->orderingIndex = waitingForSequencedPacketWriteIndex[ orderingChannel ] ++;

		// This packet supersedes all other sequenced packets on the same ordering channel
		// Delete all packets in all send lists that are sequenced and on the same ordering channel
		// UPDATE:
		// Disabled.  We don't have enough info to consistently do this.  Sometimes newer data does supercede
		// older data such as with constantly declining health, but not in all cases.
		// For example, with sequenced unreliable sound packets just because you send a newer one doesn't mean you
		// don't need the older ones because the odds are they will still arrive in order
		/*
		for (int i=0; i < NUMBER_OF_PRIORITIES; i++)
		{
		DeleteSequencedPacketsInList(orderingChannel, sendQueue[i]);
		}
		*/
	}

	else
		if ( internalPacket->reliability == RELIABLE_ORDERED )
		{
			// Assign the ordering channel and index
			internalPacket->orderingChannel = orderingChannel;
			internalPacket->orderingIndex = waitingForOrderedPacketWriteIndex[ orderingChannel ] ++;
		}

		if ( splitPacket )   // If it uses a secure header it will be generated here
		{
			// Must split the packet.  This will also generate the SHA1 if it is required. It also adds it to the send list.
			//InternalPacket packetCopy;
			//memcpy(&packetCopy, internalPacket, sizeof(InternalPacket));
			//sendPacketSet[priority].CancelWriteLock(internalPacket);
			//SplitPacket( &packetCopy, MTUSize );
			SplitPacket( internalPacket );
			//RakNet::OP_DELETE_ARRAY(packetCopy.data, __FILE__, __LINE__);
			return true;
		}

		sendPacketSet[ internalPacket->priority ].Push( internalPacket );

		//	sendPacketSet[priority].WriteUnlock();
		return true;
}
//-------------------------------------------------------------------------------------------------------
// Run this once per game cycle.  Handles internal lists and actually does the send
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::Update( SOCKET s, SystemAddress systemAddress, int MTUSize, RakNetTimeUS time,
							  unsigned bitsPerSecondLimit,
							  DataStructures::List<PluginInterface2*> &messageHandlerList,
							  RakNetRandom *rnr, unsigned short remotePortRakNetWasStartedOn_PS3)

{
	(void) bitsPerSecondLimit;
	(void) MTUSize;

	int i;

	// This line is necessary because the timer isn't accurate
	if (time <= lastUpdateTime)
	{
		// Always set the last time in case of overflow
		lastUpdateTime=time;
		return;
	}

	if (time>resetHistogramTime)
	{
		resetHistogramTime=time+100000; // update histogram at 100 millisecond intervals
		bytesReceivedLastHistogram=bytesReceivedThisHistogram;
		bytesSentLastHistogram=bytesSentThisHistogram;
		bytesReceivedThisHistogram=0;
		bytesSentThisHistogram=0;
	}


	RakNetTimeUS timeSinceLastTick = time - lastUpdateTime;
	lastUpdateTime=time;


	// Due to thread vagarities and the way I store the time to avoid slow calls to RakNet::GetTime
	// time may be less than lastAck
	if ( numPacketsOnResendBuffer!=0 && AckTimeout(time) )
	{
		// SHOW - dead connection
		// We've waited a very long time for a reliable packet to get an ack and it never has
		deadConnection = true;
		return;
	}

	BitSize_t maxDatagramPayload = BYTES_TO_BITS(congestionManager.GetMaxDatagramPayload());
	if (congestionManager.ShouldSendACKs(time,timeSinceLastTick))
	{
		SendACKs(s, systemAddress, time, rnr, remotePortRakNetWasStartedOn_PS3);
	}
	
	if (NAKs.Size()>0)
	{
		updateBitStream.Reset();
		DatagramHeaderFormat dhfNAK;
		dhfNAK.isNAK=true;
		dhfNAK.isACK=false;
		dhfNAK.isPacketPair=false;
		dhfNAK.Serialize(&updateBitStream);
		NAKs.Serialize(&updateBitStream, maxDatagramPayload, true);
		SendBitStream( s, systemAddress, &updateBitStream, rnr, remotePortRakNetWasStartedOn_PS3 );
	}

	DatagramHeaderFormat dhf;
	dhf.isContinuousSend=bandwidthExceededStatistic;
	bandwidthExceededStatistic=sendPacketSet[0].IsEmpty()==false ||
		sendPacketSet[1].IsEmpty()==false ||
		sendPacketSet[2].IsEmpty()==false ||
		sendPacketSet[3].IsEmpty()==false;

	const bool hasDataToSendOrResend = IsResendQueueEmpty()==false || bandwidthExceededStatistic;
	RakAssert(NUMBER_OF_PRIORITIES==4);
	congestionManager.Update(time, hasDataToSendOrResend);

	if (hasDataToSendOrResend==true)
	{
		InternalPacket *internalPacket;
//		bool forceSend=false;
		bool isReliable;
		bool pushedAnything;
		BitSize_t nextPacketBitLength;
		dhf.isACK=false;
		dhf.isNAK=false;
		dhf.hasBAndAS=false;
		RakAssert(BITS_TO_BYTES(maxDatagramPayload)<MAXIMUM_MTU_SIZE-UDP_HEADER_SIZE);
		ResetPacketsAndDatagrams();

		uint32_t retransmissionBandwidth = congestionManager.GetRetransmissionBandwidth(time, timeSinceLastTick);
		if (retransmissionBandwidth>=0)
		{
			allDatagramSizesSoFar=0;

			// Keep filling datagrams until we exceed retransmission bandwidth
			while (BITS_TO_BYTES(allDatagramSizesSoFar)<retransmissionBandwidth)
			{
				pushedAnything=false;

				// Fill one datagram, then break
				while ( IsResendQueueEmpty()==false )
				{
					internalPacket = resendLinkedListHead;
					RakAssert(internalPacket->messageNumberAssigned==true);

					if ( internalPacket->nextActionTime < time )
					{
						nextPacketBitLength = internalPacket->headerLength + internalPacket->dataBitLength;
						if ( datagramSizeSoFar + nextPacketBitLength > maxDatagramPayload )
						{
							// Gathers all PushPackets()
							PushDatagram();
							break;
						}

						PopListHead(false);

						CC_DEBUG_PRINTF_2("Rs %i ", internalPacket->reliableMessageNumber.val);

						// Write to the output bitstream
						statistics.messageResends++;
						statistics.messageDataBitsResent += internalPacket->dataBitLength;
						PushPacket(time,internalPacket,true); // Affects GetNewTransmissionBandwidth()
						statistics.messagesTotalBitsResent += internalPacket->dataBitLength + internalPacket->headerLength;
					//	statistics.packetsContainingOnlyAcknowlegementsAndResends++;
						internalPacket->timesSent++;
						internalPacket->nextActionTime = congestionManager.GetRTOForRetransmission()+time;
						if (internalPacket->nextActionTime-time > 10000000)
						{
//							int a=5;
							RakAssert(0);
						}

						pushedAnything=true;

						for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
							messageHandlerList[messageHandlerIndex]->OnInternalPacket(internalPacket, sendReliableMessageNumberIndex, systemAddress, (RakNetTime)(time/(RakNetTimeUS)1000), true);

						// Put the packet back into the resend list at the correct spot
						// Don't make a copy since I'm reinserting an allocated struct
						InsertPacketIntoResendList( internalPacket, time, false, false, false );
 
						// Removeme
//						printf("Resend:%i ", internalPacket->reliableMessageNumber);
					}
					else
					{
						// Filled one datagram.
						// If the 2nd and it's time to send a datagram pair, will be marked as a pair
						PushDatagram();
						break;
					}
				}

				if (pushedAnything==false)
					break;
			}
		}

		RakNetTimeUS timeSinceLastContinualSend;
		if (timeOfLastContinualSend!=0)
			timeSinceLastContinualSend=time-timeOfLastContinualSend;
		else
			timeSinceLastContinualSend=0;
		uint32_t newTransmissionBandwidth = congestionManager.GetNewTransmissionBandwidth(time, timeSinceLastTick, timeSinceLastContinualSend, unacknowledgedBytes);
		//RakAssert(bufferOverflow==false); // If asserts, buffer is too small. We are using a slot that wasn't acked yet
		if (newTransmissionBandwidth>=BITS_TO_BYTES(maxDatagramPayload))
		{
		//	printf("S+ ");
			allDatagramSizesSoFar=0;

			// Keep filling datagrams until we exceed retransmission bandwidth
			while (
				ResendBufferOverflow()==false &&
				(BITS_TO_BYTES(allDatagramSizesSoFar)<newTransmissionBandwidth ||
				// This condition means if we want to send a datagram pair, and only have one datagram buffered, exceed bandwidth to add another
				(countdownToNextPacketPair==0 &&
				datagramsToSendThisUpdateIsPair.Size()==1))
				)
			{
				// Fill with packets until MTU is reached
				for ( i = 0; i < NUMBER_OF_PRIORITIES; i++ )
				{
					pushedAnything=false;
					while ( sendPacketSet[ i ].Size() )
					{
						internalPacket = sendPacketSet[ i ].Peek();

						if (unreliableTimeout!=0 &&
							(internalPacket->reliability==UNRELIABLE || internalPacket->reliability==UNRELIABLE_SEQUENCED) &&
							time > internalPacket->creationTime+(RakNetTimeUS)unreliableTimeout)
						{
							sendPacketSet[ i ].Pop();
							// Unreliable packets are deleted
							rakFree_Ex(internalPacket->data, __FILE__, __LINE__ );
							ReleaseToInternalPacketPool( internalPacket );
							continue;
						}

						internalPacket->headerLength=GetBitStreamHeaderLength(internalPacket,false);
						nextPacketBitLength = internalPacket->headerLength + internalPacket->dataBitLength;
						if ( datagramSizeSoFar + nextPacketBitLength > maxDatagramPayload )
						{
							// Hit MTU. May still push packets if smaller ones exist at a lower priority
							break;
						}

						if ( internalPacket->reliability == RELIABLE || internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_ORDERED )
							isReliable = true;
						else
							isReliable = false;

						// Write to the output bitstream
						statistics.messagesSent[ i ] ++;
						statistics.messageDataBitsSent[ i ] += internalPacket->dataBitLength;

						sendPacketSet[ i ].Pop();
						if (isReliable)
						{
							RakAssert(internalPacket->reliableMessageNumber==(MessageNumberType)(const uint32_t)-1);
							RakAssert(internalPacket->messageNumberAssigned==false);
							internalPacket->messageNumberAssigned=true;
							internalPacket->reliableMessageNumber=sendReliableMessageNumberIndex;
							// Set to the current time when the resend queue is no longer empty
							if (numPacketsOnResendBuffer==0)
							{
								RakAssert(resendLinkedListHead==0);
								timeResendQueueNonEmpty=time;
							}


							internalPacket->nextActionTime = congestionManager.GetRTOForRetransmission()+time;
							if (internalPacket->nextActionTime-time > 10000000)
							{
//								int a=5;
								RakAssert(time-internalPacket->nextActionTime < 10000000);
							}
							//resendTree.Insert( internalPacket->reliableMessageNumber, internalPacket);
							if (resendBuffer[internalPacket->reliableMessageNumber & (uint32_t) RESEND_BUFFER_ARRAY_MASK]!=0)
							{
//								bool overflow = ResendBufferOverflow();
								RakAssert(0);
							}
							resendBuffer[internalPacket->reliableMessageNumber & (uint32_t) RESEND_BUFFER_ARRAY_MASK] = internalPacket;
							numPacketsOnResendBuffer++;

							//		printf("pre:%i ", unacknowledgedBytes);

							InsertPacketIntoResendList( internalPacket, time, false, true, true);


							//		printf("post:%i ", unacknowledgedBytes);
							sendReliableMessageNumberIndex++;
						}
						internalPacket->timesSent=1;
						statistics.messageTotalBitsSent[ i ] += nextPacketBitLength;
						PushPacket(time,internalPacket, isReliable);
						for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
							messageHandlerList[messageHandlerIndex]->OnInternalPacket(internalPacket, sendReliableMessageNumberIndex, systemAddress, (RakNetTime)(time/(RakNetTimeUS)1000), true);
						pushedAnything=true;

						// No packets pushed?
						if (pushedAnything==false)
							break;

						if (ResendBufferOverflow())
							break;
					}
					if (ResendBufferOverflow())
						break;
				}

				// No datagrams pushed?
				if (datagramSizeSoFar==0)
					break;

				// Filled one datagram.
				// If the 2nd and it's time to send a datagram pair, will be marked as a pair
				PushDatagram();
			}
		}
		else
		{
		//	printf("S- ");

		}



		for (unsigned int datagramIndex=0; datagramIndex < packetsToSendThisUpdateDatagramBoundaries.Size(); datagramIndex++)
		{
			if (datagramIndex>0)
				dhf.isContinuousSend=true;
			dhf.datagramNumber=congestionManager.GetNextDatagramSequenceNumber();
			dhf.isPacketPair=datagramsToSendThisUpdateIsPair[datagramIndex];

			bool isSecondOfPacketPair=dhf.isPacketPair && datagramIndex>0 &&  datagramsToSendThisUpdateIsPair[datagramIndex-1];
			unsigned int msgIndex, msgTerm;
			if (datagramIndex==0)
			{
				msgIndex=0;
				msgTerm=packetsToSendThisUpdateDatagramBoundaries[0];
			}
			else
			{
				msgIndex=packetsToSendThisUpdateDatagramBoundaries[datagramIndex-1];
				msgTerm=packetsToSendThisUpdateDatagramBoundaries[datagramIndex];
			}
//			DatagramMessageIDList *idList = AllocateFromDatagramMessageIDPool();
//			idList->messages.Clear();

			DatagramMessageIDList &datagramMessageIDList = datagramMessageIDArray[dhf.datagramNumber & (uint32_t) DATAGRAM_MESSAGE_ID_ARRAY_MASK];
			datagramMessageIDList.messages.Clear(true);

			// More accurate time to reset here
			dhf.sourceSystemTime=RakNet::GetTimeUS();
#if defined(USE_THREADED_SEND)
			SendToThread::SendToThreadBlock *sttb = AllocateSendToThreadBlock(s,systemAddress.binaryAddress,systemAddress.port,remotePortRakNetWasStartedOn_PS3);
			dhf.Serialize(sttb);
#else
			updateBitStream.Reset();
			dhf.Serialize(&updateBitStream);
#endif
			CC_DEBUG_PRINTF_2("S%i ",dhf.datagramNumber.val);

			while (msgIndex < msgTerm)
			{
				if ( packetsToSendThisUpdate[msgIndex]->reliability == RELIABLE ||
					packetsToSendThisUpdate[msgIndex]->reliability == RELIABLE_SEQUENCED ||
					packetsToSendThisUpdate[msgIndex]->reliability == RELIABLE_ORDERED )
				{
					datagramMessageIDList.messages.Push(packetsToSendThisUpdate[msgIndex]->reliableMessageNumber, __FILE__, __LINE__);
//					printf("%i ", packetsToSendThisUpdate[msgIndex]->reliableMessageNumber);

				}
//				else
//					printf("UNR ");
#if defined(USE_THREADED_SEND)
				WriteToThreadBlockFromInternalPacket( sttb, packetsToSendThisUpdate[msgIndex], time );
#else
				RakAssert(updateBitStream.GetNumberOfBytesUsed()<=MAXIMUM_MTU_SIZE-UDP_HEADER_SIZE);
				WriteToBitStreamFromInternalPacket( &updateBitStream, packetsToSendThisUpdate[msgIndex], time );
				RakAssert(updateBitStream.GetNumberOfBytesUsed()<=MAXIMUM_MTU_SIZE-UDP_HEADER_SIZE);
#endif
				msgIndex++;
			}
			// printf("\n");

			if (isSecondOfPacketPair)
			{
				// Pad to size of first datagram
#if defined(USE_THREADED_SEND)
				memset(sttb->data+sttb->dataWriteOffset,0,BITS_TO_BYTES(firstMessageOfPacketPairSize)-sttb->dataWriteOffset);
				sttb->dataWriteOffset+=BITS_TO_BYTES(firstMessageOfPacketPairSize)-sttb->dataWriteOffset;
#else
				RakAssert(updateBitStream.GetNumberOfBytesUsed()<=MAXIMUM_MTU_SIZE-UDP_HEADER_SIZE);
				updateBitStream.PadWithZeroToByteLength(datagramSizesInBytes[datagramIndex-1]);
				RakAssert(updateBitStream.GetNumberOfBytesUsed()<=MAXIMUM_MTU_SIZE-UDP_HEADER_SIZE);
#endif
			}

			// Store what message ids were sent with this datagram
		//	datagramMessageIDTree.Insert(dhf.datagramNumber,idList);

			congestionManager.OnSendBytes(time,UDP_HEADER_SIZE+DatagramHeaderFormat::GetDataHeaderByteLength());

#if defined(USE_THREADED_SEND)
			SendThreaded(sttb);
#else
			SendBitStream( s, systemAddress, &updateBitStream, rnr, remotePortRakNetWasStartedOn_PS3 );
#endif

			bandwidthExceededStatistic=sendPacketSet[0].IsEmpty()==false ||
				sendPacketSet[1].IsEmpty()==false ||
				sendPacketSet[2].IsEmpty()==false ||
				sendPacketSet[3].IsEmpty()==false;

			if (bandwidthExceededStatistic==true)
				timeOfLastContinualSend=time;
			else
				timeOfLastContinualSend=0;
		}

		ClearPacketsAndDatagrams();

		// Any data waiting to send after attempting to send, then bandwidth is exceeded
		bandwidthExceededStatistic=sendPacketSet[0].IsEmpty()==false ||
			sendPacketSet[1].IsEmpty()==false ||
			sendPacketSet[2].IsEmpty()==false ||
			sendPacketSet[3].IsEmpty()==false;
	}


	// Keep on top of deleting old unreliable split packets so they don't clog the list.
	DeleteOldUnreliableSplitPackets( time );
}

//-------------------------------------------------------------------------------------------------------
// Writes a bitstream to the socket
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SendBitStream( SOCKET s, SystemAddress systemAddress, RakNet::BitStream *bitStream, RakNetRandom *rnr, unsigned short remotePortRakNetWasStartedOn_PS3)
{
	(void) systemAddress;

	unsigned int oldLength, length;

	// sentFrames++;

#ifndef _RELEASE
	if (maxSendBPS>0)
	{
		//		double chanceToLosePacket = (double)currentBandwidth / (double)maxSendBPS;
		//	if (frandomMT() < (float)chanceToLosePacket)
		//	return;



	}
#endif
	// REMOVEME
	//	if (frandomMT() < .15f)
	//		return;

	// Encode the whole bitstream if the encoder is defined.

	if ( encryptor.IsKeySet() )
	{
		length = (unsigned int) bitStream->GetNumberOfBytesUsed();
		oldLength = length;

		encryptor.Encrypt( ( unsigned char* ) bitStream->GetData(), length, ( unsigned char* ) bitStream->GetData(), &length, rnr );
		statistics.encryptionBitsSent = BYTES_TO_BITS( length - oldLength );

		RakAssert( ( length % 16 ) == 0 );
	}
	else
	{
		length = (unsigned int) bitStream->GetNumberOfBytesUsed();
	}

	statistics.packetsSent++;
	statistics.totalBitsSent += length * 8;
	bytesSentThisHistogram += length;

	SocketLayer::Instance()->SendTo( s, ( char* ) bitStream->GetData(), length, systemAddress.binaryAddress, systemAddress.port, remotePortRakNetWasStartedOn_PS3 );


}

//-------------------------------------------------------------------------------------------------------
// Are we waiting for any data to be sent out or be processed by the player?
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsOutgoingDataWaiting(void)
{
	unsigned i;
	for ( i = 0; i < NUMBER_OF_PRIORITIES; i++ )
	{
		if (sendPacketSet[ i ].Size() > 0)
			return true;
	}

	return 
		//acknowlegements.Size() > 0 ||
		//resendTree.IsEmpty()==false;// || outputQueue.Size() > 0 || orderingList.Size() > 0 || splitPacketChannelList.Size() > 0;
		numPacketsOnResendBuffer!=0;
}
bool ReliabilityLayer::IsReliableOutgoingDataWaiting(void)
{
	unsigned i,j;
	for ( i = 0; i < NUMBER_OF_PRIORITIES; i++ )
	{
		for (j=0; j < sendPacketSet[ i ].Size(); j++)
		{
			if (sendPacketSet[ i ][ j ]->reliability==RELIABLE_ORDERED ||
				sendPacketSet[ i ][ j ]->reliability==RELIABLE_SEQUENCED ||
				sendPacketSet[ i ][ j ]->reliability==RELIABLE)
				return true;
		}
	}

	// return resendTree.IsEmpty()==false;
	return numPacketsOnResendBuffer!=0;
}

bool ReliabilityLayer::AreAcksWaiting(void)
{
	return acknowlegements.Size() > 0;
}

//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::ApplyNetworkSimulator( double _maxSendBPS, RakNetTime _minExtraPing, RakNetTime _extraPingVariance )
{
#ifndef _RELEASE
	maxSendBPS=_maxSendBPS;
	minExtraPing=_minExtraPing;
	extraPingVariance=_extraPingVariance;
	//	if (ping < (unsigned int)(minExtraPing+extraPingVariance)*2)
	//		ping=(minExtraPing+extraPingVariance)*2;
#endif
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SetSplitMessageProgressInterval(int interval)
{
	splitMessageProgressInterval=interval;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SetUnreliableTimeout(RakNetTime timeoutMS)
{
	unreliableTimeout=(RakNetTimeUS)timeoutMS*(RakNetTimeUS)1000;
}

//-------------------------------------------------------------------------------------------------------
// This will return true if we should not send at this time
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsSendThrottled( int MTUSize )
{
	(void) MTUSize;

	return false;
	//	return resendList.Size() > windowSize;

	// Disabling this, because it can get stuck here forever
	/*
	unsigned packetsWaiting;
	unsigned resendListDataSize=0;
	unsigned i;
	for (i=0; i < resendList.Size(); i++)
	{
	if (resendList[i])
	resendListDataSize+=resendList[i]->dataBitLength;
	}
	packetsWaiting = 1 + ((BITS_TO_BYTES(resendListDataSize)) / (MTUSize - UDP_HEADER_SIZE - 10)); // 10 to roughly estimate the raknet header

	return packetsWaiting >= windowSize;
	*/
}

//-------------------------------------------------------------------------------------------------------
// We lost a packet
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::UpdateWindowFromPacketloss( RakNetTimeUS time )
{
	(void) time;
}

//-------------------------------------------------------------------------------------------------------
// Increase the window size
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::UpdateWindowFromAck( RakNetTimeUS time )
{
	(void) time;
}

//-------------------------------------------------------------------------------------------------------
// Does what the function name says
//-------------------------------------------------------------------------------------------------------
unsigned ReliabilityLayer::RemovePacketFromResendListAndDeleteOlderReliableSequenced( const MessageNumberType messageNumber, RakNetTimeUS time, DataStructures::List<PluginInterface2*> &messageHandlerList, SystemAddress systemAddress )
{
	(void) time;
	(void) messageNumber;
	InternalPacket * internalPacket;
	//InternalPacket *temp;
	PacketReliability reliability; // What type of reliability algorithm to use with this packet
	unsigned char orderingChannel; // What ordering channel this packet is on, if the reliability type uses ordering channels
	OrderingIndexType orderingIndex; // The ID used as identification for ordering channels
	//	unsigned j;


	for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
		messageHandlerList[messageHandlerIndex]->OnAck(messageNumber, systemAddress, (RakNetTime)(time/(RakNetTimeUS)1000));

//	bool deleted;
//	deleted=resendTree.Delete(messageNumber, internalPacket);
	internalPacket = resendBuffer[messageNumber & RESEND_BUFFER_ARRAY_MASK];
	if (internalPacket)
	{
		ValidateResendList();
		resendBuffer[messageNumber & RESEND_BUFFER_ARRAY_MASK]=0;
		CC_DEBUG_PRINTF_2("AckRcv %i ", messageNumber);

		numPacketsOnResendBuffer--;

		statistics.acknowlegementsReceived++;

		reliability = (PacketReliability) internalPacket->reliability;
		orderingChannel = internalPacket->orderingChannel;
		orderingIndex = internalPacket->orderingIndex;
		totalUserDataBytesAcked+=(double) BITS_TO_BYTES(internalPacket->headerLength+internalPacket->dataBitLength);

		RemoveFromList(internalPacket, true);
		rakFree_Ex(internalPacket->data, __FILE__, __LINE__ );
		ReleaseToInternalPacketPool( internalPacket );


		// Set to zero when it becomes empty
		if (resendLinkedListHead==0)
			timeResendQueueNonEmpty=0;
		ValidateResendList();

		//printf("GOT_ACK:%i ", messageNumber);

		return 0;
	}
	else
	{

		statistics.duplicateAcknowlegementsReceived++;

//		printf("DUP MSG %i\n", messageNumber);
	}

	return (unsigned)-1;
}

//-------------------------------------------------------------------------------------------------------
// Acknowledge receipt of the packet with the specified messageNumber
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SendAcknowledgementPacket( const DatagramSequenceNumberType messageNumber, RakNetTimeUS time )
{

	//	(void) time;
	//	(void) messageNumber;
	statistics.acknowlegementsSent++;
	nextAckTimeToSend=time;
	acknowlegements.Insert(messageNumber);

	//printf("ACK_DG:%i ", messageNumber.val);

	CC_DEBUG_PRINTF_2("AckPush %i ", messageNumber);

}

//-------------------------------------------------------------------------------------------------------
// Parse an internalPacket and figure out how many header bits would be
// written.  Returns that number
//-------------------------------------------------------------------------------------------------------
BitSize_t ReliabilityLayer::GetBitStreamHeaderLength( const InternalPacket *const internalPacket, bool includeDatagramHeader )
{
	(void) includeDatagramHeader;

	BitSize_t bitLength;
	bitLength = 8*2; // bitStream->Write(internalPacket->dataBitLength); // Length of message (2 bytes)
	if ( internalPacket->reliability == RELIABLE || internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_ORDERED )
		bitLength += 8*3; // bitStream->Write(internalPacket->reliableMessageNumber); // 3 bytes.
	bitLength += 8*1; // tempChar=internalPacket->reliability; bitStream->WriteBits( (const unsigned char *)&tempChar, 3, true ); // 3 bits to write reliability.
	// bool hasSplitPacket = internalPacket->splitPacketCount>0; bitStream->Write(hasSplitPacket); // Write 1 bit to indicate if splitPacketCount>0
	// bitStream->AlignWriteToByteBoundary(); // Potentially nothing else to write
	if ( internalPacket->reliability == UNRELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_ORDERED )
	{
	bitLength += 8*3; // 	bitStream->Write(internalPacket->orderingIndex); // Used for UNRELIABLE_SEQUENCED, RELIABLE_SEQUENCED, RELIABLE_ORDERED. 3 bytes.
	bitLength += 8*1; // 	tempChar=internalPacket->orderingChannel; bitStream->Write(tempChar); // Used for UNRELIABLE_SEQUENCED, RELIABLE_SEQUENCED, RELIABLE_ORDERED. 5 bits needed, write one byte
	}
	if (internalPacket->splitPacketCount>0)
	{
	bitLength += 8*4; // 	bitStream->Write(internalPacket->splitPacketCount); // Only needed if splitPacketCount>0. 4 bytes
	bitLength += 8*sizeof(SplitPacketIdType); // 	bitStream->Write(internalPacket->splitPacketId); // Only needed if splitPacketCount>0.
	bitLength += 8*4; // 	bitStream->Write(internalPacket->splitPacketIndex); // Only needed if splitPacketCount>0. 4 bytes
	}
	return bitLength;
}

//-------------------------------------------------------------------------------------------------------
// Parse an internalPacket and create a bitstream to represent this data
//-------------------------------------------------------------------------------------------------------
BitSize_t ReliabilityLayer::WriteToBitStreamFromInternalPacket( RakNet::BitStream *bitStream, const InternalPacket *const internalPacket, RakNetTimeUS curTime )
{
	(void) curTime;

	BitSize_t start = bitStream->GetNumberOfBitsUsed();
	unsigned char tempChar;
	
	// (Incoming data may be all zeros due to padding)
	bitStream->AlignWriteToByteBoundary(); // Potentially unaligned
	tempChar=(unsigned char)internalPacket->reliability; bitStream->WriteBits( (const unsigned char *)&tempChar, 3, true ); // 3 bits to write reliability.
	bool hasSplitPacket = internalPacket->splitPacketCount>0; bitStream->Write(hasSplitPacket); // Write 1 bit to indicate if splitPacketCount>0
	bitStream->AlignWriteToByteBoundary();
	RakAssert(internalPacket->dataBitLength < 65535);
	unsigned short s; s = (unsigned short) internalPacket->dataBitLength; bitStream->WriteAlignedVar16((const char*)& s);
	if ( internalPacket->reliability == RELIABLE || internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_ORDERED )
		bitStream->Write(internalPacket->reliableMessageNumber); // Message sequence number
	bitStream->AlignWriteToByteBoundary(); // Potentially nothing else to write
	if ( internalPacket->reliability == UNRELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_ORDERED )
	{
		bitStream->Write(internalPacket->orderingIndex); // Used for UNRELIABLE_SEQUENCED, RELIABLE_SEQUENCED, RELIABLE_ORDERED.
		tempChar=internalPacket->orderingChannel; bitStream->WriteAlignedVar8((const char*)& tempChar); // Used for UNRELIABLE_SEQUENCED, RELIABLE_SEQUENCED, RELIABLE_ORDERED. 5 bits needed, write one byte
	}
	if (internalPacket->splitPacketCount>0)
	{
		bitStream->WriteAlignedVar32((const char*)& internalPacket->splitPacketCount); RakAssert(sizeof(SplitPacketIndexType)==4); // Only needed if splitPacketCount>0. 4 bytes
		bitStream->WriteAlignedVar16((const char*)& internalPacket->splitPacketId); RakAssert(sizeof(SplitPacketIdType)==2); // Only needed if splitPacketCount>0.
		bitStream->WriteAlignedVar32((const char*)& internalPacket->splitPacketIndex); // Only needed if splitPacketCount>0. 4 bytes
	}
	
	// Write the actual data.
	bitStream->WriteAlignedBytes( ( unsigned char* ) internalPacket->data, BITS_TO_BYTES( internalPacket->dataBitLength ) );

	return bitStream->GetNumberOfBitsUsed() - start;
}

//-------------------------------------------------------------------------------------------------------
// Parse a bitstream and create an internal packet to represent this data
//-------------------------------------------------------------------------------------------------------
InternalPacket* ReliabilityLayer::CreateInternalPacketFromBitStream( RakNet::BitStream *bitStream, RakNetTimeUS time )
{
	bool bitStreamSucceeded;
	InternalPacket* internalPacket;
	unsigned char tempChar;
	bool hasSplitPacket;
	bool readSuccess;

	if ( bitStream->GetNumberOfUnreadBits() < (int) sizeof( internalPacket->reliableMessageNumber ) * 8 )
		return 0; // leftover bits

	internalPacket = AllocateFromInternalPacketPool();
	if (internalPacket==0)
	{
		// Out of memory
		RakAssert(0);
		return 0;
	}
	internalPacket->creationTime = time;

	// (Incoming data may be all zeros due to padding)
	bitStream->AlignReadToByteBoundary(); // Potentially unaligned
	bitStream->ReadBits( ( unsigned char* ) ( &( tempChar ) ), 3 );
	internalPacket->reliability = ( const PacketReliability ) tempChar;
	readSuccess=bitStream->Read(hasSplitPacket); // Read 1 bit to indicate if splitPacketCount>0
	bitStream->AlignReadToByteBoundary();
	unsigned short s; bitStream->ReadAlignedVar16((char*)&s); internalPacket->dataBitLength=s; // Length of message (2 bytes)
	if ( internalPacket->reliability == RELIABLE || internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_ORDERED )
		bitStream->Read(internalPacket->reliableMessageNumber); // Message sequence number
	else
		internalPacket->reliableMessageNumber=(MessageNumberType)(const uint32_t)-1;
	bitStream->AlignReadToByteBoundary(); // Potentially nothing else to Read
	if ( internalPacket->reliability == UNRELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_ORDERED )
	{
		bitStream->Read(internalPacket->orderingIndex); // Used for UNRELIABLE_SEQUENCED, RELIABLE_SEQUENCED, RELIABLE_ORDERED. 4 bytes.
		readSuccess=bitStream->ReadAlignedVar8((char*)& internalPacket->orderingChannel); // Used for UNRELIABLE_SEQUENCED, RELIABLE_SEQUENCED, RELIABLE_ORDERED. 5 bits needed, Read one byte
	}
	else
		internalPacket->orderingChannel=0;
	if (hasSplitPacket)
	{
		bitStream->ReadAlignedVar32((char*)& internalPacket->splitPacketCount); // Only needed if splitPacketCount>0. 4 bytes
		bitStream->ReadAlignedVar16((char*)& internalPacket->splitPacketId); // Only needed if splitPacketCount>0.
		readSuccess=bitStream->ReadAlignedVar32((char*)& internalPacket->splitPacketIndex); // Only needed if splitPacketCount>0. 4 bytes
		RakAssert(readSuccess);
	}
	else
	{
		internalPacket->splitPacketCount=0;
	}

	if (readSuccess==false ||
		internalPacket->dataBitLength==0 ||
		internalPacket->reliability>=NUMBER_OF_RELIABILITIES ||
		internalPacket->orderingChannel>=32 || 
		(hasSplitPacket && (internalPacket->splitPacketIndex >= internalPacket->splitPacketCount)))
	{
		// If this assert hits, encoding is garbage
		RakAssert(internalPacket->dataBitLength==0);
		ReleaseToInternalPacketPool( internalPacket );
		return 0;
	}

	// Allocate memory to hold our data
	internalPacket->data = (unsigned char*) rakMalloc_Ex( (size_t) BITS_TO_BYTES( internalPacket->dataBitLength ), __FILE__, __LINE__ );

	if (internalPacket->data == 0)
	{
		RakAssert("Out of memory in ReliabilityLayer::CreateInternalPacketFromBitStream" && 0);
		notifyOutOfMemory(__FILE__, __LINE__);
		ReleaseToInternalPacketPool( internalPacket );
		return 0;
	}

	// Set the last byte to 0 so if ReadBits does not read a multiple of 8 the last bits are 0'ed out
	internalPacket->data[ BITS_TO_BYTES( internalPacket->dataBitLength ) - 1 ] = 0;

	// Read the data the packet holds
	bitStreamSucceeded = bitStream->ReadAlignedBytes( ( unsigned char* ) internalPacket->data, BITS_TO_BYTES( internalPacket->dataBitLength ) );

	if ( bitStreamSucceeded == false )
	{
		// If this hits, most likely the variable buff is too small in RunUpdateCycle in RakPeer.cpp
		RakAssert("Couldn't read all the data"  && 0);

		rakFree_Ex(internalPacket->data, __FILE__, __LINE__ );
		ReleaseToInternalPacketPool( internalPacket );
		return 0;
	}

	return internalPacket;
}


//-------------------------------------------------------------------------------------------------------
// Get the SHA1 code
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::GetSHA1( unsigned char * const buffer, unsigned int
							   nbytes, char code[ SHA1_LENGTH ] )
{
	CSHA1 sha1;

	sha1.Reset();
	sha1.Update( ( unsigned char* ) buffer, nbytes );
	sha1.Final();
	memcpy( code, sha1.GetHash(), SHA1_LENGTH );
}

//-------------------------------------------------------------------------------------------------------
// Check the SHA1 code
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::CheckSHA1( char code[ SHA1_LENGTH ], unsigned char *
								 const buffer, unsigned int nbytes )
{
	char code2[ SHA1_LENGTH ];
	GetSHA1( buffer, nbytes, code2 );

	for ( int i = 0; i < SHA1_LENGTH; i++ )
		if ( code[ i ] != code2[ i ] )
			return false;

	return true;
}

//-------------------------------------------------------------------------------------------------------
// Search the specified list for sequenced packets on the specified ordering
// stream, optionally skipping those with splitPacketId, and delete them
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::DeleteSequencedPacketsInList( unsigned char orderingChannel, DataStructures::List<InternalPacket*>&theList, int splitPacketId )
{
	unsigned i = 0;

	while ( i < theList.Size() )
	{
		if ( ( theList[ i ]->reliability == RELIABLE_SEQUENCED || theList[ i ]->reliability == UNRELIABLE_SEQUENCED ) &&
			theList[ i ]->orderingChannel == orderingChannel && ( splitPacketId == -1 || theList[ i ]->splitPacketId != (unsigned int) splitPacketId ) )
		{
			InternalPacket * internalPacket = theList[ i ];
			theList.RemoveAtIndex( i );
			rakFree_Ex(internalPacket->data, __FILE__, __LINE__ );
			ReleaseToInternalPacketPool( internalPacket );
		}

		else
			i++;
	}
}

//-------------------------------------------------------------------------------------------------------
// Search the specified list for sequenced packets with a value less than orderingIndex and delete them
// Note - I added functionality so you can use the Queue as a list (in this case for searching) but it is less efficient to do so than a regular list
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::DeleteSequencedPacketsInList( unsigned char orderingChannel, DataStructures::Queue<InternalPacket*>&theList )
{
	InternalPacket * internalPacket;
	int listSize = theList.Size();
	int i = 0;

	while ( i < listSize )
	{
		if ( ( theList[ i ]->reliability == RELIABLE_SEQUENCED || theList[ i ]->reliability == UNRELIABLE_SEQUENCED ) && theList[ i ]->orderingChannel == orderingChannel )
		{
			internalPacket = theList[ i ];
			theList.RemoveAtIndex( i );
			rakFree_Ex(internalPacket->data, __FILE__, __LINE__ );
			ReleaseToInternalPacketPool( internalPacket );
			listSize--;
		}

		else
			i++;
	}
}

//-------------------------------------------------------------------------------------------------------
// Returns true if newPacketOrderingIndex is older than the waitingForPacketOrderingIndex
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsOlderOrderedPacket( OrderingIndexType newPacketOrderingIndex, OrderingIndexType waitingForPacketOrderingIndex )
{
	OrderingIndexType maxRange = (OrderingIndexType) (const uint32_t)-1;

	if ( waitingForPacketOrderingIndex > maxRange/(OrderingIndexType)2 )
	{
		if ( newPacketOrderingIndex >= waitingForPacketOrderingIndex - maxRange/(OrderingIndexType)2+(OrderingIndexType)1 && newPacketOrderingIndex < waitingForPacketOrderingIndex )
		{
			return true;
		}
	}

	else
		if ( newPacketOrderingIndex >= ( OrderingIndexType ) ( waitingForPacketOrderingIndex - (( OrderingIndexType ) maxRange/(OrderingIndexType)2+(OrderingIndexType)1) ) ||
			newPacketOrderingIndex < waitingForPacketOrderingIndex )
		{
			return true;
		}

		// Old packet
		return false;
}

//-------------------------------------------------------------------------------------------------------
// Split the passed packet into chunks under MTU_SIZEbytes (including headers) and save those new chunks
// Optimized version
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SplitPacket( InternalPacket *internalPacket )
{
	// Doing all sizes in bytes in this function so I don't write partial bytes with split packets
	internalPacket->splitPacketCount = 1; // This causes GetBitStreamHeaderLength to account for the split packet header
	unsigned int headerLength = (unsigned int) BITS_TO_BYTES( GetBitStreamHeaderLength( internalPacket, true ) );
	unsigned int dataByteLength = (unsigned int) BITS_TO_BYTES( internalPacket->dataBitLength );
	int maxDataSize;
	int maximumSendBlock, byteOffset, bytesToSend;
	SplitPacketIndexType splitPacketIndex;
	int i;
	InternalPacket **internalPacketArray;

	maxDataSize = congestionManager.GetMaxDatagramPayload();

#ifdef _DEBUG
	// Make sure we need to split the packet to begin with
	RakAssert( dataByteLength > maxDataSize - headerLength );
#endif

	// How much to send in the largest block
	maximumSendBlock = maxDataSize - headerLength;

	// Calculate how many packets we need to create
	internalPacket->splitPacketCount = ( ( dataByteLength - 1 ) / ( maximumSendBlock ) + 1 );

	statistics.totalSplits += internalPacket->splitPacketCount;

	// Optimization
	// internalPacketArray = RakNet::OP_NEW<InternalPacket*>(internalPacket->splitPacketCount, __FILE__, __LINE__ );
	bool usedAlloca=false;
#if !defined(_XBOX) && !defined(_X360)
	if (sizeof( InternalPacket* ) * internalPacket->splitPacketCount < MAX_ALLOCA_STACK_ALLOCATION)
	{
		internalPacketArray = ( InternalPacket** ) alloca( sizeof( InternalPacket* ) * internalPacket->splitPacketCount );
		usedAlloca=true;
	}
	else
#endif
		internalPacketArray = (InternalPacket**) rakMalloc_Ex( sizeof(InternalPacket*) * internalPacket->splitPacketCount, __FILE__, __LINE__ );

	for ( i = 0; i < ( int ) internalPacket->splitPacketCount; i++ )
	{
		internalPacketArray[ i ] = AllocateFromInternalPacketPool();
		//internalPacketArray[ i ] = (InternalPacket*) alloca( sizeof( InternalPacket ) );
		//		internalPacketArray[ i ] = sendPacketSet[internalPacket->priority].WriteLock();
		memcpy( internalPacketArray[ i ], internalPacket, sizeof( InternalPacket ) );
	}

	// This identifies which packet this is in the set
	splitPacketIndex = 0;

	// Do a loop to send out all the packets
	do
	{
		byteOffset = splitPacketIndex * maximumSendBlock;
		bytesToSend = dataByteLength - byteOffset;

		if ( bytesToSend > maximumSendBlock )
			bytesToSend = maximumSendBlock;

		// Copy over our chunk of data
		internalPacketArray[ splitPacketIndex ]->data = (unsigned char*) rakMalloc_Ex( bytesToSend, __FILE__, __LINE__ );

		memcpy( internalPacketArray[ splitPacketIndex ]->data, internalPacket->data + byteOffset, bytesToSend );

		if ( bytesToSend != maximumSendBlock )
			internalPacketArray[ splitPacketIndex ]->dataBitLength = internalPacket->dataBitLength - splitPacketIndex * ( maximumSendBlock << 3 );
		else
			internalPacketArray[ splitPacketIndex ]->dataBitLength = bytesToSend << 3;

		internalPacketArray[ splitPacketIndex ]->splitPacketIndex = splitPacketIndex;
		internalPacketArray[ splitPacketIndex ]->splitPacketId = splitPacketId;
		internalPacketArray[ splitPacketIndex ]->splitPacketCount = internalPacket->splitPacketCount;

		if ( splitPacketIndex > 0 )   // For the first split packet index we keep the reliableMessageNumber already assigned
		{
			// For every further packet we use a new reliableMessageNumber.
			// Note that all split packets are reliable
			//	internalPacketArray[ splitPacketIndex ]->reliableMessageNumber = sendMessageNumberIndex;
			internalPacketArray[ splitPacketIndex ]->reliableMessageNumber = (MessageNumberType) (const uint32_t)-1;
			internalPacketArray[ splitPacketIndex ]->messageNumberAssigned=false;
		}

	}

	while ( ++splitPacketIndex < internalPacket->splitPacketCount );

	splitPacketId++; // It's ok if this wraps to 0

	//	InternalPacket *workingPacket;

	// Copy all the new packets into the split packet list
	for ( i = 0; i < ( int ) internalPacket->splitPacketCount; i++ )
	{
		internalPacket->headerLength=headerLength;
		sendPacketSet[ internalPacket->priority ].Push( internalPacketArray[ i ] );
		//		workingPacket=sendPacketSet[internalPacket->priority].WriteLock();
		//		memcpy(workingPacket, internalPacketArray[ i ], sizeof(InternalPacket));
		//		sendPacketSet[internalPacket->priority].WriteUnlock();
	}

	// Delete the original
	rakFree_Ex(internalPacket->data, __FILE__, __LINE__ );
	ReleaseToInternalPacketPool( internalPacket );

	if (usedAlloca==false)
		rakFree_Ex(internalPacketArray, __FILE__, __LINE__ );
}

//-------------------------------------------------------------------------------------------------------
// Insert a packet into the split packet list
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::InsertIntoSplitPacketList( InternalPacket * internalPacket, RakNetTimeUS time )
{
	bool objectExists;
	unsigned index;
	index=splitPacketChannelList.GetIndexFromKey(internalPacket->splitPacketId, &objectExists);
	if (objectExists==false)
	{
		SplitPacketChannel *newChannel = RakNet::OP_NEW<SplitPacketChannel>( __FILE__, __LINE__ );
		//SplitPacketChannel *newChannel = RakNet::OP_NEW<SplitPacketChannel>(1, __FILE__, __LINE__ );
		newChannel->firstPacket=0;
		index=splitPacketChannelList.Insert(internalPacket->splitPacketId, newChannel, true);
		// Preallocate to the final size, to avoid runtime copies
		newChannel->splitPacketList.Preallocate(internalPacket->splitPacketCount);
	}
	splitPacketChannelList[index]->splitPacketList.Insert(internalPacket);
	splitPacketChannelList[index]->lastUpdateTime=time;

	if (internalPacket->splitPacketIndex==0)
		splitPacketChannelList[index]->firstPacket=internalPacket;

	if (splitMessageProgressInterval &&
		splitPacketChannelList[index]->firstPacket &&
		splitPacketChannelList[index]->splitPacketList.Size()!=splitPacketChannelList[index]->firstPacket->splitPacketCount &&
		(splitPacketChannelList[index]->splitPacketList.Size()%splitMessageProgressInterval)==0)
	{
		// Return ID_DOWNLOAD_PROGRESS
		// Write splitPacketIndex (SplitPacketIndexType)
		// Write splitPacketCount (SplitPacketIndexType)
		// Write byteLength (4)
		// Write data, splitPacketChannelList[index]->splitPacketList[0]->data
		InternalPacket *progressIndicator = AllocateFromInternalPacketPool();
		unsigned int length = sizeof(MessageID) + sizeof(unsigned int)*2 + sizeof(unsigned int) + (unsigned int) BITS_TO_BYTES(splitPacketChannelList[index]->firstPacket->dataBitLength);
		progressIndicator->data = (unsigned char*) rakMalloc_Ex( length, __FILE__, __LINE__ );
		progressIndicator->dataBitLength=BYTES_TO_BITS(length);
		progressIndicator->data[0]=(MessageID)ID_DOWNLOAD_PROGRESS;
		unsigned int temp;
		temp=splitPacketChannelList[index]->splitPacketList.Size();
		memcpy(progressIndicator->data+sizeof(MessageID), &temp, sizeof(unsigned int));
		temp=(unsigned int)internalPacket->splitPacketCount;
		memcpy(progressIndicator->data+sizeof(MessageID)+sizeof(unsigned int)*1, &temp, sizeof(unsigned int));
		temp=(unsigned int) BITS_TO_BYTES(splitPacketChannelList[index]->firstPacket->dataBitLength);
		memcpy(progressIndicator->data+sizeof(MessageID)+sizeof(unsigned int)*2, &temp, sizeof(unsigned int));

		memcpy(progressIndicator->data+sizeof(MessageID)+sizeof(unsigned int)*3, splitPacketChannelList[index]->firstPacket->data, (size_t) BITS_TO_BYTES(splitPacketChannelList[index]->firstPacket->dataBitLength));
		outputQueue.Push(progressIndicator);
	}
}

//-------------------------------------------------------------------------------------------------------
// Take all split chunks with the specified splitPacketId and try to
//reconstruct a packet.  If we can, allocate and return it.  Otherwise return 0
// Optimized version
//-------------------------------------------------------------------------------------------------------
InternalPacket * ReliabilityLayer::BuildPacketFromSplitPacketList( SplitPacketChannel *splitPacketChannel, RakNetTimeUS time )
{
	unsigned int j;
	InternalPacket * internalPacket, *splitPacket;
	int splitPacketPartLength;

	// Reconstruct
	internalPacket = CreateInternalPacketCopy( splitPacketChannel->splitPacketList[0], 0, 0, time );
	internalPacket->dataBitLength=0;
	for (j=0; j < splitPacketChannel->splitPacketList.Size(); j++)
		internalPacket->dataBitLength+=splitPacketChannel->splitPacketList[j]->dataBitLength;
	splitPacketPartLength=BITS_TO_BYTES(splitPacketChannel->firstPacket->dataBitLength);

	internalPacket->data = (unsigned char*) rakMalloc_Ex( (size_t) BITS_TO_BYTES( internalPacket->dataBitLength ), __FILE__, __LINE__ );

	for (j=0; j < splitPacketChannel->splitPacketList.Size(); j++)
	{
		splitPacket=splitPacketChannel->splitPacketList[j];
		memcpy(internalPacket->data+splitPacket->splitPacketIndex*splitPacketPartLength, splitPacket->data, (size_t) BITS_TO_BYTES(splitPacketChannel->splitPacketList[j]->dataBitLength));
	}

	for (j=0; j < splitPacketChannel->splitPacketList.Size(); j++)
	{
		rakFree_Ex(splitPacketChannel->splitPacketList[j]->data, __FILE__, __LINE__ );
		ReleaseToInternalPacketPool(splitPacketChannel->splitPacketList[j]);
	}
	RakNet::OP_DELETE(splitPacketChannel, __FILE__, __LINE__);

	return internalPacket;
}
//-------------------------------------------------------------------------------------------------------
InternalPacket * ReliabilityLayer::BuildPacketFromSplitPacketList( SplitPacketIdType splitPacketId, RakNetTimeUS time,
					SOCKET s, SystemAddress systemAddress, RakNetRandom *rnr, unsigned short remotePortRakNetWasStartedOn_PS3)
{
	unsigned int i;
	bool objectExists;
	SplitPacketChannel *splitPacketChannel;
	InternalPacket * internalPacket;

	i=splitPacketChannelList.GetIndexFromKey(splitPacketId, &objectExists);
	splitPacketChannel=splitPacketChannelList[i];
	if (splitPacketChannel->splitPacketList.Size()==splitPacketChannel->splitPacketList[0]->splitPacketCount)
	{
		// Ack immediately, because for large files this can take a long time
		SendACKs(s, systemAddress, time, rnr, remotePortRakNetWasStartedOn_PS3);
		internalPacket=BuildPacketFromSplitPacketList(splitPacketChannel,time);
		splitPacketChannelList.RemoveAtIndex(i);
		return internalPacket;
	}
	else
	{
		return 0;
	}
}
//-------------------------------------------------------------------------------------------------------
// Delete any unreliable split packets that have long since expired
void ReliabilityLayer::DeleteOldUnreliableSplitPackets( RakNetTimeUS time )
{
	unsigned i,j;
	i=0;
	while (i < splitPacketChannelList.Size())
	{
		if (time > splitPacketChannelList[i]->lastUpdateTime + (RakNetTimeUS)timeoutTime*(RakNetTimeUS)1000 &&
			(splitPacketChannelList[i]->splitPacketList[0]->reliability==UNRELIABLE || splitPacketChannelList[i]->splitPacketList[0]->reliability==UNRELIABLE_SEQUENCED))
		{
			for (j=0; j < splitPacketChannelList[i]->splitPacketList.Size(); j++)
			{
				RakNet::OP_DELETE_ARRAY(splitPacketChannelList[i]->splitPacketList[j]->data, __FILE__, __LINE__);
				ReleaseToInternalPacketPool(splitPacketChannelList[i]->splitPacketList[j]);
			}
			RakNet::OP_DELETE(splitPacketChannelList[i], __FILE__, __LINE__);
			splitPacketChannelList.RemoveAtIndex(i);
		}
		else
			i++;
	}
}

//-------------------------------------------------------------------------------------------------------
// Creates a copy of the specified internal packet with data copied from the original starting at dataByteOffset for dataByteLength bytes.
// Does not copy any split data parameters as that information is always generated does not have any reason to be copied
//-------------------------------------------------------------------------------------------------------
InternalPacket * ReliabilityLayer::CreateInternalPacketCopy( InternalPacket *original, int dataByteOffset, int dataByteLength, RakNetTimeUS time )
{
	InternalPacket * copy = AllocateFromInternalPacketPool();
#ifdef _DEBUG
	// Remove accessing undefined memory error
	memset( copy, 255, sizeof( InternalPacket ) );
#endif
	// Copy over our chunk of data

	if ( dataByteLength > 0 )
	{
		copy->data = (unsigned char*) rakMalloc_Ex( BITS_TO_BYTES(dataByteLength ), __FILE__, __LINE__ );
		memcpy( copy->data, original->data + dataByteOffset, dataByteLength );
	}
	else
		copy->data = 0;

	copy->dataBitLength = dataByteLength << 3;
	copy->creationTime = time;
	copy->nextActionTime = 0;
	copy->orderingIndex = original->orderingIndex;
	copy->orderingChannel = original->orderingChannel;
	copy->reliableMessageNumber = original->reliableMessageNumber;
	copy->priority = original->priority;
	copy->reliability = original->reliability;

	// REMOVEME
	//	if (copy->reliableMessageNumber > 30000)
	//	{
	//		int a=5;
	//	}

	return copy;
}

//-------------------------------------------------------------------------------------------------------
// Get the specified ordering list
//-------------------------------------------------------------------------------------------------------
DataStructures::LinkedList<InternalPacket*> *ReliabilityLayer::GetOrderingListAtOrderingStream( unsigned char orderingChannel )
{
	if ( orderingChannel >= orderingList.Size() )
		return 0;

	return orderingList[ orderingChannel ];
}

//-------------------------------------------------------------------------------------------------------
// Add the internal packet to the ordering list in order based on order index
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::AddToOrderingList( InternalPacket * internalPacket )
{
#ifdef _DEBUG
	RakAssert( internalPacket->orderingChannel < NUMBER_OF_ORDERED_STREAMS );
#endif

	if ( internalPacket->orderingChannel >= NUMBER_OF_ORDERED_STREAMS )
	{
		return;
	}

	DataStructures::LinkedList<InternalPacket*> *theList;

	if ( internalPacket->orderingChannel >= orderingList.Size() || orderingList[ internalPacket->orderingChannel ] == 0 )
	{
		// Need a linked list in this index
		orderingList.Replace( RakNet::OP_NEW<DataStructures::LinkedList<InternalPacket*> >(__FILE__,__LINE__) , 0, internalPacket->orderingChannel );
		theList=orderingList[ internalPacket->orderingChannel ];
	}
	else
	{
		// Have a linked list in this index
		if ( orderingList[ internalPacket->orderingChannel ]->Size() == 0 )
		{
			theList=orderingList[ internalPacket->orderingChannel ];
		}
		else
		{
			theList = GetOrderingListAtOrderingStream( internalPacket->orderingChannel );
		}
	}

	theList->End();
	theList->Add(internalPacket);
}

//-------------------------------------------------------------------------------------------------------
// Inserts a packet into the resend list in order
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::InsertPacketIntoResendList( InternalPacket *internalPacket, RakNetTimeUS time, bool makeCopyOfInternalPacket, bool firstResend, bool modifyUnacknowledgedBytes )
{
	(void) makeCopyOfInternalPacket;
	(void) firstResend;
	(void) time;
	(void) internalPacket;

	if (makeCopyOfInternalPacket)
	{
		InternalPacket *pool=AllocateFromInternalPacketPool();
		memcpy(pool, internalPacket, sizeof(InternalPacket));
		AddToListTail(pool, modifyUnacknowledgedBytes);
	}
	else
	{
		AddToListTail(internalPacket, modifyUnacknowledgedBytes);
	}

	RakAssert(internalPacket->nextActionTime!=0);

}

//-------------------------------------------------------------------------------------------------------
// If Read returns -1 and this returns true then a modified packet was detected
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsCheater( void ) const
{
	return cheater;
}

//-------------------------------------------------------------------------------------------------------
//  Were you ever unable to deliver a packet despite retries?
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsDeadConnection( void ) const
{
	return deadConnection;
}

//-------------------------------------------------------------------------------------------------------
//  Causes IsDeadConnection to return true
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::KillConnection( void )
{
	deadConnection=true;
}


//-------------------------------------------------------------------------------------------------------
// Statistics
//-------------------------------------------------------------------------------------------------------
RakNetStatistics * const ReliabilityLayer::GetStatistics( RakNetStatistics *rns )
{
	unsigned i;
	memcpy(rns, &statistics, sizeof(statistics));

	for ( i = 0; i < NUMBER_OF_PRIORITIES; i++ )
	{
		rns->messageSendBuffer[i] = sendPacketSet[i].Size();
	}

	//	rns->acknowlegementsPending = acknowlegements.Size();
	rns->acknowlegementsPending = 0;

	rns->bitsPerSecondSent=bytesSentLastHistogram*10*8; // Polls 10 times a second
	rns->bitsPerSecondReceived=bytesReceivedLastHistogram*10*8;
	rns->estimatedLinkCapacityMBPS=congestionManager.GetEstimatedBandwidth();

	rns->messagesWaitingForReassembly = 0;
	for (i=0; i < splitPacketChannelList.Size(); i++)
		rns->messagesWaitingForReassembly+=splitPacketChannelList[i]->splitPacketList.Size();
	rns->internalOutputQueueSize = outputQueue.Size();
	//rns->lossySize = lossyWindowSize == MAXIMUM_WINDOW_SIZE + 1 ? 0 : lossyWindowSize;
	//	rns->lossySize=0;
	// The connection is full if we are continuously sending data and we had to throttle back recently.
	// rns->bandwidthExceeded = continuousSend && (lastUpdateTime-lastTimeBetweenPacketsIncrease) > (RakNetTimeUS) 1000000;
	rns->bandwidthExceeded = bandwidthExceededStatistic;
	rns->messagesOnResendQueue = GetResendListDataSize();

	rns->isInSlowStart=congestionManager.GetIsInSlowStart();
	rns->CWNDLimit=congestionManager.GetCWNDLimit();
	rns->unacknowledgedBytes=unacknowledgedBytes;
	rns->timeToNextAllowedSend=congestionManager.GetNextAllowedSend();

	rns->localContinuousReceiveRate=congestionManager.GetLocalReceiveRate(RakNet::GetTimeUS());
	rns->remoteContinuousReceiveRate=congestionManager.GetRemoveReceiveRate();
	if (rns->isInSlowStart)
	{
		rns->localSendRate=0.0;
		rns->estimatedLinkCapacityMBPS=0.0;
	}
	else
	{
		rns->localSendRate=congestionManager.GetLocalSendRate();
		rns->estimatedLinkCapacityMBPS=congestionManager.GetEstimatedBandwidth();
	}

	return rns;
}

//-------------------------------------------------------------------------------------------------------
// Returns the number of packets in the resend queue, not counting holes
//-------------------------------------------------------------------------------------------------------
unsigned int ReliabilityLayer::GetResendListDataSize(void) const
{
	// Not accurate but thread-safe.  The commented version might crash if the queue is cleared while we loop through it
	// return resendTree.Size();
	return numPacketsOnResendBuffer;
}

//-------------------------------------------------------------------------------------------------------
// Process threaded commands
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::UpdateThreadedMemory(void)
{
	if ( freeThreadedMemoryOnNextUpdate )
	{
		freeThreadedMemoryOnNextUpdate = false;
		FreeThreadedMemory();
	}
}
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::AckTimeout(RakNetTimeUS curTime)
{
	return curTime > timeResendQueueNonEmpty && timeResendQueueNonEmpty && curTime - timeResendQueueNonEmpty > (RakNetTimeUS)timeoutTime*1000;
}
//-------------------------------------------------------------------------------------------------------
RakNetTimeUS ReliabilityLayer::GetNextSendTime(void) const
{
	return nextSendTime;
}
//-------------------------------------------------------------------------------------------------------
RakNetTimeUS ReliabilityLayer::GetTimeBetweenPackets(void) const
{
	return timeBetweenPackets;
}
//-------------------------------------------------------------------------------------------------------
RakNetTimeUS ReliabilityLayer::GetAckPing(void) const
{
	return ackPing;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::ResetPacketsAndDatagrams(void)
{
	packetsToSendThisUpdate.Clear();
	packetsToDeallocThisUpdate.Clear();
	packetsToSendThisUpdateDatagramBoundaries.Clear();
	datagramsToSendThisUpdateIsPair.Clear();
	datagramSizesInBytes.Clear();
	datagramSizeSoFar=0;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::PushPacket(RakNetTimeUS time, InternalPacket *internalPacket, bool isReliable)
{
	datagramSizeSoFar+=internalPacket->dataBitLength+internalPacket->headerLength;
	RakAssert(BITS_TO_BYTES(datagramSizeSoFar)<MAXIMUM_MTU_SIZE-UDP_HEADER_SIZE);
	allDatagramSizesSoFar+=internalPacket->dataBitLength+internalPacket->headerLength;
	packetsToSendThisUpdate.Push(internalPacket);
	packetsToDeallocThisUpdate.Push(isReliable==false);
	RakAssert(internalPacket->headerLength==GetBitStreamHeaderLength(internalPacket,false));

#if defined(USE_THREADED_SEND)
	congestionManager.OnSendBytes(time, BITS_TO_BYTES(internalPacket->dataBitLength+internalPacket->headerLength));
#else
	congestionManager.OnSendBytes(time, BITS_TO_BYTES(internalPacket->dataBitLength+internalPacket->headerLength));
#endif
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::PushDatagram(void)
{
	if (datagramSizeSoFar>0)
	{
		packetsToSendThisUpdateDatagramBoundaries.Push(packetsToSendThisUpdate.Size());
		datagramsToSendThisUpdateIsPair.Push(false);
		RakAssert(BITS_TO_BYTES(datagramSizeSoFar)<MAXIMUM_MTU_SIZE-UDP_HEADER_SIZE);
		datagramSizesInBytes.Push(BITS_TO_BYTES(datagramSizeSoFar));
		datagramSizeSoFar=0;

		if (countdownToNextPacketPair==0)
		{
			if (TagMostRecentPushAsSecondOfPacketPair())
				countdownToNextPacketPair=15;
		}
		else
			countdownToNextPacketPair--;
	}
}
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::TagMostRecentPushAsSecondOfPacketPair(void)
{
	if (datagramsToSendThisUpdateIsPair.Size()>=2)
	{
		datagramsToSendThisUpdateIsPair[datagramsToSendThisUpdateIsPair.Size()-2]=true;
		datagramsToSendThisUpdateIsPair[datagramsToSendThisUpdateIsPair.Size()-1]=true;
		return true;
	}
	return false;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::ClearPacketsAndDatagrams(void)
{
	unsigned int i;
	for (i=0; i < packetsToDeallocThisUpdate.Size(); i++)
	{
		if (packetsToDeallocThisUpdate[i])
		{
			rakFree_Ex(packetsToSendThisUpdate[i]->data, __FILE__, __LINE__ );
			ReleaseToInternalPacketPool( packetsToSendThisUpdate[i] );
		}
	}
	packetsToDeallocThisUpdate.Clear();
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::MoveToListHead(InternalPacket *internalPacket)
{
	if ( internalPacket == resendLinkedListHead )
		return;
	if (resendLinkedListHead==0)
	{
		internalPacket->next=internalPacket;
		internalPacket->prev=internalPacket;
		resendLinkedListHead=internalPacket;
		return;
	}
	internalPacket->prev->next = internalPacket->next;
	internalPacket->next->prev = internalPacket->prev;
	internalPacket->next=resendLinkedListHead;
	internalPacket->prev=resendLinkedListHead->prev;
	internalPacket->prev->next=internalPacket;
	resendLinkedListHead->prev=internalPacket;
	resendLinkedListHead=internalPacket;
	RakAssert(internalPacket->headerLength+internalPacket->dataBitLength>0);

	ValidateResendList();
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::RemoveFromList(InternalPacket *internalPacket, bool modifyUnacknowledgedBytes)
{
	InternalPacket *newPosition;
	internalPacket->prev->next = internalPacket->next;
	internalPacket->next->prev = internalPacket->prev;
	newPosition = internalPacket->next;
	if ( internalPacket == resendLinkedListHead )
		resendLinkedListHead = newPosition;
	if (resendLinkedListHead==internalPacket)
		resendLinkedListHead=0;

	if (modifyUnacknowledgedBytes)
	{
		RakAssert(unacknowledgedBytes>=BITS_TO_BYTES(internalPacket->headerLength+internalPacket->dataBitLength));
		unacknowledgedBytes-=BITS_TO_BYTES(internalPacket->headerLength+internalPacket->dataBitLength);
		// printf("-unacknowledgedBytes:%i ", unacknowledgedBytes);


		ValidateResendList();
	}
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::AddToListTail(InternalPacket *internalPacket, bool modifyUnacknowledgedBytes)
{
	if (modifyUnacknowledgedBytes)
	{
		unacknowledgedBytes+=BITS_TO_BYTES(internalPacket->headerLength+internalPacket->dataBitLength);
		// printf("+unacknowledgedBytes:%i ", unacknowledgedBytes);
	}

	if (resendLinkedListHead==0)
	{
		internalPacket->next=internalPacket;
		internalPacket->prev=internalPacket;
		resendLinkedListHead=internalPacket;
		return;
	}
	internalPacket->next=resendLinkedListHead;
	internalPacket->prev=resendLinkedListHead->prev;
	internalPacket->prev->next=internalPacket;
	resendLinkedListHead->prev=internalPacket;

	ValidateResendList();

}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::PopListHead(bool modifyUnacknowledgedBytes)
{
	RakAssert(resendLinkedListHead!=0);
	RemoveFromList(resendLinkedListHead, modifyUnacknowledgedBytes);
}
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsResendQueueEmpty(void) const
{
	return resendLinkedListHead==0;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SendACKs(SOCKET s, SystemAddress systemAddress, RakNetTimeUS time, RakNetRandom *rnr, unsigned short remotePortRakNetWasStartedOn_PS3)
{

	BitSize_t maxDatagramPayload = BYTES_TO_BITS(congestionManager.GetMaxDatagramPayload());

	while (acknowlegements.Size()>0)
	{
		// Send acks
		updateBitStream.Reset();
		DatagramHeaderFormat dhf;
		dhf.isACK=true;
		dhf.isNAK=false;
		dhf.isPacketPair=false;
		dhf.sourceSystemTime=time;
		double B;
		double AS;
		bool hasBAndAS;
		congestionManager.OnSendAckGetBAndAS(time, &hasBAndAS,&B,&AS);
		dhf.hasBAndAS=hasBAndAS;
		dhf.sourceSystemTime=nextAckTimeToSend;
		dhf.B=(float)B;
		dhf.AS=(float)AS;
#if defined(USE_THREADED_SEND)
		SendToThread::SendToThreadBlock *sttb = AllocateSendToThreadBlock(s,systemAddress.binaryAddress,systemAddress.port,remotePortRakNetWasStartedOn_PS3);
		dhf.Serialize(sttb);
		// Won't work
		acknowlegements.Serialize(&updateBitStream, maxDatagramPayload, true)
			statistics.acknowlegementBitsSent +=BYTES_TO_BITS(sttb->dataWriteOffset);
		SendThreaded(sttb);
		congestionManager.OnSendAck(time,sttb->dataWriteOffset);
#else
		updateBitStream.Reset();
		dhf.Serialize(&updateBitStream);
		CC_DEBUG_PRINTF_1("AckSnd ");
		acknowlegements.Serialize(&updateBitStream, maxDatagramPayload, true);
		statistics.acknowlegementBitsSent += updateBitStream.GetNumberOfBitsUsed();
		SendBitStream( s, systemAddress, &updateBitStream, rnr, remotePortRakNetWasStartedOn_PS3 );
		congestionManager.OnSendAck(time,updateBitStream.GetNumberOfBytesUsed());
#endif

		// I think this is causing a bug where if the estimated bandwidth is very low for the recipient, only acks ever get sent
	//	congestionManager.OnSendBytes(time,UDP_HEADER_SIZE+updateBitStream.GetNumberOfBytesUsed());
	}


}
/*
//-------------------------------------------------------------------------------------------------------
ReliabilityLayer::DatagramMessageIDList* ReliabilityLayer::AllocateFromDatagramMessageIDPool(void)
{
	DatagramMessageIDList*s;
	s=datagramMessageIDPool.Allocate();
	// Call new operator, memoryPool doesn't do this
	s = new ((void*)s) DatagramMessageIDList;
	return s;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::ReleaseToDatagramMessageIDPool(DatagramMessageIDList* d)
{
	d->~DatagramMessageIDList();
	datagramMessageIDPool.Release(d);
}
*/
//-------------------------------------------------------------------------------------------------------
InternalPacket* ReliabilityLayer::AllocateFromInternalPacketPool(void)
{
	InternalPacket *ip = internalPacketPool.Allocate();
	return ip;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::ReleaseToInternalPacketPool(InternalPacket *ip)
{
	internalPacketPool.Release(ip);
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::ValidateResendList(void) const
{
	/*
	unsigned int count1=0, count2=0;
	for (unsigned int i=0; i < RESEND_BUFFER_ARRAY_LENGTH; i++)
		if (resendBuffer[i])
			count1++;
	
	if (resendLinkedListHead)
	{
		InternalPacket *internalPacket = resendLinkedListHead;
		do 
		{
			count2++;
			internalPacket=internalPacket->next;
		} while (internalPacket!=resendLinkedListHead);
	}
	RakAssert(count1==count2);
	RakAssert(count2<=RESEND_BUFFER_ARRAY_LENGTH);
	*/
}
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::ResendBufferOverflow(void) const
{
	int index1 = sendReliableMessageNumberIndex & (uint32_t) RESEND_BUFFER_ARRAY_MASK;
//	int index2 = (sendReliableMessageNumberIndex+(uint32_t)1) & (uint32_t) RESEND_BUFFER_ARRAY_MASK;
	RakAssert(index1<RESEND_BUFFER_ARRAY_LENGTH);
	return resendBuffer[index1]!=0; // || resendBuffer[index2]!=0;

}
//-------------------------------------------------------------------------------------------------------
#if defined(USE_THREADED_SEND)
SendToThread::SendToThreadBlock* ReliabilityLayer::AllocateSendToThreadBlock(SOCKET s,
																			 unsigned int binaryAddress,
																			 unsigned short port,
																			 unsigned short remotePortRakNetWasStartedOn_PS3)
{
	SendToThread::SendToThreadBlock *sttb = SendToThread::AllocateBlock();
	sttb->binaryAddress=binaryAddress;
	sttb->port=port;
	sttb->remotePortRakNetWasStartedOn_PS3=remotePortRakNetWasStartedOn_PS3;
	sttb->s=s;
	sttb->dataWriteOffset=0;
	return sttb;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SendThreaded(SendToThread::SendToThreadBlock* sttb)
{
	SendToThread::ProcessBlock(sttb);	
}
//-------------------------------------------------------------------------------------------------------
#endif


#if defined(RELIABILITY_LAYER_NEW_UNDEF_ALLOCATING_QUEUE)
#pragma pop_macro("new")
#undef RELIABILITY_LAYER_NEW_UNDEF_ALLOCATING_QUEUE
#endif


#ifdef _MSC_VER
#pragma warning( pop )
#endif
