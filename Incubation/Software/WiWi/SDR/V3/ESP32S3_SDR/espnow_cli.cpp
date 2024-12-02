


#include "espnow_cli.h"


void printPacketInfo(const ReceivedPacket *packet) {
  if (packet == nullptr) {
      SERIAL_PRINTLN("No packet to print.");
      return;
  }

  // Print basic metadata
  sprintf(print_buffer,"Received data from %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                packet->mac[0], packet->mac[1], packet->mac[2],
                packet->mac[3], packet->mac[4], packet->mac[5]);
  SERIAL_PRINT(print_buffer);
  sprintf(print_buffer, "RSSI: %d, Channel: %d, Data Length: %d bytes\r\n",
                packet->rssi, packet->channel, (int)packet->data_len);
  SERIAL_PRINT(print_buffer);

  // Print raw data in human-readable ASCII format
  SERIAL_PRINT("Data (ASCII): ");
  for (size_t i = 0; i < packet->data_len; i++) {
      char c = (char)packet->data[i];
      if (isprint(c)) {
          SERIAL_PRINT(c);
      } else {
          SERIAL_PRINT('.');
      }
  }
  SERIAL_PRINTLN();

  // Print raw data in hex format
  SERIAL_PRINT("Data (Hex): ");
  for (size_t i = 0; i < packet->data_len; i++) {
    sprintf(print_buffer, "0x%02X ", packet->data[i]);
    SERIAL_PRINT(print_buffer);
  }
  SERIAL_PRINTLN();
}


// Peer APIs
void espnow_cli_addPeer(EmbeddedCli *cli, char *args, void *context) {
    if (embeddedCliGetTokenCount(args) != 3) {
      SERIAL_PRINTLN("Usage: addPeer <MAC_ADDRESS> <CHANNEL>");
      return;
    }

    // Extract arguments
    const char *macStr = embeddedCliGetToken(args, 1);
    const char *channelStr = embeddedCliGetToken(args, 2);

    // Parse MAC address
    uint8_t mac[6];
    if (sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
      SERIAL_PRINTLN("Invalid MAC address format. Use: XX:XX:XX:XX:XX:XX");
      return;
    }

    // Parse channel
    uint8_t channel = atoi(channelStr);
    if (channel < 1 || channel > 13) {
      SERIAL_PRINTLN("Invalid channel. Use a value between 1 and 13.");
      return;
    }

    // Add the peer
    if (addPeer(mac, channel)) {
      sprintf(print_buffer,"Peer added: MAC=%s, Channel=%d\r\n", macStr, channel);
      SERIAL_PRINT(print_buffer);
    } else {
      SERIAL_PRINTLN("Failed to add peer. Ensure the peer list is not full.");
    }
}


void espnow_cli_removePeer(EmbeddedCli *cli, char *args, void *context)
{
  if (embeddedCliGetTokenCount(args) != 2) {
    SERIAL_PRINTLN("Usage: removePeer <MAC_ADDRESS>");
    return;
  }

  // Extract arguments
  const char *macStr = embeddedCliGetToken(args, 1);

  // Parse MAC address
  uint8_t mac[6];
  if (sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
              &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
    SERIAL_PRINTLN("Invalid MAC address format. Use: XX:XX:XX:XX:XX:XX");
    return;
  }

  // Remove the peer
  if (removePeer(mac)) {
    sprintf(print_buffer,"Peer removed: MAC=%s\r\n", macStr);
    SERIAL_PRINT(print_buffer);
  } else {
    SERIAL_PRINTLN("Failed to remove peer. Ensure the MAC address exists in the peer list.");
  }
}


void espnow_cli_listPeers(EmbeddedCli *cli, char *args, void *context)
{
  SERIAL_PRINTLN("List of Active Peers:");
  for (int i = 0; i < MAX_PEERS; i++) {
      if (peers[i].active) {
          sprintf(print_buffer,"Peer %d: MAC=%02X:%02X:%02X:%02X:%02X:%02X, Channel=%d\r\n", i + 1,
                        peers[i].mac[0], peers[i].mac[1], peers[i].mac[2],
                        peers[i].mac[3], peers[i].mac[4], peers[i].mac[5],
                        peers[i].channel);
          SERIAL_PRINT(print_buffer);
      }
  }
}

