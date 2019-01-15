
#include "Network.h"

#include <stdio.h>
#include <iphlpapi.h>
#include <Ws2tcpip.h>

#pragma warning(push)
#pragma warning(disable : 4456)

CNetwork::CNetwork()
{
    mhSocket             = INVALID_SOCKET;
    mhUDPSocket          = INVALID_SOCKET;
    mhUDPBroadcastSocket = INVALID_SOCKET;
    mnLastError          = 0;
    maErrorStr[0]        = 0;

    InitWinsock();
}


CNetwork::~CNetwork()
{
    WSACleanup();
}


bool CNetwork::InitWinsock()
{
    WORD    wVersionRequested = MAKEWORD(2,2);
    WSADATA wsaData;

    // Initialize WinSock and check version

    if (WSAStartup(wVersionRequested, &wsaData) != 0)
    {
        SetErrorString();
        return false;
    }
    if (wsaData.wVersion != wVersionRequested)
    {	
        SetErrorString();
        return false;
    }
    return true;
} // InitWinsock


bool CNetwork::Connect(const char* pServerAddr, unsigned short nPort)
{
    mnLastError   = 0;
    maErrorStr[0] = 0;

    // Connect to QTM RT server.

    mhSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in sAddr;

    // First check if the address is a dotted number "A.B.C.D"

    if (inet_pton(AF_INET, pServerAddr, &(sAddr.sin_addr)) == NULL)
    {
        // If it wasn't a dotted number lookup the server name

        struct addrinfo hints, *servinfo;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(pServerAddr, NULL, &hints, &servinfo) != 0)
        {
            sprintf_s(maErrorStr, sizeof(maErrorStr), "Error looking up host name.");
            closesocket(mhSocket);
            return false;
        }
        if (servinfo == NULL)
        {
            sprintf_s(maErrorStr, sizeof(maErrorStr), "Error looking up host name.");
            closesocket(mhSocket);
            return false;
        }
        sAddr.sin_addr = ((sockaddr_in *)servinfo[0].ai_addr)->sin_addr;
    }
    sAddr.sin_port = htons(nPort);
    sAddr.sin_family = AF_INET;


    if (connect(mhSocket, (sockaddr*)(&sAddr), sizeof(sAddr)) == SOCKET_ERROR)
    {
        SetErrorString();
        closesocket(mhSocket);
        return false;
    }

    // Disable Nagle's algorithm

    char bNoDelay = 1;

    if (setsockopt(mhSocket, IPPROTO_TCP, TCP_NODELAY, &bNoDelay, sizeof(bNoDelay)))
    {
        SetErrorString();

        closesocket(mhSocket);
        return false;
    }

    return true;
} // Connect


void CNetwork::Disconnect()
{
    // Try to shutdown gracefully

    shutdown(mhSocket, SD_SEND);
    closesocket(mhSocket);
    closesocket(mhUDPSocket);
    closesocket(mhUDPBroadcastSocket);
    mhSocket             = INVALID_SOCKET;
    mhUDPSocket          = INVALID_SOCKET;
    mhUDPBroadcastSocket = INVALID_SOCKET;
} // Disconnect


bool CNetwork::Connected()
{
    return mhSocket != INVALID_SOCKET;
}

bool CNetwork::CreateUDPSocket(unsigned short &nUDPPort, bool bBroadcast)
{
    if (nUDPPort == 0 || nUDPPort > 1023)
    {
        SOCKET tempSocket = INVALID_SOCKET;

        // Create UDP socket for data streaming
        sockaddr_in RecvAddr;
        RecvAddr.sin_family = AF_INET;
        RecvAddr.sin_port = htons(nUDPPort);
        RecvAddr.sin_addr.s_addr = INADDR_ANY;

        tempSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (tempSocket != INVALID_SOCKET)
        {
            u_long argp = 1;
            // Make socket unblocking.
            if (ioctlsocket(tempSocket, FIONBIO , &argp) == 0)
            {
                if (bind(tempSocket, (SOCKADDR *) &RecvAddr, sizeof(RecvAddr)) != -1)
                {
                    nUDPPort = GetUdpServerPort(tempSocket);

                    if (bBroadcast)
                    {
                        char nBroadcast = 1;
                        if (setsockopt(tempSocket, SOL_SOCKET, SO_BROADCAST, &nBroadcast,
                                       sizeof(nBroadcast)) == 0)
                        {
                            mhUDPBroadcastSocket = tempSocket;
                            return true;
                        }
                        else
                        {
                            sprintf_s(maErrorStr, sizeof(maErrorStr), "Failed to set socket options for UDP server socket."); 
                        }
                    }
                    else
                    {
                        mhUDPSocket = tempSocket;
                        return true;
                    }
                }
                else
                {
                    sprintf_s(maErrorStr, sizeof(maErrorStr), "Failed to bind UDP server socket."); 
                }
            }
            else
            {
                sprintf_s(maErrorStr, sizeof(maErrorStr), "Failed to make UDP server socket unblocking."); 
            }
        }
        else
        {
            sprintf_s(maErrorStr, sizeof(maErrorStr), "Failed to create UDP server socket."); 
        }
        closesocket(tempSocket);
    }

    return false;
} // CreateUDPSocket

