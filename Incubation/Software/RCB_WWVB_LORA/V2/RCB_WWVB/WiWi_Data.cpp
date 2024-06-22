#include "WiWi_Data.h"

/******************** Global variables ***************/
uint8_t wiwi_mac_addr = 0;


//////// Data Path 3 & 4, free buffer list
LinkedList_git<int> free_packet_list = LinkedList_git<int>();

//////// Data Path 3 structures, RX packets
LinkedList_git<int> rx_packet_list = LinkedList_git<int>();

//////// Data Path 4 structures, TX packets
LinkedList_git<int> tx_packet_list = LinkedList_git<int>();

packet packet_buffer[PACKET_BUFFER_SIZE]; // actual packet buffer

//////// Client connections
ConnectionManager clientmanager; 


/******************** Generic client structure functions ***************/

// Initialize the connection manager
void ConnectionManager_Init(ConnectionManager* manager) {
    manager->numClients = 0;
}

// Add a new client to the manager
int ConnectionManager_AddClient(ConnectionManager* manager, int clientID) {
    if (manager->numClients >= MAX_CONNECTIONS) {
        return 0; // Cannot add more clients, reached the limit
    }
    // find the first unused clients
    int i = 0;
    for ( int i = 0; i < MAX_CONNECTIONS; i++ ) {
      if (!manager->clients[i].isValid) break;
    }

    // Initialize the new client's connection state
    manager->clients[i].clientMac = clientID;
    manager->clients[i].sequenceNumber = 0;
    manager->clients[i].ackNumber = 0;
    manager->clients[i].isValid = true;

    // Increment the number of clients
    manager->numClients++;

    return 1; // Client added successfully
}

// Retrieve a client's connection by client ID
ClientConnection* ConnectionManager_GetClient(ConnectionManager* manager, int clientID) {
    for (int i = 0; i < manager->numClients; i++) {
        if (manager->clients[i].clientMac == clientID) {
            return &manager->clients[i];
        }
    }
    return NULL; // Client not found
}

// Update a client's connection state , when get a received packet
void ConnectionManager_UpdateAckNumber(ConnectionManager* manager, int clientID, int newAckNumber) {
    ClientConnection* client = ConnectionManager_GetClient(manager, clientID);
    if (client != NULL) {
        client->ackNumber = newAckNumber;
    }
}

// Check if ack number equals seq number for a given client
int ConnectionManager_CheckAckEqualsSeq(ConnectionManager* manager, int clientID) {
    ClientConnection* client = ConnectionManager_GetClient(manager, clientID);
    if (client != NULL) {
        return client->ackNumber == client->sequenceNumber;
    }
    return 0; // Client not found or invalid
}

// Invalidate a client's connection
int ConnectionManager_InvalidateClient(ConnectionManager* manager, int clientID) {
    ClientConnection* client = ConnectionManager_GetClient(manager, clientID);
    if (client != NULL) {
      client->isValid = 0;
      client->sequenceNumber = 0;
      client->ackNumber = 0;
      client->clientMac = 0;
      client->last_transmit_time = 0;
      client->last_receive_time = 0;
      client->retransmit_count = 0;
      return 1; // Client invalidated successfully
    }
    return 0; // Client not found
}
