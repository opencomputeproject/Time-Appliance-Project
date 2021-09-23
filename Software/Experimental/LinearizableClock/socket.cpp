/*
 *   Copyright (C) 2002
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "socket.h"

#include <arpa/inet.h>  // For inet_addr()
#include <netdb.h>      // For gethostbyname()
#include <netinet/in.h> // For sockaddr_in
#include <sys/socket.h> // For socket(), connect(), send(), and recv()
#include <sys/types.h>  // For data types
#include <unistd.h>     // For close()
typedef void raw_type;  // Type used for raw data on this platform

#include <errno.h> // For errno

// SocketException Code

SocketException::SocketException(const string &message, bool inclSysMsg)
    : userMessage(message) {
  if (inclSysMsg) {
    userMessage.append(": ");
    userMessage.append(strerror(errno));
  }
}

SocketException::~SocketException() {}

const char *SocketException::what() const noexcept {
  return userMessage.c_str();
}

// Function to fill in address structure given an address and port
static void fillAddr(const string &address, unsigned short port,
                     sockaddr_in &addr) {
  memset(&addr, 0, sizeof(addr)); // Zero out address structure
  addr.sin_family = AF_INET;      // Internet address

  hostent *host; // Resolve name
  if ((host = gethostbyname(address.c_str())) == NULL) {
    // strerror() will not work for gethostbyname() and hstrerror()
    // is supposedly obsolete
    throw SocketException("Failed to resolve name (gethostbyname())");
  }
  addr.sin_addr.s_addr = *((unsigned long *)host->h_addr_list[0]);

  addr.sin_port = htons(port); // Assign port in network byte order
}

uint32_t GetAddrFamily() { return (IPMODE == IPv6) ? PF_INET6 : PF_INET; }

bool IsIPv4() { return (IPMODE == IPv4); }

Address::Address(unsigned char protocol_version, const std::string &host,
                 unsigned short int port) {
  addrinfo hints;
  addrinfo *addr;
  char port_str[6];

  memset(&hints, 0, sizeof(hints));

  hints.ai_family = GetAddrFamily();
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = 0;
  hints.ai_protocol = IPPROTO_UDP;
  snprintf(port_str, 6, "%d", port);

  if (getaddrinfo(host.c_str(), port_str, &hints, &addr) != 0) {
    std::string ext_error = get_error_message();
    throw SocketException("getaddrinfo() failed. " + ext_error);
  }

  this->paddr_len = addr->ai_addrlen;
  this->paddr = (sockaddr *)new char[addr->ai_addrlen];
  memcpy(this->paddr, addr->ai_addr, addr->ai_addrlen);
  freeaddrinfo(addr);
}

// Socket Code

Socket::Socket(int type, int protocol) {
  // Make a new socket
  if ((sockDesc = socket(GetAddrFamily(), type, protocol)) < 0) {
    throw SocketException("Socket creation failed (socket())", true);
  }
}

Socket::Socket(int sockDesc) { this->sockDesc = sockDesc; }

Socket::~Socket() {
  ::close(sockDesc);
  sockDesc = -1;
}

string Socket::getLocalAddress() const {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);

  if (getsockname(sockDesc, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0) {
    throw SocketException("Fetch of local address failed (getsockname())",
                          true);
  }
  return inet_ntoa(addr.sin_addr);
}

unsigned short Socket::getLocalPort() const {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);

  if (getsockname(sockDesc, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0) {
    throw SocketException("Fetch of local port failed (getsockname())", true);
  }
  return ntohs(addr.sin_port);
}

void Socket::setLocalPort(unsigned short localPort) {
  if (IsIPv4()) {
    // Bind the socket to its port
    sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = GetAddrFamily();
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(localPort);

    if (::bind(sockDesc, (sockaddr *)&localAddr, sizeof(sockaddr_in)) < 0) {
      throw SocketException("Set of local port failed (bind())", true);
    }
  } else {
    sockaddr_in6 localAddr;
    localAddr.sin6_family = GetAddrFamily();
    localAddr.sin6_port = htons(localPort);
    localAddr.sin6_addr = in6addr_any;
    if (::bind(sockDesc, (sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
      throw SocketException("Set of local port failed (bind())", true);
    }
  }
}

void Socket::setLocalAddressAndPort(const string &localAddress,
                                    unsigned short localPort) {
  // Get the address of the requested host
  sockaddr_in localAddr;
  fillAddr(localAddress, localPort, localAddr);

  if (::bind(sockDesc, (sockaddr *)&localAddr, sizeof(sockaddr_in)) < 0) {
    throw SocketException("Set of local address and port failed (bind())",
                          true);
  }
}

void Socket::cleanUp() {}

unsigned short Socket::resolveService(const string &service,
                                      const string &protocol) {
  struct servent *serv; /* Structure containing service information */

  if ((serv = getservbyname(service.c_str(), protocol.c_str())) == NULL)
    return atoi(service.c_str()); /* Service is port number */
  else
    return ntohs(serv->s_port); /* Found port (network byte order) by name */
}