unsigned short CNetwork::GetUdpServerPort(SOCKET nSocket)
{
    sockaddr_in RecvAddr;
    int addrlen = sizeof(RecvAddr);
    if (getsockname(nSocket, (struct sockaddr *)&RecvAddr, &addrlen) == 0 &&
        RecvAddr.sin_family == AF_INET &&
        addrlen == sizeof(RecvAddr))
    {
        return ntohs(RecvAddr.sin_port);
    }
    return 0;
}

unsigned short CNetwork::GetUdpServerPort()
{
    return GetUdpServerPort(mhUDPSocket);
}

unsigned short CNetwork::GetUdpBroadcastServerPort()
{
    return GetUdpServerPort(mhUDPBroadcastSocket);
}

// Receive a data packet. Data is stored in a local static buffer
// Returns number of bytes in received message, 0 on timeout or -1 if there is an error. 
int CNetwork::Receive(char* rtDataBuff, int nDataBufSize, bool bHeader, int nTimeout, unsigned int *ipAddr)
{
    int         nRecved = 0;
    sockaddr_in source_addr;
    int         fromlen = sizeof(source_addr);

    fd_set ReadFDs, WriteFDs, ExceptFDs;
    FD_ZERO(&ReadFDs);
    FD_ZERO(&WriteFDs);
    FD_ZERO(&ExceptFDs);

    if (mhSocket != INVALID_SOCKET)
    {
        FD_SET(mhSocket, &ReadFDs);
        FD_SET(mhSocket, &ExceptFDs);
    }
    if (mhUDPSocket != INVALID_SOCKET)
    {
        FD_SET(mhUDPSocket, &ReadFDs);
        FD_SET(mhUDPSocket, &ExceptFDs);
    }
    if (mhUDPBroadcastSocket != INVALID_SOCKET)
    {
        FD_SET(mhUDPBroadcastSocket, &ReadFDs);
        FD_SET(mhUDPBroadcastSocket, &ExceptFDs);
    }

    TIMEVAL* pTimeout;
    TIMEVAL  timeout;

    if (nTimeout < 0)
    {
        pTimeout = NULL;
    }
    else
    {
        timeout.tv_sec  = nTimeout / 1000000;
        timeout.tv_usec = nTimeout % 1000000;
        pTimeout = &timeout;
    }

    // Wait for activity on the TCP and UDP sockets.
    int nSelectRes = select(0, &ReadFDs, &WriteFDs, &ExceptFDs, pTimeout);
    
    if (nSelectRes == 0)
    {
        return 0; // Select timeout.
    }
    if (nSelectRes > 0)
    {
        if (FD_ISSET(mhSocket, &ExceptFDs))
        {
            // General socket error
            FD_CLR(mhSocket, &ExceptFDs);
            SetErrorString();
            nRecved = SOCKET_ERROR;
        }
        else if (FD_ISSET(mhSocket, &ReadFDs))
        {
            nRecved = recv(mhSocket, rtDataBuff, bHeader ? 8 : nDataBufSize, 0);
            FD_CLR(mhSocket, &ReadFDs);
        }
        else if (FD_ISSET(mhUDPSocket, &ExceptFDs))
        {
            // General socket error
            FD_CLR(mhUDPSocket, &ExceptFDs);
            SetErrorString();
            nRecved = SOCKET_ERROR;
        }
        else if (FD_ISSET(mhUDPSocket, &ReadFDs))
        {
            nRecved = recvfrom(mhUDPSocket, rtDataBuff, nDataBufSize, 0, (sockaddr*)&source_addr, &fromlen);
            FD_CLR(mhUDPSocket, &ReadFDs);
        }
        else if (FD_ISSET(mhUDPBroadcastSocket, &ExceptFDs))
        {
            // General socket error
            FD_CLR(mhUDPBroadcastSocket, &ExceptFDs);
            SetErrorString();
            nRecved = SOCKET_ERROR;
        }
        else if (FD_ISSET(mhUDPBroadcastSocket, &ReadFDs))
        {
            sockaddr_in source_addr;

            nRecved = recvfrom(mhUDPBroadcastSocket, rtDataBuff, nDataBufSize, 0, (sockaddr*)&source_addr, &fromlen);
            FD_CLR(mhUDPBroadcastSocket, &ReadFDs);
            if (ipAddr)
            {
                *ipAddr = source_addr.sin_addr.s_addr;
            }
        }
    }
    else
    {
        nRecved = -1;
    }

    if (nRecved == -1)
    {
        SetErrorString();
        Disconnect();
    }
    
    return nRecved;
} // RecvMessage