// Channel APIs
void espnow_cli_Channel(EmbeddedCli *cli, char *args, void *context)
{
  int tokenCount = embeddedCliGetTokenCount(args);

  if (tokenCount == 1) {
      // No arguments: print current channel
      uint8_t currentChannel = getChannel();
      Serial.printf("Current Channel: %d\n", currentChannel);
  } else if (tokenCount == 2) {
      // One argument: set the channel
      const char *channelStr = embeddedCliGetToken(args, 1);
      uint8_t channel = atoi(channelStr);
      if (channel < 1 || channel > 13) {
          SERIAL_PRINTLN("Invalid channel. Use a value between 1 and 13.");
          return;
      }
      if (setChannel(channel)) {
          Serial.printf("Channel set to: %d\n", channel);
      } else {
          SERIAL_PRINTLN("Failed to set channel.");
      }
  } else {
      // Invalid usage
      SERIAL_PRINTLN("Usage: Channel [<CHANNEL>]");
  }
}

// Data transmit
void espnow_cli_sendDataHuman(EmbeddedCli *cli, char *args, void *context) {
  int tokenCount = embeddedCliGetTokenCount(args);
  if (tokenCount < 3) {
      SERIAL_PRINTLN("Usage: sendDataHuman <MAC_ADDRESS> <HEX_DATA...>");
      return;
  }

  // Extract the MAC address
  const char *macStr = embeddedCliGetToken(args, 1);

  // Parse the MAC address
  uint8_t mac[6];
  if (sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
              &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
    SERIAL_PRINTLN("Invalid MAC address format. Use: XX:XX:XX:XX:XX:XX");
    return;
  }

  // Parse the hex data
  uint8_t data[250];
  size_t dataLength = 0;
  for (int i = 2; i < tokenCount; i++) {
    const char *hexStr = embeddedCliGetToken(args, i);
    int byte;
    if (sscanf(hexStr, "0x%x", &byte) != 1) {
      sprintf(print_buffer, "Invalid hex byte: %s\r\n", hexStr);
      SERIAL_PRINT(print_buffer);
      return;
    }
    if (dataLength >= sizeof(data)) {
      SERIAL_PRINTLN("Error: Too many bytes. Maximum is 250.");
      return;
    }
    data[dataLength++] = (uint8_t)byte;
  }

  // Send the data
  if (sendData(mac, data, dataLength)) {
    sprintf(print_buffer, "Data sent to MAC=%s: ", macStr); SERIAL_PRINT(print_buffer);
    for (size_t i = 0; i < dataLength; i++) {
      sprintf(print_buffer, "0x%02X ", data[i]);
      SERIAL_PRINT(print_buffer);
    }
    SERIAL_PRINTLN();
  } else {
    SERIAL_PRINTLN("Failed to send data.");
  }
}