// CommunicatingSocket Code

CommunicatingSocket::CommunicatingSocket(int type, int protocol)
    : Socket(type, protocol) {}

CommunicatingSocket::CommunicatingSocket(int newConnSD) : Socket(newConnSD) {}

void CommunicatingSocket::connect(const string &foreignAddress,
                                  unsigned short foreignPort) {
  // Get the address of the requested host
  sockaddr_in destAddr;
  fillAddr(foreignAddress, foreignPort, destAddr);

  // Try to connect to the given port
  if (::connect(sockDesc, (sockaddr *)&destAddr, sizeof(destAddr)) < 0) {
    throw SocketException("Connect failed (connect())", true);
  }
}

void CommunicatingSocket::send(const void *buffer, int bufferLen) {
  if (::send(sockDesc, (raw_type *)buffer, bufferLen, 0) < 0) {
    throw SocketException("Send failed (send())", true);
  }
}

int CommunicatingSocket::recv(void *buffer, int bufferLen) {
  int rtn;
  if ((rtn = ::recv(sockDesc, (raw_type *)buffer, bufferLen, 0)) < 0) {
    throw SocketException("Received failed (recv())", true);
  }

  return rtn;
}

string CommunicatingSocket::getForeignAddress() {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);

  if (getpeername(sockDesc, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0) {
    throw SocketException("Fetch of foreign address failed (getpeername())",
                          true);
  }
  return inet_ntoa(addr.sin_addr);
}

unsigned short CommunicatingSocket::getForeignPort() {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);

  if (getpeername(sockDesc, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0) {
    throw SocketException("Fetch of foreign port failed (getpeername())", true);
  }
  return ntohs(addr.sin_port);
}

// TCPSocket Code

TCPSocket::TCPSocket() : CommunicatingSocket(SOCK_STREAM, IPPROTO_TCP) {}

TCPSocket::TCPSocket(const string &foreignAddress, unsigned short foreignPort)
    : CommunicatingSocket(SOCK_STREAM, IPPROTO_TCP) {
  connect(foreignAddress, foreignPort);
}

TCPSocket::TCPSocket(int newConnSD) : CommunicatingSocket(newConnSD) {}

// TCPServerSocket Code

TCPServerSocket::TCPServerSocket(unsigned short localPort, int queueLen)
    : Socket(SOCK_STREAM, IPPROTO_TCP) {
  setLocalPort(localPort);
  setListen(queueLen);
}

TCPServerSocket::TCPServerSocket(const string &localAddress,
                                 unsigned short localPort, int queueLen)
    : Socket(SOCK_STREAM, IPPROTO_TCP) {
  setLocalAddressAndPort(localAddress, localPort);
  setListen(queueLen);
}

TCPSocket *TCPServerSocket::accept() {
  int newConnSD;
  if ((newConnSD = ::accept(sockDesc, NULL, 0)) < 0) {
    throw SocketException("Accept failed (accept())", true);
  }

  return new TCPSocket(newConnSD);
}

void TCPServerSocket::setListen(int queueLen) {
  if (listen(sockDesc, queueLen) < 0) {
    throw SocketException("Set listening socket failed (listen())", true);
  }
}

// UDPSocket Code

