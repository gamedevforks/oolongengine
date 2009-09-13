/*
 Oolong Engine for the iPhone / iPod touch
 Copyright (c) 2007-2008 Wolfgang Engel  http://code.google.com/p/oolongengine/
 
 Author: Joe Woynillowicz
 
 This software is provided 'as-is', without any express or implied warranty.
 In no event will the authors be held liable for any damages arising from the use of this software.
 Permission is granted to anyone to use this software for any purpose, 
 including commercial applications, and to alter it and redistribute it freely, 
 subject to the following restrictions:
 
 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.
 */

/* 
 This example is a simplified version of the Raknet sample "Chat Example" packaged with the Raknet SDK
 created by Jenkins Software http://www.jenkinssoftware.com/
 */

#include "MessageIdentifiers.h"
#include "RakNetworkFactory.h"
#include "RakPeerInterface.h"
#include "RakNetStatistics.h"
#include "RakNetTypes.h"
#include "BitStream.h"
#include "RakSleep.h"
#include "PacketLogger.h"
 
#include <iostream>

unsigned char GetPacketIdentifier(Packet *p);

int main (int argc, char * const argv[]) {
    
    RakPeerInterface* server = RakNetworkFactory::GetRakPeerInterface();
		
	server->SetIncomingPassword("Oolong", (int)strlen("Oolong"));
	server->SetTimeoutTime(5000,UNASSIGNED_SYSTEM_ADDRESS);
	
	Packet* packet;
	unsigned char packetIdentifier;
	
	SystemAddress clientID=UNASSIGNED_SYSTEM_ADDRESS;
	
	std::cout << "Starting server..." << std::endl;
	// Starting the server is very simple.  2 players allowed.
	// 0 means we don't care about a connectionValidationInteger, and false
	// for low priority threads
	SocketDescriptor socketDescriptor(9050,0);
	bool started = server->Startup(32, 30, &socketDescriptor, 1);
	server->SetMaximumIncomingConnections(5);
	if(started)
		std::cout << "Server started, waiting for connections." << std::endl;
	else {
		std::cout << "Server failed to start. Terminating!" << std::endl;
		exit(1);
	}
	
	server->SetOccasionalPing(true);
	server->SetUnreliableTimeout(1000);
	
	DataStructures::List<RakNetSmartPtr<RakNetSocket> > sockets;
	server->GetSockets(sockets);
	std::cout << "Ports used by RakNet: " << std::endl;
	for(unsigned int i = 0; i < sockets.Size(); i++)
	{
		std::cout << (i+1) << ". " << sockets[i]->boundAddress.port << std::endl;
	}
	
	char message[2048];
	
	// server loop
	while(1)
	{
		// This sleep keeps RakNet responsive
		RakSleep(30); 
		
		packet = server->Receive();
		if(packet == 0)
			continue;
		
		packetIdentifier = GetPacketIdentifier(packet);
		
		switch(packetIdentifier)
		{
			case ID_DISCONNECTION_NOTIFICATION:
				// Connection lost normally
				std::cout << "ID_DISCONNETION_NOTIFICATION from " << packet->systemAddress.ToString(true) << std::endl;
				break;
				
			case ID_NEW_INCOMING_CONNECTION:
				// Somebody connected.  We have their IP now
				std::cout << "ID_NEW_INCOMING_CONNECTION from " << packet->systemAddress.ToString(true) << " with GUID " << packet->guid.ToString() << std::endl;
				clientID = packet->systemAddress; // Record the player ID of the client
				break;	
		
			case ID_CONNECTION_LOST:
				// Couldn't deliver a reliable packet - i.e. the other system was abnormally
				// terminated
				std::cout << "ID_CONNECTION_LOST from " << packet->systemAddress.ToString(true) << std::endl;
				break;
				
			default:
				// The server knows the static data of all clients, so we can prefix the message
				// With the name data
				std::cout << packet->data << std::endl;
				
				// Relay the message.  We prefix the name for other clients.  This demonstrates
				// That messages can be changed on the server before being broadcast
				// Sending is the same as before
				sprintf(message, "%s", packet->data);
				server->Send(message, (const int)strlen(message)+1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
				break;				
		}
		
		server->DeallocatePacket(packet);
	}
	
	server->Shutdown(300);
	
	RakNetworkFactory::DestroyRakPeerInterface(server);

	
	return 0;
}

// If the first byte is ID_TIMESTAMP, then we want the 5th byte
// Otherwise we want the 1st byte
unsigned char GetPacketIdentifier(Packet *p)
{
	if (p==0)
		return 255;
	
	if ((unsigned char)p->data[0] == ID_TIMESTAMP)
	{
		RakAssert(p->length > sizeof(unsigned char) + sizeof(unsigned long));
		return (unsigned char) p->data[sizeof(unsigned char) + sizeof(unsigned long)];
	}
	else
		return (unsigned char) p->data[0];
}