bool CNetwork::Send(const char* pSendBuf, int nSize)
{
    int         nSent      = 0;
    int         nTotSent   = 0;

    while (nTotSent < nSize)
    {
        nSent = send(mhSocket, pSendBuf + nTotSent, nSize - nTotSent, 0);
        if (nSent == SOCKET_ERROR)
        {
            SetErrorString();
            return false;
        }
        nTotSent += nSent;
    }

    return true;
} // Send


bool CNetwork::SendUDPBroadcast(const char* pSendBuf, int nSize, short nPort, unsigned int nFilterAddr /* = 0 */ )
{
    bool bBroadCastSent = false;

    if (mhUDPBroadcastSocket != INVALID_SOCKET)
    {
        IP_ADAPTER_INFO* pAdptInfo  = NULL;
        IP_ADAPTER_INFO* pNextAd    = NULL;
        ULONG ulLen                 = 0;
        DWORD erradapt;
        
        // Find all network interfaces.
        erradapt = ::GetAdaptersInfo(pAdptInfo, &ulLen);
        if (erradapt == ERROR_BUFFER_OVERFLOW)
        {
            pAdptInfo = (IP_ADAPTER_INFO*)malloc(ulLen);
            erradapt = ::GetAdaptersInfo(pAdptInfo, &ulLen);      
        }

        if (erradapt == ERROR_SUCCESS)
        {
            sockaddr_in recvAddr;
            recvAddr.sin_family      = AF_INET;
            recvAddr.sin_port        = htons(nPort);
            recvAddr.sin_addr.s_addr = 0xffffffff;

            // Send broadcast on all Ethernet interfaces.
            pNextAd = pAdptInfo;
            while( pNextAd )
            {
                if (pNextAd->Type == MIB_IF_TYPE_ETHERNET)
                {
                    unsigned int nIPaddr;
                    unsigned int nIPmask;
                    
                    if (inet_pton(AF_INET, pNextAd->IpAddressList.IpAddress.String, &nIPaddr) == NULL ||
                        inet_pton(AF_INET, pNextAd->IpAddressList.IpMask.String, &nIPmask) == NULL)
                    {
                        return false;
                    }
                    recvAddr.sin_addr.s_addr = nIPaddr | (~nIPmask);
                    if (recvAddr.sin_addr.s_addr != (nFilterAddr | (~nIPmask)))
                    {
                        if (sendto(mhUDPBroadcastSocket, pSendBuf, nSize, 0, (sockaddr*)&recvAddr, sizeof(recvAddr)) == nSize)
                        {
                            bBroadCastSent = true;
                        }
                    }
                }
                pNextAd = pNextAd->Next;
            }
        }
        free(pAdptInfo);      
    }

    return bBroadCastSent;
} // SendUDPBroadcast


void CNetwork::SetErrorString()
{ 
    char *tError = NULL; 
    mnLastError  = GetLastError(); 
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
                  NULL, mnLastError, 0, reinterpret_cast<LPTSTR>(&tError), 0, NULL); 

    sprintf_s(maErrorStr, sizeof(maErrorStr), "%s", tError); 

    LocalFree(tError);
}


char* CNetwork::GetErrorString()
{
    return maErrorStr;
}


int CNetwork::GetError() const
{
    return mnLastError;
}


bool CNetwork::IsLocalAddress(unsigned int nAddr) const
{
    IP_ADAPTER_INFO* pAdptInfo  = NULL;
    IP_ADAPTER_INFO* pNextAd    = NULL;
    DWORD            erradapt;
    ULONG            ulLen      = 0;

    // Find all network interfaces.
    erradapt = ::GetAdaptersInfo(pAdptInfo, &ulLen);
    if (erradapt == ERROR_BUFFER_OVERFLOW)
    {
        pAdptInfo = (IP_ADAPTER_INFO*)malloc(ulLen);
        erradapt = ::GetAdaptersInfo(pAdptInfo, &ulLen);      
    }

    if (erradapt == ERROR_SUCCESS)
    {
        pNextAd = pAdptInfo;
        while( pNextAd )
        {
            if (pNextAd->Type == MIB_IF_TYPE_ETHERNET)
            {
                // Check if it's a response from a local interface.
                unsigned int addr;
                if (inet_pton(AF_INET, pNextAd->IpAddressList.IpAddress.String, &addr) != NULL)
                {
                    return addr == nAddr;
                }
            }
            pNextAd = pNextAd->Next;
        }
    }
    free(pAdptInfo);

    return false;
}

#pragma warning( pop ) 