void espnow_cli_sendDataMachine(EmbeddedCli *cli, char *args, void *context) {
    // Ensure the input is not null
    if (!args) {
      SERIAL_PRINTLN("Error: No arguments provided.");
      return;
    }

    // Parse MAC address (first 17 characters for XX:XX:XX:XX:XX:XX)
    uint8_t mac[6];
    if (sscanf(args, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
      SERIAL_PRINTLN("Invalid MAC address format. Use: XX:XX:XX:XX:XX:XX");
      return;
    }

    // Find the byte count and data start after the MAC address
    char *byteCountStart = args + 17; // Skip the MAC address (17 characters: XX:XX:XX:XX:XX:XX)
    while (*byteCountStart == ' ') byteCountStart++; // Skip spaces

    // Parse byte count
    int byteCount = atoi(byteCountStart);
    if (byteCount <= 0 || byteCount > 250) {
      SERIAL_PRINTLN("Invalid byte count. Must be between 1 and 250.");
      return;
    }

    // Find the start of the raw data (skip past the byte count)
    char *dataStart = byteCountStart;
    while (*dataStart && *dataStart != ' ') dataStart++; // Move past the byte count
    while (*dataStart == ' ') dataStart++; // Skip spaces to reach the data

    // Ensure there is enough data
    size_t inputDataLength = strlen(dataStart);
    if (inputDataLength < byteCount) {
        SERIAL_PRINTLN("Insufficient data provided for the byte count.");
        return;
    }

    // Send the data
    if (sendData(mac, (uint8_t *)dataStart, byteCount)) {
      sprintf(print_buffer, "Raw data sent to MAC=%02X:%02X:%02X:%02X:%02X:%02X\r\n",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      SERIAL_PRINT(print_buffer);
    } else {
      SERIAL_PRINTLN("Failed to send raw data.");
    }
}




// Data reception
void espnow_cli_receiveData(EmbeddedCli *cli, char *args, void *context)
{
  if (!hasReceivedData()) {
      SERIAL_PRINTLN("No received data available.");
      return;
  }

  // Process all received packets in the buffer
  while (hasReceivedData()) {
      // Pop the next packet from the ring buffer
      ReceivedPacket *packet = popReceivedData();

      // Print the packet information
      printPacketInfo(packet);
  }
}

static Node espnow_addPeer_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {"addPeer",
    "Add an ESP NOW peer MAC address",
    true,
    nullptr,
    espnow_cli_addPeer}
};

static Node espnow_removePeer_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {"removePeer",
    "Remove an ESP NOW peer MAC address",
    true,
    nullptr,
    espnow_cli_removePeer}
};

static Node espnow_listPeer_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {"listPeers",
    "List current ESP NOW Peer MAC addresses",
    true,
    nullptr,
    espnow_cli_listPeers}
};

static Node espnow_channel_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {"channel",
    "List current channel with no arguments, set channel with one argument",
    true,
    nullptr,
    espnow_cli_Channel}
};
static Node espnow_sendDataHuman_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {"sendDataHuman",
    "send ESP NOW data, first argument is peer MAC, followed by hex bytes of data to send",
    true, 
    nullptr,
    espnow_cli_sendDataHuman}
};
static Node espnow_sendDataMachine_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {"sendDataMachine",
    "send ESP NOW data, first argument is peer MAC, second argument is number of bytes, followed by that number of bytes to send",
    false, // make this one special, because it allows for raw bytes 
    nullptr,
    espnow_cli_sendDataMachine}
};

static Node espnow_receiveData_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {"receiveData",
    "Receive one packet of data from buffer",
    true, 
    nullptr,
    espnow_cli_receiveData}
};

// Define all files

static Node * espnow_files[] = { &espnow_addPeer_node, &espnow_removePeer_node,
  &espnow_listPeer_node, &espnow_channel_node, &espnow_sendDataHuman_node, 
  &espnow_sendDataMachine_node, &espnow_receiveData_node };



void espnow_dir_operation(EmbeddedCli *cli, char *args, void *context);
// Define the SPI directory
static Node espnow_dir = {
    .name = "espnow",
    .type = MY_DIRECTORY,
    .cliBinding = {"espnow",
          "espnow operations",
          true,
          nullptr,
          espnow_dir_operation},
    .parent = 0,
    .children = espnow_files,
    .num_children = sizeof(espnow_files) / sizeof(espnow_files[0])
};








/********************** Boiler plate code for CLI ****************/

void espnow_dir_operation(EmbeddedCli *cli, char *args, void *context)
{
  change_to_node(&espnow_dir);
}

void espnow_init() {
  for (int i = 0; i < espnow_dir.num_children; i++) {
    espnow_files[i]->parent = &espnow_dir;
  }
  add_root_filesystem(&espnow_dir);
}



void init_espnow_cli()
{
  espnow_init();
  init_espnow_code();
}