UDPSocket::UDPSocket() : CommunicatingSocket(SOCK_DGRAM, IPPROTO_UDP) {
  setBroadcast();
}

UDPSocket::UDPSocket(unsigned short localPort)
    : CommunicatingSocket(SOCK_DGRAM, IPPROTO_UDP) {
  setLocalPort(localPort);
  setBroadcast();
}

UDPSocket::UDPSocket(const string &localAddress, unsigned short localPort)
    : CommunicatingSocket(SOCK_DGRAM, IPPROTO_UDP) {
  setLocalAddressAndPort(localAddress, localPort);
  setBroadcast();
}

void UDPSocket::setBroadcast() {
  // If this fails, we'll hear about it when we try to send.  This will allow
  // system that cannot broadcast to continue if they don't plan to broadcast
  int broadcastPermission = 1;
  setsockopt(sockDesc, SOL_SOCKET, SO_BROADCAST,
             (raw_type *)&broadcastPermission, sizeof(broadcastPermission));
}

void UDPSocket::disconnect() {
  sockaddr_in nullAddr;
  memset(&nullAddr, 0, sizeof(nullAddr));
  nullAddr.sin_family = AF_UNSPEC;

  // Try to disconnect
  if (::connect(sockDesc, (sockaddr *)&nullAddr, sizeof(nullAddr)) < 0) {
    if (errno != EAFNOSUPPORT) {
      throw SocketException("Disconnect failed (connect())", true);
    }
  }
}

void UDPSocket::sendTo(const void *buffer, int bufferLen, const Address &addr) {
  // Write out the whole buffer as a single message.
  if (sendto(sockDesc, (raw_type *)buffer, bufferLen, 0, addr.paddr,
             addr.paddr_len) != bufferLen) {
    throw SocketException("Send failed (sendto())", true);
  }
}

int UDPSocket::recvFrom(void *buffer, int bufferLen, string &sourceAddress,
                        unsigned short &sourcePort) {
  if (IsIPv4()) {
    sockaddr_in clntAddr;
    socklen_t addrLen = sizeof(clntAddr);
    int rtn;
    if ((rtn = recvfrom(sockDesc, (raw_type *)buffer, bufferLen, 0,
                        (sockaddr *)&clntAddr, (socklen_t *)&addrLen)) < 0) {
      throw SocketException("Receive failed (recvfrom())", true);
    }
    sourceAddress = inet_ntoa(clntAddr.sin_addr);
    sourcePort = ntohs(clntAddr.sin_port);

    return rtn;
  } else {
    sockaddr_in6 clntAddr;
    socklen_t addrLen = sizeof(clntAddr);
    int rtn;
    if ((rtn = recvfrom(sockDesc, (raw_type *)buffer, bufferLen, 0,
                        (sockaddr *)&clntAddr, (socklen_t *)&addrLen)) < 0) {
      throw SocketException("Receive failed (recvfrom())", true);
    }
    char buffer[256];
    inet_ntop(GetAddrFamily(), &(clntAddr.sin6_addr), buffer, 256);
    sourceAddress = std::string(buffer);
    sourcePort = ntohs(clntAddr.sin6_port);

    return rtn;
  }
}

void UDPSocket::setMulticastTTL(unsigned char multicastTTL) {
  if (setsockopt(sockDesc, IPPROTO_IP, IP_MULTICAST_TTL,
                 (raw_type *)&multicastTTL, sizeof(multicastTTL)) < 0) {
    throw SocketException("Multicast TTL set failed (setsockopt())", true);
  }
}

void UDPSocket::joinGroup(const string &multicastGroup) {
  struct ip_mreq multicastRequest;

  multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup.c_str());
  multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(sockDesc, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                 (raw_type *)&multicastRequest, sizeof(multicastRequest)) < 0) {
    throw SocketException("Multicast group join failed (setsockopt())", true);
  }
}

void UDPSocket::leaveGroup(const string &multicastGroup) {
  struct ip_mreq multicastRequest;

  multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup.c_str());
  multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(sockDesc, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                 (raw_type *)&multicastRequest, sizeof(multicastRequest)) < 0) {
    throw SocketException("Multicast group leave failed (setsockopt())", true);
  }
}
