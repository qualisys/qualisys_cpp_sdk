#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <float.h>
#include <cctype>
#include <thread>
#include <string.h>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <locale>
#include <vector>
#include <iterator>
#include <functional>

#include "RTProtocol.h"
#include "Markup.h"
#include "Network.h"
#include <stdexcept>

#ifdef _WIN32
#include <iphlpapi.h>
// import the internet protocol helper library.
#pragma comment(lib, "IPHLPAPI.lib")
#else
#include <arpa/inet.h>

#endif

namespace
{
    inline void RemoveInvalidChars(std::string& str)
    {
        auto isInvalidChar = [](int c) -> int
        {
            // iscntrl: control codes(NUL, etc.), '\t', '\n', '\v', '\f', '\r', backspace (DEL)
            // isspace: some common checks but also 0x20 (SPACE)
            // return != 0 --> invalid char
            return std::iscntrl(c) + std::isspace(c);
        };
        str.erase(std::remove_if(str.begin(), str.end(), isInvalidChar), str.end());
    }

    const int qtmPacketHeaderSize = 8; // 8 bytes

    static const auto DEGREES_OF_FREEDOM =
    {
        std::make_pair(CRTProtocol::EDegreeOfFreedom::RotationX, "RotationX"),
        std::make_pair(CRTProtocol::EDegreeOfFreedom::RotationY, "RotationY"),
        std::make_pair(CRTProtocol::EDegreeOfFreedom::RotationZ, "RotationZ"),
        std::make_pair(CRTProtocol::EDegreeOfFreedom::TranslationX, "TranslationX"),
        std::make_pair(CRTProtocol::EDegreeOfFreedom::TranslationY, "TranslationY"),
        std::make_pair(CRTProtocol::EDegreeOfFreedom::TranslationZ, "TranslationZ")
    };
}

unsigned int CRTProtocol::GetSystemFrequency() const
{
    return msGeneralSettings.nCaptureFrequency;
}

CRTProtocol::CRTProtocol()
{
    mpoNetwork      = new CNetwork();
    mpoRTPacket     = nullptr;
    meLastEvent     = CRTPacket::EventCaptureStopped;
    meState         = CRTPacket::EventCaptureStopped;
    mnMajorVersion  = 1;
    mnMinorVersion  = 0;
    mbBigEndian     = false;
    maErrorStr[0]   = 0;
    mnBroadcastPort = 0;
    mpFileBuffer    = nullptr;
    mbIsMaster = false;
    mDataBuff.resize(65535);
    mSendBuffer.resize(5000);
} // CRTProtocol


CRTProtocol::~CRTProtocol()
{
    if (mpoNetwork)
    {
        delete mpoNetwork;
        mpoNetwork = nullptr;
    }
    if (mpoRTPacket)
    {
        delete mpoRTPacket;
        mpoRTPacket = nullptr;
    }
} // ~CRTProtocol


bool CRTProtocol::Connect(const char* pServerAddr, unsigned short nPort, unsigned short* pnUDPServerPort,
                          int nMajorVersion, int nMinorVersion, bool bBigEndian)
{
    CRTPacket::EPacketType eType;
    char                   tTemp[100];
    char                   pResponseStr[256];

    mbBigEndian = bBigEndian;
    mbIsMaster = false;

    mnMajorVersion = 1;
    if ((nMajorVersion == 1) && (nMinorVersion == 0))
    {
        mnMinorVersion = 0;
    }
    else
    {
        mnMinorVersion = 1;
        if (mbBigEndian)
        {
            nPort += 2;
        }
        else
        {
            nPort += 1;
        }
    }

    if (mpoRTPacket)
    {
        delete mpoRTPacket;
    }
    mpoRTPacket = new CRTPacket(nMajorVersion, nMinorVersion, bBigEndian);

    if (mpoRTPacket == nullptr)
    {
        strcpy(maErrorStr, "Could not allocate data packet.");
        return false;
    }

    if (mpoNetwork->Connect(pServerAddr, nPort))
    {
        if (pnUDPServerPort != nullptr)
        {
            if (mpoNetwork->CreateUDPSocket(*pnUDPServerPort) == false)
            {
                sprintf(maErrorStr, "CreateUDPSocket failed. %s", mpoNetwork->GetErrorString());
                Disconnect();
                return false;
            }
        }

        // Welcome message
        if (Receive(eType, true) == CNetwork::ResponseType::success)
        {
            if (eType == CRTPacket::PacketError)
            {
                strcpy(maErrorStr, mpoRTPacket->GetErrorString());
                Disconnect();
                return false;
            }
            else if (eType == CRTPacket::PacketCommand)
            {
                const std::string welcomeMessage("QTM RT Interface connected");
                if (strncmp(welcomeMessage.c_str(), mpoRTPacket->GetCommandString(), welcomeMessage.size()) == 0)
                {
                    // Set protocol version
                    if (SetVersion(nMajorVersion, nMinorVersion))
                    {
                        // Set byte order.
                        // Unless we use protocol version 1.0, we have set the byte order by selecting the correct port.

                        if ((mnMajorVersion == 1) && (mnMinorVersion == 0))
                        {
                            if (mbBigEndian)
                            {
                                strcpy(tTemp, "ByteOrder BigEndian");
                            }
                            else
                            {
                                strcpy(tTemp,  "ByteOrder LittleEndian");
                            }

                            if (SendCommand(tTemp, pResponseStr))
                            {
                                return true;
                            }
                            else
                            {
                                strcpy(maErrorStr, "Set byte order failed.");
                            }
                        }
                        else
                        {
                            GetState(meState, true);
                            return true;
                        }
                    }
                    Disconnect();
                    return false;
                }
            }
        }
    }
    else
    {
        if (mpoNetwork->GetError() == 10061)
        {
            strcpy(maErrorStr, "Check if QTM is running on target machine.");
        }
        else
        {
            strcpy(maErrorStr, mpoNetwork->GetErrorString());
        }
    }
    Disconnect();
    return false;
} // Connect


unsigned short CRTProtocol::GetUdpServerPort()
{
    if (mpoNetwork)
    {
        return mpoNetwork->GetUdpServerPort();
    }
    return 0;
}


void CRTProtocol::Disconnect()
{
    mpoNetwork->Disconnect();
    mnBroadcastPort = 0;
    if (mpoRTPacket)
    {
        delete mpoRTPacket;
        mpoRTPacket = nullptr;
    }
    mbIsMaster = false;
} // Disconnect


bool CRTProtocol::Connected() const
{
    return mpoNetwork->Connected();
}


bool CRTProtocol::SetVersion(int nMajorVersion, int nMinorVersion)
{
    char tTemp[256];
    char pResponseStr[256];

    sprintf(tTemp, "Version %u.%u", nMajorVersion, nMinorVersion);

    if (SendCommand(tTemp, pResponseStr))
    {
        sprintf(tTemp, "Version set to %u.%u", nMajorVersion, nMinorVersion);

        if (strcmp(pResponseStr, tTemp) == 0)
        {
            mnMajorVersion = nMajorVersion;
            mnMinorVersion = nMinorVersion;
            mpoRTPacket->SetVersion(mnMajorVersion, mnMinorVersion);
            return true;
        }

        if (pResponseStr)
        {
            sprintf(maErrorStr, "%s.", pResponseStr);
        }
        else
        {
            strcpy(maErrorStr, "Set Version failed.");
        }
    }
    else
    {
        strcpy(tTemp, maErrorStr);
        sprintf(maErrorStr, "Send Version failed. %s.", tTemp);
    }
    return false;
}


bool CRTProtocol::GetVersion(unsigned int &nMajorVersion, unsigned int &nMinorVersion)
{
    if (!Connected())
    {
        return false;
    }

    nMajorVersion = mnMajorVersion;
    nMinorVersion = mnMinorVersion;

    return true;
}


bool CRTProtocol::GetQTMVersion(char* pVersion, unsigned int nVersionLen)
{
    if (SendCommand("QTMVersion", pVersion, nVersionLen))
    {
        return true;
    }
    strcpy(maErrorStr, "Get QTM Version failed.");
    return false;
}


bool CRTProtocol::GetByteOrder(bool &bBigEndian)
{
    char pResponseStr[256];

    if (SendCommand("ByteOrder", pResponseStr))
    {
        bBigEndian = (strcmp(pResponseStr, "Byte order is big endian") == 0);
        return true;
    }
    strcpy(maErrorStr, "Get Byte order failed.");
    return false;
}


bool CRTProtocol::CheckLicense(const char* pLicenseCode)
{
    char tTemp[100];
    char pResponseStr[256];

    if (strlen(pLicenseCode) <= 85)
    {
        sprintf(tTemp, "CheckLicense %s", pLicenseCode);

        if (SendCommand(tTemp, pResponseStr))
        {
            if (strcmp(pResponseStr, "License pass") == 0)
            {
                return true;
            }
            strcpy(maErrorStr, "Wrong license code.");
        }
        else
        {
            strcpy(maErrorStr, "CheckLicense failed.");
        }
    }
    else
    {
        strcpy(maErrorStr, "License code too long.");
    }
    return false;
}


bool CRTProtocol::DiscoverRTServer(unsigned short nServerPort, bool bNoLocalResponses, unsigned short nDiscoverPort)
{
    char pData[10];
    SDiscoverResponse sResponse;        

    if (mnBroadcastPort == 0)
    {
        if (!mpoNetwork->CreateUDPSocket(nServerPort, true))
        {
            strcpy(maErrorStr, mpoNetwork->GetErrorString());
            return false;
        }
        mnBroadcastPort = nServerPort;
    }
    else
    {
        nServerPort = mnBroadcastPort;
    }

    *((unsigned int*)pData)         = (unsigned int)10;
    *((unsigned int*)(pData + 4))   = (unsigned int)CRTPacket::PacketDiscover;
    *((unsigned short*)(pData + 8)) = htons(nServerPort);

    if (mpoNetwork->SendUDPBroadcast(pData, 10, nDiscoverPort))
    {
        mvsDiscoverResponseList.clear();

        CNetwork::Response response(CNetwork::ResponseType::error, 0);

        do 
        {
            unsigned int nAddr = 0;
            response = mpoNetwork->ReceiveUdpBroadcast(mDataBuff.data(), (int)mDataBuff.size(), 100000, &nAddr);

            if (response && response.received > qtmPacketHeaderSize)
            {
                if (CRTPacket::GetType(mDataBuff.data()) == CRTPacket::PacketCommand)
                {
                    char* discoverResponse  = CRTPacket::GetCommandString(mDataBuff.data());
                    sResponse.nAddr = nAddr;
                    sResponse.nBasePort = CRTPacket::GetDiscoverResponseBasePort(mDataBuff.data());

                    if (discoverResponse && (!bNoLocalResponses || !mpoNetwork->IsLocalAddress(nAddr)))
                    {
                        strcpy(sResponse.message, discoverResponse);
                        mvsDiscoverResponseList.push_back(sResponse);
                    }
                }
            }
        } while (response && response.received > qtmPacketHeaderSize); // Keep reading until no more responses.

        return true;
    }
    return false;
}


int CRTProtocol::GetNumberOfDiscoverResponses()
{
    return (int)mvsDiscoverResponseList.size();
}


bool CRTProtocol::GetDiscoverResponse(unsigned int nIndex, unsigned int &nAddr, unsigned short &nBasePort, std::string& message)
{
    if (nIndex < mvsDiscoverResponseList.size())
    {
        nAddr     = mvsDiscoverResponseList[nIndex].nAddr;
        nBasePort = mvsDiscoverResponseList[nIndex].nBasePort;
        message   = mvsDiscoverResponseList[nIndex].message;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCurrentFrame(const char* components)
{
    char pCommandStr[256];
    strcpy(pCommandStr, "GetCurrentFrame ");
    strcat(pCommandStr, components);

    if (SendCommand(pCommandStr))
    {
        return true;
    }
    strcpy(maErrorStr, "GetCurrentFrame failed.");

    return false;
}


bool CRTProtocol::GetCurrentFrame(unsigned int nComponentType, const SComponentOptions& componentOptions)
{
    char components[256];

    if (GetComponentString(components, nComponentType, componentOptions))
    {
        return GetCurrentFrame(components);
    }
    else
    {
        strcpy(maErrorStr, "DataComponent missing.");
    }
    return false;
}


bool CRTProtocol::StreamFrames(unsigned int nComponentType)
{
    return StreamFrames(RateAllFrames, 0, 0, nullptr, nComponentType);
}

bool CRTProtocol::StreamFrames(EStreamRate eRate, unsigned int nRateArg, unsigned short nUDPPort, const char* pUDPAddr, const char* components)
{
    std::ostringstream commandString;

    if (eRate == RateFrequencyDivisor)
    {
        commandString << "StreamFrames FrequencyDivisor:" << nRateArg << " ";
    }
    else if (eRate == RateFrequency)
    {
        commandString << "StreamFrames Frequency:" << nRateArg << " ";
    }
    else if (eRate == RateAllFrames)
    {
        commandString << "StreamFrames AllFrames ";
    }
    else
    {
        commandString << "No valid rate.";
        return false;
    }

    if (nUDPPort > 0)
    {
        if (pUDPAddr != nullptr && strlen(pUDPAddr) > 64)
        {
            strcpy(maErrorStr, "UDP address string too long.");
            return false;
        }
        commandString << "UDP";
        if (pUDPAddr != nullptr)
        {
            commandString << ":" << pUDPAddr;
        }
        commandString << ":" << nUDPPort << " ";
    }

    commandString << components;

    if (SendCommand(commandString.str().c_str()))
    {
        return true;
    }
    strcpy(maErrorStr, "StreamFrames failed.");

    return false;
}

bool CRTProtocol::StreamFrames(EStreamRate eRate, unsigned int nRateArg, unsigned short nUDPPort, const char* pUDPAddr,
                               unsigned int nComponentType, const SComponentOptions& componentOptions)
{
    char components[256];

    if (GetComponentString(components, nComponentType, componentOptions))
    {
        return StreamFrames(eRate, nRateArg, nUDPPort, pUDPAddr, components);
    }
    else
    {
        strcpy(maErrorStr, "DataComponent missing.");
    }

    return false;
}


bool CRTProtocol::StreamFramesStop()
{
    if (SendCommand("StreamFrames Stop"))
    {
        return true;
    }
    strcpy(maErrorStr, "StreamFrames Stop failed.");
    return false;
}


bool CRTProtocol::GetState(CRTPacket::EEvent &eEvent, bool bUpdate, int nTimeout)
{
    CRTPacket::EPacketType eType;

    if (bUpdate)
    {
        bool result;
        if (mnMajorVersion > 1 || mnMinorVersion > 9)
        {
            result = SendCommand("GetState");
        }
        else
        {
            result = SendCommand("GetLastEvent");
        }
        if (result)
        {
            CNetwork::ResponseType response;
            do 
            {
                response = Receive(eType, false, nTimeout);
                if (response == CNetwork::ResponseType::success)
                {
                    if (mpoRTPacket->GetEvent(eEvent))
                    {
                        return true;
                    }
                }
            } while (response == CNetwork::ResponseType::success);
        }
        strcpy(maErrorStr, "GetLastEvent failed.");
    }
    else
    {
        eEvent = meState;
        return true;
    }
    return false;
}


bool CRTProtocol::GetCapture(const char* pFileName, bool bC3D)
{
    CRTPacket::EPacketType eType;
    char                   pResponseStr[256];

    mpFileBuffer = fopen(pFileName, "wb");
    if (mpFileBuffer != nullptr)
    {
        if (bC3D)
        {
            // C3D file
            if (SendCommand((mnMajorVersion > 1 || mnMinorVersion > 7) ? "GetCaptureC3D" : "GetCapture", pResponseStr))
            {
                if (strcmp(pResponseStr, "Sending capture") == 0)
                {
                    if (Receive(eType, true, 5000000) == CNetwork::ResponseType::success) // Wait for C3D file in 5 seconds.
                    {
                        if (eType == CRTPacket::PacketC3DFile)
                        {
                            if (mpFileBuffer != nullptr)
                            {
                                fclose(mpFileBuffer);
                                return true;
                            }
                            strcpy(maErrorStr, "Writing C3D file failed.");
                        }
                        else
                        {
                            strcpy(maErrorStr, "Wrong packet type received.");
                        }
                    }
                    else
                    {
                        strcpy(maErrorStr, "No packet received.");
                    }
                }
                else
                {
                    sprintf(maErrorStr, "%s failed.", (mnMajorVersion > 1 || mnMinorVersion > 7) ? "GetCaptureC3D" : "GetCapture");
                }
            }
            else
            {
                sprintf(maErrorStr, "%s failed.", (mnMajorVersion > 1 || mnMinorVersion > 7) ? "GetCaptureC3D" : "GetCapture");
            }
        }
        else
        {
            // QTM file
            if (SendCommand("GetCaptureQTM", pResponseStr))
            {
                if (strcmp(pResponseStr, "Sending capture") == 0)
                {
                    if (Receive(eType, true, 5000000) == CNetwork::ResponseType::success) // Wait for QTM file in 5 seconds.
                    {
                        if (eType == CRTPacket::PacketQTMFile)
                        {
                            if (mpFileBuffer != nullptr)
                            {
                                fclose(mpFileBuffer);
                                return true;
                            }
                            strcpy(maErrorStr, "Writing QTM file failed.");
                        }
                        else
                        {
                            strcpy(maErrorStr, "Wrong packet type received.");
                        }
                    }
                    else
                    {
                        sprintf(maErrorStr, "No packet received. %s.", maErrorStr);
                    }
                }
                else
                {
                    strcpy(maErrorStr, "GetCaptureQTM failed.");
                }
            }
            else
            {
                strcpy(maErrorStr, "GetCaptureQTM failed.");
            }
        }
    }
    if (mpFileBuffer)
    {
        fclose(mpFileBuffer);
    }

    return false;
}


bool CRTProtocol::SendTrig()
{
    char pResponseStr[256];

    if (SendCommand("Trig", pResponseStr))
    {
        if (strcmp(pResponseStr, "Trig ok") == 0)
        {
            return true;
        }
    }
    if (pResponseStr)
    {
        sprintf(maErrorStr, "%s.", pResponseStr);
    }
    else
    {
        strcpy(maErrorStr, "Trig failed.");
    }
    return false;
}


bool CRTProtocol::SetQTMEvent(const char* pLabel)
{
    char tTemp[100];
    char pResponseStr[256];

    if (strlen(pLabel) <= 92)
    {
        sprintf(tTemp, "%s %s", (mnMajorVersion > 1 || mnMinorVersion > 7) ? "SetQTMEvent" : "Event", pLabel);

        if (SendCommand(tTemp, pResponseStr))
        {
            if (strcmp(pResponseStr, "Event set") == 0)
            {
                return true;
            }
        }
        if (pResponseStr)
        {
            sprintf(maErrorStr, "%s.", pResponseStr);
        }
        else
        {
            sprintf(maErrorStr, "%s failed.", (mnMajorVersion > 1 || mnMinorVersion > 7) ? "SetQTMEvent" : "Event");
        }
    }
    else
    {
        strcpy(maErrorStr, "Event label too long.");
    }
    return false;
}


bool CRTProtocol::TakeControl(const char* pPassword)
{
    char pResponseStr[256];
    char pCmd[64];

    strcpy(pCmd, "TakeControl");
    if (pPassword != nullptr)
    {
        // Add password
        if (pPassword[0] != 0)
        {
            strcat(pCmd, " ");
            strcat(pCmd, pPassword);
        }
    }
    if (SendCommand(pCmd, pResponseStr))
    {
        if (strcmp("You are now master", pResponseStr)     == 0 ||
            strcmp("You are already master", pResponseStr) == 0)
        {
            mbIsMaster = true;
            return true;
        }
    }
    if (pResponseStr)
    {
        sprintf(maErrorStr, "%s.", pResponseStr);
    }
    else
    {
        strcpy(maErrorStr, "TakeControl failed.");
    }
    mbIsMaster = false;
    return false;
} // TakeControl


bool CRTProtocol::ReleaseControl()
{
    char pResponseStr[256];

    if (SendCommand("ReleaseControl", pResponseStr))
    {
        if (strcmp("You are now a regular client", pResponseStr)     == 0 ||
            strcmp("You are already a regular client", pResponseStr) == 0)
        {
            mbIsMaster = false;
            return true;
        }
    }
    if (pResponseStr)
    {
        sprintf(maErrorStr, "%s.", pResponseStr);
    }
    else
    {
        strcpy(maErrorStr, "ReleaseControl failed.");
    }
    return false;
} // ReleaseControl


bool CRTProtocol::IsControlling()
{
    return mbIsMaster;
}


bool CRTProtocol::NewMeasurement()
{
    char pResponseStr[256];

    if (SendCommand("New", pResponseStr))
    {
        if (strcmp(pResponseStr, "Creating new connection") == 0 ||
            strcmp(pResponseStr, "Already connected") == 0)
        {
            return true;
        }
    }
    if (pResponseStr)
    {
        sprintf(maErrorStr, "%s.", pResponseStr);
    }
    else
    {
        strcpy(maErrorStr, "New failed.");
    }
    return false;
}

bool CRTProtocol::CloseMeasurement()
{
    char pResponseStr[256];

    if (SendCommand("Close", pResponseStr))
    {
        if (strcmp(pResponseStr, "Closing connection") == 0 ||
            strcmp(pResponseStr, "File closed") == 0 ||
            strcmp(pResponseStr, "Closing file") == 0 ||
            strcmp(pResponseStr, "No connection to close") == 0)
        {
            return true;
        }
    }
    if (pResponseStr)
    {
        sprintf(maErrorStr, "%s.", pResponseStr);
    }
    else
    {
        strcpy(maErrorStr, "Close failed.");
    }
    return false;
}


bool CRTProtocol::StartCapture()
{
    char pResponseStr[256];

    if (SendCommand("Start", pResponseStr))
    {
        if (strcmp(pResponseStr, "Starting measurement") == 0)
        {
            return true;
        }
    }
    if (pResponseStr)
    {
        sprintf(maErrorStr, "%s.", pResponseStr);
    }
    else
    {
        strcpy(maErrorStr, "Start failed.");
    }
    return false;
}


bool CRTProtocol::StartRTOnFile()
{
    char pResponseStr[256];

    if (SendCommand("Start rtfromfile", pResponseStr))
    {
        if (strcmp(pResponseStr, "Starting RT from file") == 0)
        {
            return true;
        }
    }
    if (pResponseStr)
    {
        if (strcmp(pResponseStr, "RT from file already running") == 0)
        {
            return true;
        }
        sprintf(maErrorStr, "%s.", pResponseStr);
    }
    else
    {
        strcpy(maErrorStr, "Starting RT from file failed.");
    }
    return false;
}


bool CRTProtocol::StopCapture()
{
    char pResponseStr[256];

    if (SendCommand("Stop", pResponseStr))
    {
        if (strcmp(pResponseStr, "Stopping measurement") == 0)
        {
            return true;
        }
    }
    if (pResponseStr)
    {
        sprintf(maErrorStr, "%s.", pResponseStr);
    }
    else
    {
        strcpy(maErrorStr, "Stop failed.");
    }
    return false;
}


bool CRTProtocol::Calibrate(const bool refine, SCalibration &calibrationResult, int timeout)
{
    char pResponseStr[256];

    if (SendCommand(refine ? "calibrate refine" : "calibrate", pResponseStr))
    {
        if (strcmp(pResponseStr, "Starting calibration") == 0)
        {
            if (ReceiveCalibrationSettings(timeout))
            {
                GetCalibrationSettings(calibrationResult);
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    if (pResponseStr)
    {
        sprintf(maErrorStr, "%s.", pResponseStr);
    }
    else
    {
        strcpy(maErrorStr, "Calibrate failed.");
    }
    return false;
}


bool CRTProtocol::LoadCapture(const char* pFileName)
{
    std::string sendString = "Load \"";
    char pResponseStr[256];

    if (strlen(pFileName) <= 250)
    {
        sendString += pFileName;
        sendString += "\"";

        if (SendCommand(sendString.c_str(), pResponseStr, 20000000)) // Set timeout to 20 s for Load command.
        {
            if (strcmp(pResponseStr, "Measurement loaded") == 0)
            {
                return true;
            }
        }
        if (strlen(pResponseStr) > 0)
        {
            sprintf(maErrorStr, "%s.", pResponseStr);
        }
        else
        {
            strcpy(maErrorStr, "Load failed.");
        }
    }
    else
    {
        strcpy(maErrorStr, "File name too long.");
    }
    return false;
}


bool CRTProtocol::SaveCapture(const char* pFileName, bool bOverwrite, char* pNewFileName, int nSizeOfNewFileName)
{
    char tTemp[100];
    char pResponseStr[256];
    char tNewFileNameTmp[300];

    tNewFileNameTmp[0] = 0;

    if (strlen(pFileName) <= 94)
    {
        sprintf(tTemp, "Save %s%s", pFileName, bOverwrite ? " Overwrite" : "");

        if (SendCommand(tTemp, pResponseStr))
        {
            if (strcmp(pResponseStr, "Measurement saved") == 0)
            {
                if (pNewFileName && nSizeOfNewFileName > 0)
                {
                    pNewFileName[0] = 0;
                }
                return true;
            }
            if (sscanf(pResponseStr, "Measurement saved as '%[^']'", tNewFileNameTmp) == 1)
            {
                if (pNewFileName)
                {
                    strcpy(pNewFileName, tNewFileNameTmp);
                }
                return true;
            }
        }
        if (pResponseStr && strlen(pResponseStr) > 0)
        {
            sprintf(maErrorStr, "%s.", pResponseStr);
        }
        else
        {
            strcpy(maErrorStr, "Save failed.");
        }
    }
    else
    {
        strcpy(maErrorStr, "File name too long.");
    }
    return false;
}


bool CRTProtocol::LoadProject(const char* pFileName)
{
    char tTemp[100];
    char pResponseStr[256];

    if (strlen(pFileName) <= 94)
    {
        sprintf(tTemp, "LoadProject %s", pFileName);

        if (SendCommand(tTemp, pResponseStr, 20000000)) // Timeout 20 s
        {
            if (strcmp(pResponseStr, "Project loaded") == 0)
            {
                return true;
            }
        }
        if (pResponseStr)
        {
            sprintf(maErrorStr, "%s.", pResponseStr);
        }
        else
        {
            strcpy(maErrorStr, "Load project failed.");
        }
    }
    else
    {
        strcpy(maErrorStr, "File name too long.");
    }
    return false;
}


bool CRTProtocol::Reprocess()
{
    char pResponseStr[256];

    if (SendCommand("Reprocess", pResponseStr))
    {
        if (strcmp(pResponseStr, "Reprocessing file") == 0)
        {
            return true;
        }
    }
    if (pResponseStr)
    {
        sprintf(maErrorStr, "%s.", pResponseStr);
    }
    else
    {
        strcpy(maErrorStr, "Reprocess failed.");
    }
    return false;
}


bool CRTProtocol::GetEventString(CRTPacket::EEvent eEvent, char* pStr)
{
    switch (eEvent)
    {
        case CRTPacket::EventConnected: strcpy(pStr, "Connected");
            break;
        case CRTPacket::EventConnectionClosed: strcpy(pStr, "Connection Closed");
            break;
        case CRTPacket::EventCaptureStarted: strcpy(pStr, "Capture Started");
            break;
        case CRTPacket::EventCaptureStopped: strcpy(pStr, "Capture Stopped");
            break;
        case CRTPacket::EventCaptureFetchingFinished: strcpy(pStr, "Fetching Finished");
            break;
        case CRTPacket::EventCalibrationStarted: strcpy(pStr, "Calibration Started");
            break;
        case CRTPacket::EventCalibrationStopped: strcpy(pStr, "Calibration Stopped");
            break;
        case CRTPacket::EventRTfromFileStarted: strcpy(pStr, "RT From File Started");
            break;
        case CRTPacket::EventRTfromFileStopped: strcpy(pStr, "RT From File Stopped");
            break;
        case CRTPacket::EventWaitingForTrigger: strcpy(pStr, "Waiting For Trigger");
            break;
        case CRTPacket::EventCameraSettingsChanged: strcpy(pStr, "Camera Settings Changed");
            break;
        case CRTPacket::EventQTMShuttingDown: strcpy(pStr, "QTM Shutting Down");
            break;
        case CRTPacket::EventCaptureSaved: strcpy(pStr, "Capture Saved");
            break;
        case CRTPacket::EventReprocessingStarted: strcpy(pStr, "Reprocessing Started");
            break;
        case CRTPacket::EventReprocessingStopped: strcpy(pStr, "Reprocessing Stopped");
            break;
        case CRTPacket::EventTrigger: strcpy(pStr, "Trigger");
            break;
        default:
            return false;
    }
    return true;
}


bool CRTProtocol::ConvertRateString(const char* pRate, EStreamRate &eRate, unsigned int &nRateArg)
{
    std::string rateString;

    rateString.assign(pRate);
    rateString = ToLower(rateString);

    eRate = RateNone;

    if (rateString.compare(0, 9, "allframes", 9) == 0)
    {
        eRate = RateAllFrames;
    }
    else if (rateString.compare(0, 10, "frequency:") == 0)
    {
        nRateArg = atoi(rateString.substr(10).c_str());
        if (nRateArg > 0)
        {
            eRate = RateFrequency;
        }
    }
    else if (rateString.compare(0, 17, "frequencydivisor:") == 0)
    {
        nRateArg = atoi(rateString.substr(17).c_str());
        if (nRateArg > 0)
        {
            eRate = RateFrequencyDivisor;
        }
    }

    return eRate != RateNone;
}

std::vector<std::pair<unsigned int, std::string>> CRTProtocol::GetComponents(const std::string componentsString)
{
    std::vector<std::pair<unsigned int, std::string>> components; // Vector containing pair (component, option string).
    auto result = std::back_inserter(components);
    std::istringstream iss(componentsString);
    std::string item;
    while (std::getline(iss, item, ' '))
    {
        unsigned int component;
        std::string option;
        if (GetComponent(item, component, option))
        {
            *result++ = std::make_pair(component, option);
        }
        else
        {
            // Parsing failed. Unrecognized component.
            components.clear();
            break;
        }
    }

    return components;
}

bool CRTProtocol::GetComponent(std::string componentStr, unsigned int &component, std::string &option)
{
    // Make string lower case.
    componentStr = ToLower(componentStr);
    option = "";

    if (componentStr == "2d")
    {
        component = CRTProtocol::cComponent2d;
        return true;
    }
    if (componentStr == "2dlin")
    {
        component = CRTProtocol::cComponent2dLin;
        return true;
    }
    if (componentStr == "3d")
    {
        component = CRTProtocol::cComponent3d;
        return true;
    }
    if (componentStr == "3dres")
    {
        component = CRTProtocol::cComponent3dRes;
        return true;
    }
    if (componentStr == "3dnolabels")
    {
        component = CRTProtocol::cComponent3dNoLabels;
        return true;
    }
    if (componentStr == "3dnolabelsres")
    {
        component = CRTProtocol::cComponent3dNoLabelsRes;
        return true;
    }
    if (componentStr == "analogsingle")
    {
        component = CRTProtocol::cComponentAnalogSingle;
        return true;
    }
    if (componentStr.find("analogsingle:") != std::string::npos)
    {
        option = componentStr.substr(strlen("analogsingle:"));
        component = CRTProtocol::cComponentAnalogSingle;
        return true;
    }
    if (componentStr == "analog")
    {
        component = CRTProtocol::cComponentAnalog;
        return true;
    }
    if (componentStr.find("analog:") != std::string::npos)
    {
        option = componentStr.substr(strlen("analog:"));
        component = CRTProtocol::cComponentAnalog;
        return true;
    }
    if (componentStr == "force")
    {
        component = CRTProtocol::cComponentForce;
        return true;
    }
    if (componentStr == "forcesingle")
    {
        component = CRTProtocol::cComponentForceSingle;
        return true;
    }
    if (componentStr == "6d")
    {
        component = CRTProtocol::cComponent6d;
        return true;
    }
    if (componentStr == "6dres")
    {
        component = CRTProtocol::cComponent6dRes;
        return true;
    }
    if (componentStr == "6deuler")
    {
        component = CRTProtocol::cComponent6dEuler;
        return true;
    }
    if (componentStr == "6deulerres")
    {
        component = CRTProtocol::cComponent6dEulerRes;
        return true;
    }
    if (componentStr == "image")
    {
        component = CRTProtocol::cComponentImage;
        return true;
    }
    if (componentStr == "gazevector")
    {
        component = CRTProtocol::cComponentGazeVector;
        return true;
    }
    if (componentStr == "eyetracker")
    {
        component = CRTProtocol::cComponentEyeTracker;
        return true;
    }
    if (componentStr == "timecode")
    {
        component = CRTProtocol::cComponentTimecode;
        return true;
    }
    if (componentStr == "skeleton")
    {
        component = CRTProtocol::cComponentSkeleton;
        return true;
    }
    if (componentStr == "skeleton:global")
    {
        option = "global";
        component = CRTProtocol::cComponentSkeleton;
        return true;
    }
    return false;
}


unsigned int CRTProtocol::ConvertComponentString(const char* componentsString)
{
    auto components = GetComponents(std::string(componentsString));

    unsigned int componentTypes = 0;

    for (auto const& component : components)
    {
        componentTypes += component.first;
    }

    return componentTypes;
}


bool CRTProtocol::GetComponentString(char* pComponentStr, unsigned int nComponentType, const SComponentOptions& options)
{
    pComponentStr[0] = 0;

    if (nComponentType & cComponent2d)
    {
        strcat(pComponentStr, "2D ");
    }
    if (nComponentType & cComponent2dLin)
    {
        strcat(pComponentStr, "2DLin ");
    }
    if (nComponentType & cComponent3d)
    {
        strcat(pComponentStr, "3D ");
    }
    if (nComponentType & cComponent3dRes)
    {
        strcat(pComponentStr, "3DRes ");
    }
    if (nComponentType & cComponent3dNoLabels)
    {
        strcat(pComponentStr, "3DNoLabels ");
    }
    if (nComponentType & cComponent3dNoLabelsRes)
    {
        strcat(pComponentStr, "3DNoLabelsRes ");
    }
    if (nComponentType & cComponent6d)
    {
        strcat(pComponentStr, "6D ");
    }
    if (nComponentType & cComponent6dRes)
    {
        strcat(pComponentStr, "6DRes ");
    }
    if (nComponentType & cComponent6dEuler)
    {
        strcat(pComponentStr, "6DEuler ");
    }
    if (nComponentType & cComponent6dEulerRes)
    {
        strcat(pComponentStr, "6DEulerRes ");
    }
    if (nComponentType & cComponentAnalog)
    {
        strcat(pComponentStr, "Analog");

        if (options.mAnalogChannels != nullptr)
        {
            strcat(pComponentStr, ":");
            strcat(pComponentStr, options.mAnalogChannels);
        }

        strcat(pComponentStr, " ");
    }
    if (nComponentType & cComponentAnalogSingle)
    {
        strcat(pComponentStr, "AnalogSingle");

        if (options.mAnalogChannels != nullptr)
        {
            strcat(pComponentStr, ":");
            strcat(pComponentStr, options.mAnalogChannels);
        }

        strcat(pComponentStr, " ");
    }
    if (nComponentType & cComponentForce)
    {
        strcat(pComponentStr, "Force ");
    }
    if (nComponentType & cComponentForceSingle)
    {
        strcat(pComponentStr, "ForceSingle ");
    }
    if (nComponentType & cComponentGazeVector)
    {
        strcat(pComponentStr, "GazeVector ");
    }
    if (nComponentType & cComponentEyeTracker)
    {
        strcat(pComponentStr, "EyeTracker ");
    }
    if (nComponentType & cComponentImage)
    {
        strcat(pComponentStr, "Image ");
    }
    if (nComponentType & cComponentTimecode)
    {
        strcat(pComponentStr, "Timecode ");
    }
    if (nComponentType & cComponentSkeleton)
    {
        strcat(pComponentStr, "Skeleton");

        if (options.mSkeletonGlobalData)
        {
            strcat(pComponentStr, ":global");
        }

        strcat(pComponentStr, " ");
    }

    return (pComponentStr[0] != 0);
}


int CRTProtocol::ReceiveRTPacket(CRTPacket::EPacketType &eType, bool bSkipEvents, int nTimeout)
{
    int returnVal = -1;
    auto response = Receive(eType, bSkipEvents, nTimeout);
    
    switch (response)
    {
        case CNetwork::ResponseType::success:
            returnVal = mpoRTPacket->GetSize();
            break;
        case CNetwork::ResponseType::timeout:
        case CNetwork::ResponseType::disconnect:
            returnVal = 0;
            break;
        case CNetwork::ResponseType::error:
            returnVal = -1;
            break;
    }

    return returnVal;
}


CNetwork::ResponseType CRTProtocol::Receive(CRTPacket::EPacketType &eType, bool bSkipEvents, int nTimeout)
{
    CNetwork::Response response(CNetwork::ResponseType::error, 0);
    unsigned int nRecvedTotal = 0;
    unsigned int nFrameSize;

    eType = CRTPacket::PacketNone;

    do 
    {
        nRecvedTotal = 0;

        response = mpoNetwork->Receive(mDataBuff.data(), (int)mDataBuff.size(), true, nTimeout);

        if (response.type == CNetwork::ResponseType::timeout)
        {
            // Receive timeout.
            strcpy(maErrorStr, "Data receive timeout.");
            return CNetwork::ResponseType::timeout;
        }
        if (response.type == CNetwork::ResponseType::error)
        {
            strcpy(maErrorStr, "Socket Error.");
            return CNetwork::ResponseType::error;
        }
        if (response.type == CNetwork::ResponseType::disconnect)
        {
            strcpy(maErrorStr, "Disconnected from server.");
            return CNetwork::ResponseType::disconnect;
        }        
        if (response.received < qtmPacketHeaderSize)
        {
            // QTM header not received.
            strcpy(maErrorStr, "Couldn't read header bytes.");
            return CNetwork::ResponseType::error;
        }
        nRecvedTotal += response.received;

        bool bBigEndian = (mbBigEndian || (mnMajorVersion == 1 && mnMinorVersion == 0));
        nFrameSize = mpoRTPacket->GetSize(mDataBuff.data(), bBigEndian);
        eType      = mpoRTPacket->GetType(mDataBuff.data(), bBigEndian);
        
        unsigned int nReadSize;

        if (eType == CRTPacket::PacketC3DFile || eType == CRTPacket::PacketQTMFile)
        {
            if (mpFileBuffer != nullptr)
            {
                rewind(mpFileBuffer); // Start from the beginning
                if (fwrite(mDataBuff.data() + sizeof(int) * 2, 1, nRecvedTotal - sizeof(int) * 2, mpFileBuffer) !=
                    nRecvedTotal - sizeof(int) * 2)
                {
                    strcpy(maErrorStr, "Failed to write file to disk.");
                    fclose(mpFileBuffer);
                    mpFileBuffer = nullptr;
                    return CNetwork::ResponseType::error;
                }
                // Receive more data until we have read the whole packet
                while (nRecvedTotal < nFrameSize) 
                {
                    nReadSize = nFrameSize - nRecvedTotal;
                    if (nFrameSize > mDataBuff.size())                                                                                                                                                                                                                                                                                             
                    {
                        nReadSize = (int)mDataBuff.size();
                    }
                    // As long as we haven't received enough data, wait for more
                    response = mpoNetwork->Receive(&(mDataBuff.data()[sizeof(int) * 2]), nReadSize, false, cWaitForDataTimeout);
                    if (response.type == CNetwork::ResponseType::timeout)
                    {
                        strcpy(maErrorStr, "Packet truncated.");
                        return CNetwork::ResponseType::error;
                    }
                    if (response.type == CNetwork::ResponseType::error)
                    {
                        strcpy(maErrorStr, "Socket Error.");
                        fclose(mpFileBuffer);
                        mpFileBuffer = nullptr;
                        return CNetwork::ResponseType::error;
                    }
                    if (response.type == CNetwork::ResponseType::disconnect)
                    {
                        strcpy(maErrorStr, "Disconnected from server.");
                        return CNetwork::ResponseType::disconnect;
                    }

                    if (fwrite(mDataBuff.data() + sizeof(int) * 2, 1, response.received, mpFileBuffer) != (size_t)(response.received))
                    {
                        strcpy(maErrorStr, "Failed to write file to disk.");
                        fclose(mpFileBuffer);
                        mpFileBuffer = nullptr;
                        return CNetwork::ResponseType::error;
                    }
                    nRecvedTotal += response.received;
                }
            }
            else
            {
                strcpy(maErrorStr, "Receive file buffer not opened.");
                if (mpFileBuffer)
                {
                    fclose(mpFileBuffer);
                }
                mpFileBuffer = nullptr;
                return CNetwork::ResponseType::error;
            }
        }
        else
        {
            if (nFrameSize > mDataBuff.size())
            {
                mDataBuff.resize(nFrameSize);
            }

            // Receive more data until we have read the whole packet
            while (nRecvedTotal < nFrameSize) 
            {
                // As long as we haven't received enough data, wait for more
                response = mpoNetwork->Receive(&(mDataBuff.data()[nRecvedTotal]), nFrameSize - nRecvedTotal, false, -1);
                if (response.type == CNetwork::ResponseType::timeout)
                {
                    strcpy(maErrorStr, "Packet truncated.");
                    return CNetwork::ResponseType::error;
                }
                if (response.type == CNetwork::ResponseType::error)
                {
                    strcpy(maErrorStr, "Socket Error.");
                    return CNetwork::ResponseType::error;
                }
                if (response.type == CNetwork::ResponseType::disconnect)
                {
                    strcpy(maErrorStr, "Disconnected from server.");
                    return CNetwork::ResponseType::disconnect;
                }
                nRecvedTotal += response.received;
            }
        }

        mpoRTPacket->SetData(mDataBuff.data());

        if (mpoRTPacket->GetEvent(meLastEvent)) // Update last event if there is an event
        {
            if (meLastEvent != CRTPacket::EventCameraSettingsChanged)
            {
                meState = meLastEvent;
            }
        }
    } while (bSkipEvents && eType == CRTPacket::PacketEvent);
    
    if (nRecvedTotal == nFrameSize)
    {
        return CNetwork::ResponseType::success;
    }
    strcpy(maErrorStr, "Packet truncated.");

    return CNetwork::ResponseType::error;
} // ReceiveRTPacket


CRTPacket* CRTProtocol::GetRTPacket()
{
    return mpoRTPacket;
}


bool CRTProtocol::ReadXmlBool(CMarkup* xml, const std::string& element, bool& value) const
{
    if (!xml->FindChildElem(element.c_str()))
    {
        return false;
    }

    auto str = xml->GetChildData();
    RemoveInvalidChars(str);
    str = ToLower(str);

    if (str == "true")
    {
        value = true;
    }
    else if (str == "false")
    {
        value = false;
    }
    else
    {
        // Don't change value, just report error.
        return false;
    }

    return true;
}


bool CRTProtocol::ReadSettings(std::string settingsType, CMarkup &oXML)
{
    CRTPacket::EPacketType eType;

    mvsAnalogDeviceSettings.clear();

    auto sendStr = std::string("GetParameters ") + settingsType;
    if (!SendCommand(sendStr.c_str()))
    {
        sprintf(maErrorStr, "GetParameters %s failed", settingsType.c_str());
        return false;
    }

    auto received = Receive(eType, true);

    if (received == CNetwork::ResponseType::timeout)
    {
        strcat(maErrorStr, " Expected XML packet.");
        return false;
    }
    if (received == CNetwork::ResponseType::error)
    {
        return false;
    }

    if (eType != CRTPacket::PacketXML)
    {
        if (eType == CRTPacket::PacketError)
        {
            sprintf(maErrorStr, "%s.", mpoRTPacket->GetErrorString());
        }
        else
        {
            sprintf(maErrorStr, "GetParameters %s returned wrong packet type. Got type %d expected type 2.", settingsType.c_str(), eType);
        }
        return false;
    }

    oXML.SetDoc(mpoRTPacket->GetXMLString());
    
    return true;
}


bool CRTProtocol::ReadCameraSystemSettings()
{
    return ReadGeneralSettings();
}


bool CRTProtocol::ReadGeneralSettings()
{
    CMarkup                 oXML;
    std::string             tStr;

    msGeneralSettings.vsCameras.clear();

    if (!ReadSettings("General", oXML))
    {
        return false;
    }

    // ==================== General ====================
    if (!oXML.FindChildElem("General"))
    {
        return false;
    }
    oXML.IntoElem();

    if (!oXML.FindChildElem("Frequency"))
    {
        return false;
    }
    msGeneralSettings.nCaptureFrequency = atoi(oXML.GetChildData().c_str());

    if (!oXML.FindChildElem("Capture_Time"))
    {
        return false;
    }
    msGeneralSettings.fCaptureTime = (float)atof(oXML.GetChildData().c_str());

    // Refactored variant of all this copy/paste code. TODO: Refactor everything else.
    if (!ReadXmlBool(&oXML, "Start_On_External_Trigger", msGeneralSettings.bStartOnExternalTrigger))
    {
        return false;
    }
    if (mnMajorVersion > 1 || mnMinorVersion > 14)
    {
        if (!ReadXmlBool(&oXML, "Start_On_Trigger_NO", msGeneralSettings.bStartOnTrigNO))
        {
            return false;
        }
        if (!ReadXmlBool(&oXML, "Start_On_Trigger_NC", msGeneralSettings.bStartOnTrigNC))
        {
            return false;
        }
        if (!ReadXmlBool(&oXML, "Start_On_Trigger_Software", msGeneralSettings.bStartOnTrigSoftware))
        {
            return false;
        }
    }

    // ==================== External time base ====================
    if (!oXML.FindChildElem("External_Time_Base"))
    {
        return false;
    }
    oXML.IntoElem();

    if (!oXML.FindChildElem("Enabled"))
    {
        return false;
    }
    tStr = ToLower(oXML.GetChildData());
    msGeneralSettings.sExternalTimebase.bEnabled = (tStr == "true");

    if (!oXML.FindChildElem("Signal_Source"))
    {
        return false;
    }
    tStr = ToLower(oXML.GetChildData());
    if (tStr == "control port")
    {
        msGeneralSettings.sExternalTimebase.eSignalSource = SourceControlPort;
    }
    else if (tStr == "ir receiver")
    {
        msGeneralSettings.sExternalTimebase.eSignalSource = SourceIRReceiver;
    }
    else if (tStr == "smpte")
    {
        msGeneralSettings.sExternalTimebase.eSignalSource = SourceSMPTE;
    }
    else if (tStr == "irig")
    {
        msGeneralSettings.sExternalTimebase.eSignalSource = SourceIRIG;
    }
    else if (tStr == "video sync")
    {
        msGeneralSettings.sExternalTimebase.eSignalSource = SourceVideoSync;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Signal_Mode"))
    {
        return false;
    }
    tStr = ToLower(oXML.GetChildData());
    if (tStr == "periodic")
    {
        msGeneralSettings.sExternalTimebase.bSignalModePeriodic = true;
    }
    else if (tStr == "non-periodic")
    {
        msGeneralSettings.sExternalTimebase.bSignalModePeriodic = false;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Frequency_Multiplier"))
    {
        return false;
    }
    unsigned int nMultiplier;
    tStr = oXML.GetChildData();
    if (sscanf(tStr.c_str(), "%u", &nMultiplier) == 1)
    {
        msGeneralSettings.sExternalTimebase.nFreqMultiplier = nMultiplier;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Frequency_Divisor"))
    {
        return false;
    }
    unsigned int nDivisor;
    tStr = oXML.GetChildData();
    if (sscanf(tStr.c_str(), "%u", &nDivisor) == 1)
    {
        msGeneralSettings.sExternalTimebase.nFreqDivisor = nDivisor;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Frequency_Tolerance"))
    {
        return false;
    }
    unsigned int nTolerance;
    tStr = oXML.GetChildData();
    if (sscanf(tStr.c_str(), "%u", &nTolerance) == 1)
    {
        msGeneralSettings.sExternalTimebase.nFreqTolerance = nTolerance;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Nominal_Frequency"))
    {
        return false;
    }
    tStr = ToLower(oXML.GetChildData());

    if (tStr == "none")
    {
        msGeneralSettings.sExternalTimebase.fNominalFrequency = -1; // -1 = disabled
    }
    else
    {
        float fFrequency;
        if (sscanf(tStr.c_str(), "%f", &fFrequency) == 1)
        {
            msGeneralSettings.sExternalTimebase.fNominalFrequency = fFrequency;
        }
        else
        {
            return false;
        }
    }

    if (!oXML.FindChildElem("Signal_Edge"))
    {
        return false;
    }
    tStr = ToLower(oXML.GetChildData());
    if (tStr == "negative")
    {
        msGeneralSettings.sExternalTimebase.bNegativeEdge = true;
    }
    else if (tStr == "positive")
    {
        msGeneralSettings.sExternalTimebase.bNegativeEdge = false;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Signal_Shutter_Delay"))
    {
        return false;
    }
    unsigned int nDelay;
    tStr = oXML.GetChildData();
    if (sscanf(tStr.c_str(), "%u", &nDelay) == 1)
    {
        msGeneralSettings.sExternalTimebase.nSignalShutterDelay = nDelay;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("Non_Periodic_Timeout"))
    {
        return false;
    }
    float fTimeout;
    tStr = oXML.GetChildData();
    if (sscanf(tStr.c_str(), "%f", &fTimeout) == 1)
    {
        msGeneralSettings.sExternalTimebase.fNonPeriodicTimeout = fTimeout;
    }
    else
    {
        return false;
    }

    oXML.OutOfElem(); // External_Time_Base


    // External_Timestamp
    if (oXML.FindChildElem("External_Timestamp"))
    {
        oXML.IntoElem();

        if (oXML.FindChildElem("Enabled"))
        {
            tStr = ToLower(oXML.GetChildData());
            msGeneralSettings.sTimestamp.bEnabled = (tStr == "true");
        }
        if (oXML.FindChildElem("Type"))
        {
            tStr = ToLower(oXML.GetChildData());
            if (tStr == "smpte")
            {
                msGeneralSettings.sTimestamp.nType = Timestamp_SMPTE;
            }
            else if (tStr == "irig")
            {
                msGeneralSettings.sTimestamp.nType = Timestamp_IRIG;
            }
            else
            {
                msGeneralSettings.sTimestamp.nType = Timestamp_CameraTime;
            }
        }
        if (oXML.FindChildElem("Frequency"))
        {
            unsigned int timestampFrequency;
            tStr = oXML.GetChildData();
            if (sscanf(tStr.c_str(), "%u", &timestampFrequency) == 1)
            {
                msGeneralSettings.sTimestamp.nFrequency = timestampFrequency;
            }
        }
        oXML.OutOfElem();
    }
    // External_Timestamp


    const char* processings[3] = { "Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions" };
    EProcessingActions* processingActions[3] =
    {
        &msGeneralSettings.eProcessingActions,
        &msGeneralSettings.eRtProcessingActions,
        &msGeneralSettings.eReprocessingActions
    };
    auto actionsCount = (mnMajorVersion > 1 || mnMinorVersion > 13) ? 3 : 1;
    for (auto i = 0; i < actionsCount; i++)
    {
        // ==================== Processing actions ====================
        if (!oXML.FindChildElem(processings[i]))
        {
            return false;
        }
        oXML.IntoElem();

        *processingActions[i] = ProcessingNone;

        if (mnMajorVersion > 1 || mnMinorVersion > 13)
        {
            if (!oXML.FindChildElem("PreProcessing2D"))
            {
                return false;
            }
            if (CompareNoCase(oXML.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingPreProcess2D);
            }
        }

        if (!oXML.FindChildElem("Tracking"))
        {
            return false;
        }
        tStr = ToLower(oXML.GetChildData());
        if (tStr == "3d")
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingTracking3D);
        }
        else if (tStr == "2d" && i != 1) // i != 1 => Not RtProcessingSettings
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingTracking2D);
        }

        if (i != 1) //Not RtProcessingSettings
        {
            if (!oXML.FindChildElem("TwinSystemMerge"))
            {
                return false;
            }
            if (CompareNoCase(oXML.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingTwinSystemMerge);
            }

            if (!oXML.FindChildElem("SplineFill"))
            {
                return false;
            }
            if (CompareNoCase(oXML.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingSplineFill);
            }
        }

        if (!oXML.FindChildElem("AIM"))
        {
            return false;
        }
        if (CompareNoCase(oXML.GetChildData(), "true"))
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingAIM);
        }

        if (!oXML.FindChildElem("Track6DOF"))
        {
            return false;
        }
        if (CompareNoCase(oXML.GetChildData(), "true"))
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + Processing6DOFTracking);
        }

        if (!oXML.FindChildElem("ForceData"))
        {
            return false;
        }
        if (CompareNoCase(oXML.GetChildData(), "true"))
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingForceData);
        }

        if (mnMajorVersion > 1 || mnMinorVersion > 11)
        {
            if (!oXML.FindChildElem("GazeVector"))
            {
                return false;
            }
            if (CompareNoCase(oXML.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingGazeVector);
            }
        }

        if (i != 1) //Not RtProcessingSettings
        {
            if (!oXML.FindChildElem("ExportTSV"))
            {
                return false;
            }
            if (CompareNoCase(oXML.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportTSV);
            }

            if (!oXML.FindChildElem("ExportC3D"))
            {
                return false;
            }
            if (CompareNoCase(oXML.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportC3D);
            }

            if (!oXML.FindChildElem("ExportMatlabFile"))
            {
                return false;
            }
            if (CompareNoCase(oXML.GetChildData(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportMatlabFile);
            }

            if (mnMajorVersion > 1 || mnMinorVersion > 11)
            {
                if (!oXML.FindChildElem("ExportAviFile"))
                {
                    return false;
                }
                if (CompareNoCase(oXML.GetChildData(), "true"))
                {
                    *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportAviFile);
                }
            }
        }
        oXML.OutOfElem(); // Processing_Actions
    }

    if (oXML.FindChildElem("EulerAngles"))
    {
        oXML.IntoElem();
        msGeneralSettings.eulerRotations[0] = oXML.GetAttrib("First");
        msGeneralSettings.eulerRotations[1] = oXML.GetAttrib("Second");
        msGeneralSettings.eulerRotations[2] = oXML.GetAttrib("Third");
        oXML.OutOfElem();
    }

    SSettingsGeneralCamera sCameraSettings;

    while (oXML.FindChildElem("Camera"))
    {
        oXML.IntoElem();

        if (!oXML.FindChildElem("ID"))
        {
            return false;
        }
        sCameraSettings.nID = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Model"))
        {
            return false;
        }
        tStr = ToLower(oXML.GetChildData());

        if (tStr == "macreflex")
        {
            sCameraSettings.eModel = ModelMacReflex;
        }
        else if (tStr == "proreflex 120")
        {
            sCameraSettings.eModel = ModelProReflex120;
        }
        else if (tStr == "proreflex 240")
        {
            sCameraSettings.eModel = ModelProReflex240;
        }
        else if (tStr == "proreflex 500")
        {
            sCameraSettings.eModel = ModelProReflex500;
        }
        else if (tStr == "proreflex 1000")
        {
            sCameraSettings.eModel = ModelProReflex1000;
        }
        else if (tStr == "oqus 100")
        {
            sCameraSettings.eModel = ModelOqus100;
        }
        else if (tStr == "oqus 200" || tStr == "oqus 200 c")
        {
            sCameraSettings.eModel = ModelOqus200C;
        }
        else if (tStr == "oqus 300")
        {
            sCameraSettings.eModel = ModelOqus300;
        }
        else if (tStr == "oqus 300 plus")
        {
            sCameraSettings.eModel = ModelOqus300Plus;
        }
        else if (tStr == "oqus 400")
        {
            sCameraSettings.eModel = ModelOqus400;
        }
        else if (tStr == "oqus 500")
        {
            sCameraSettings.eModel = ModelOqus500;
        }
        else if (tStr == "oqus 500 plus")
        {
            sCameraSettings.eModel = ModelOqus500Plus;
        }
        else if (tStr == "oqus 700")
        {
            sCameraSettings.eModel = ModelOqus700;
        }
        else if (tStr == "oqus 700 plus")
        {
            sCameraSettings.eModel = ModelOqus700Plus;
        }
        else if (tStr == "oqus 600 plus")
        {
            sCameraSettings.eModel = ModelOqus600Plus;
        }
        else if (tStr == "miqus m1")
        {
            sCameraSettings.eModel = ModelMiqusM1;
        }
        else if (tStr == "miqus m3")
        {
            sCameraSettings.eModel = ModelMiqusM3;
        }
        else if (tStr == "miqus m5")
        {
            sCameraSettings.eModel = ModelMiqusM5;
        }
        else if (tStr == "miqus sync unit")
        {
            sCameraSettings.eModel = ModelMiqusSyncUnit;
        }
        else if (tStr == "miqus video")
        {
            sCameraSettings.eModel = ModelMiqusVideo;
        }
        else if (tStr == "miqus video color")
        {
            sCameraSettings.eModel = ModelMiqusVideoColor;
        }
        else if (tStr == "miqus hybrid")
        {
            sCameraSettings.eModel = ModelMiqusHybrid;
        }
        else if (tStr == "arqus a5")
        {
            sCameraSettings.eModel = ModelArqusA5;
        }
        else if (tStr == "arqus a9")
        {
            sCameraSettings.eModel = ModelArqusA9;
        }
        else if (tStr == "arqus a12")
        {
            sCameraSettings.eModel = ModelArqusA12;
        }
        else if (tStr == "arqus a26")
        {
            sCameraSettings.eModel = ModelArqusA26;
        }
        else
        {
            sCameraSettings.eModel = ModelUnknown;
        }

        // Only available from protocol version 1.10 and later.
        if (oXML.FindChildElem("Underwater"))
        {
            tStr = ToLower(oXML.GetChildData());
            sCameraSettings.bUnderwater = (tStr == "true");
        }

        if (oXML.FindChildElem("Supports_HW_Sync"))
        {
            tStr = ToLower(oXML.GetChildData());
            sCameraSettings.bSupportsHwSync = (tStr == "true");
        }

        if (!oXML.FindChildElem("Serial"))
        {
            return false;
        }
        sCameraSettings.nSerial = atoi(oXML.GetChildData().c_str());

        // ==================== Camera Mode ====================
        if (!oXML.FindChildElem("Mode"))
        {
            return false;
        }
        tStr = ToLower(oXML.GetChildData());
        if (tStr == "marker")
        {
            sCameraSettings.eMode = ModeMarker;
        }
        else if (tStr == "marker intensity")
        {
            sCameraSettings.eMode = ModeMarkerIntensity;
        }
        else if (tStr == "video")
        {
            sCameraSettings.eMode = ModeVideo;
        }
        else
        {
            return false;
        }

        if (mnMajorVersion > 1 || mnMinorVersion > 11)
        {
            // ==================== Video frequency ====================
            if (!oXML.FindChildElem("Video_Frequency"))
            {
                return false;
            }
            sCameraSettings.nVideoFrequency = atoi(oXML.GetChildData().c_str());
        }

        // ==================== Video Resolution ====================
        if (oXML.FindChildElem("Video_Resolution"))
        {
            tStr = ToLower(oXML.GetChildData());
            if (tStr == "1080p")
            {
                sCameraSettings.eVideoResolution = VideoResolution1080p;
            }
            else if (tStr == "720p")
            {
                sCameraSettings.eVideoResolution = VideoResolution720p;
            }
            else if (tStr == "540p")
            {
                sCameraSettings.eVideoResolution = VideoResolution540p;
            }
            else if (tStr == "480p")
            {
                sCameraSettings.eVideoResolution = VideoResolution480p;
            }
        }
        else
        {
            sCameraSettings.eVideoResolution = VideoResolutionNone;
        }

        // ==================== Video AspectRatio ====================
        if (oXML.FindChildElem("Video_Aspect_Ratio"))
        {
            tStr = ToLower(oXML.GetChildData());
            if (tStr == "16x9")
            {
                sCameraSettings.eVideoAspectRatio = VideoAspectRatio16x9;
            }
            else if (tStr == "4x3")
            {
                sCameraSettings.eVideoAspectRatio = VideoAspectRatio4x3;
            }
            else if (tStr == "1x1")
            {
                sCameraSettings.eVideoAspectRatio = VideoAspectRatio1x1;
            }
        }
        else
        {
            sCameraSettings.eVideoAspectRatio = VideoAspectRatioNone;
        }

        // ==================== Video exposure ====================
        if (!oXML.FindChildElem("Video_Exposure"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("Current"))
        {
            return false;
        }
        sCameraSettings.nVideoExposure = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Min"))
        {
            return false;
        }
        sCameraSettings.nVideoExposureMin = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Max"))
        {
            return false;
        }
        sCameraSettings.nVideoExposureMax = atoi(oXML.GetChildData().c_str());
        oXML.OutOfElem(); // Video_Exposure

        // ==================== Video flash time ====================
        if (!oXML.FindChildElem("Video_Flash_Time"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("Current"))
        {
            return false;
        }
        sCameraSettings.nVideoFlashTime = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Min"))
        {
            return false;
        }
        sCameraSettings.nVideoFlashTimeMin = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Max"))
        {
            return false;
        }
        sCameraSettings.nVideoFlashTimeMax = atoi(oXML.GetChildData().c_str());
        oXML.OutOfElem(); // Video_Flash_Time

        // ==================== Marker exposure ====================
        if (!oXML.FindChildElem("Marker_Exposure"))
        {
            return false;
        }
        oXML.IntoElem();
        
        if (!oXML.FindChildElem("Current"))
        {
            return false;
        }
        sCameraSettings.nMarkerExposure = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Min"))
        {
            return false;
        }
        sCameraSettings.nMarkerExposureMin = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Max"))
        {
            return false;
        }
        sCameraSettings.nMarkerExposureMax = atoi(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // Marker_Exposure

        // ==================== Marker threshold ====================
        if (!oXML.FindChildElem("Marker_Threshold"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("Current"))
        {
            return false;
        }
        sCameraSettings.nMarkerThreshold = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Min"))
        {
            return false;
        }
        sCameraSettings.nMarkerThresholdMin = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Max"))
        {
            return false;
        }
        sCameraSettings.nMarkerThresholdMax = atoi(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // Marker_Threshold

        // ==================== Position ====================
        if (!oXML.FindChildElem("Position"))
        {
            return false;
        }
        oXML.IntoElem();
        
        if (!oXML.FindChildElem("X"))
        {
            return false;
        }
        sCameraSettings.fPositionX = (float)atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Y"))
        {
            return false;
        }
        sCameraSettings.fPositionY = (float)atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Z"))
        {
            return false;
        }
        sCameraSettings.fPositionZ = (float)atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_1_1"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[0][0] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_2_1"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[1][0] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_3_1"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[2][0] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_1_2"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[0][1] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_2_2"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[1][1] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_3_2"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[2][1] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_1_3"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[0][2] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_2_3"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[1][2] = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Rot_3_3"))
        {
            return false;
        }
        sCameraSettings.fPositionRotMatrix[2][2] = (float)atof(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // Position


        if (!oXML.FindChildElem("Orientation"))
        {
            return false;
        }
        sCameraSettings.nOrientation = atoi(oXML.GetChildData().c_str());

        // ==================== Marker resolution ====================
        if (!oXML.FindChildElem("Marker_Res"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("Width"))
        {
            return false;
        }
        sCameraSettings.nMarkerResolutionWidth = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Height"))
        {
            return false;
        }
        sCameraSettings.nMarkerResolutionHeight = atoi(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // Marker_Res

        // ==================== Video resolution ====================
        if (!oXML.FindChildElem("Video_Res"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("Width"))
        {
            return false;
        }
        sCameraSettings.nVideoResolutionWidth = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Height"))
        {
            return false;
        }
        sCameraSettings.nVideoResolutionHeight = atoi(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // Video_Res

        // ==================== Marker FOV ====================
        if (!oXML.FindChildElem("Marker_FOV"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("Left"))
        {
            return false;
        }
        sCameraSettings.nMarkerFOVLeft = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Top"))
        {
            return false;
        }
        sCameraSettings.nMarkerFOVTop = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Right"))
        {
            return false;
        }
        sCameraSettings.nMarkerFOVRight = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Bottom"))
        {
            return false;
        }
        sCameraSettings.nMarkerFOVBottom = atoi(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // Marker_FOV

        // ==================== Video FOV ====================
        if (!oXML.FindChildElem("Video_FOV"))
        {
            return false;
        }
        oXML.IntoElem();

        if (!oXML.FindChildElem("Left"))
        {
            return false;
        }
        sCameraSettings.nVideoFOVLeft = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Top"))
        {
            return false;
        }
        sCameraSettings.nVideoFOVTop = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Right"))
        {
            return false;
        }
        sCameraSettings.nVideoFOVRight = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Bottom"))
        {
            return false;
        }
        sCameraSettings.nVideoFOVBottom = atoi(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // Video_FOV

        // ==================== Sync out ====================
        // Only available from protocol version 1.10 and later.
        for (int port = 0; port < 3; port++)
        {
            char syncOutStr[16];
            sprintf(syncOutStr, "Sync_Out%s", port == 0 ? "" : (port == 1 ? "2" : "_MT"));
            if (oXML.FindChildElem(syncOutStr))
            {
                oXML.IntoElem();

                if (port < 2)
                {
                    if (!oXML.FindChildElem("Mode"))
                    {
                        return false;
                    }
                    tStr = ToLower(oXML.GetChildData());
                    if (tStr == "shutter out")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeShutterOut;
                    }
                    else if (tStr == "multiplier")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeMultiplier;
                    }
                    else if (tStr == "divisor")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeDivisor;
                    }
                    else if (tStr == "camera independent")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeIndependentFreq;
                    }
                    else if (tStr == "measurement time")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeMeasurementTime;
                    }
                    else if (tStr == "continuous 100hz")
                    {
                        sCameraSettings.eSyncOutMode[port] = ModeFixed100Hz;
                    }
                    else
                    {
                        return false;
                    }

                    if (sCameraSettings.eSyncOutMode[port] == ModeMultiplier ||
                        sCameraSettings.eSyncOutMode[port] == ModeDivisor ||
                        sCameraSettings.eSyncOutMode[port] == ModeIndependentFreq)
                    {
                        if (!oXML.FindChildElem("Value"))
                        {
                            return false;
                        }
                        sCameraSettings.nSyncOutValue[port] = atoi(oXML.GetChildData().c_str());

                        if (!oXML.FindChildElem("Duty_Cycle"))
                        {
                            return false;
                        }
                        sCameraSettings.fSyncOutDutyCycle[port] = (float)atof(oXML.GetChildData().c_str());
                    }
                }
                if (port == 2 ||
                    (sCameraSettings.eSyncOutMode[port] != ModeFixed100Hz))
                {
                    if (!oXML.FindChildElem("Signal_Polarity"))
                    {
                        return false;
                    }
                    if (CompareNoCase(oXML.GetChildData(), "negative"))
                    {
                        sCameraSettings.bSyncOutNegativePolarity[port] = true;
                    }
                    else
                    {
                        sCameraSettings.bSyncOutNegativePolarity[port] = false;
                    }
                }
                oXML.OutOfElem(); // Sync_Out
            }
            else
            {
                sCameraSettings.eSyncOutMode[port] = ModeIndependentFreq;
                sCameraSettings.nSyncOutValue[port] = 0;
                sCameraSettings.fSyncOutDutyCycle[port] = 0;
                sCameraSettings.bSyncOutNegativePolarity[port] = false;
            }
        }

        if (oXML.FindChildElem("LensControl"))
        {
            oXML.IntoElem();
            if (oXML.FindChildElem("Focus"))
            {
                oXML.IntoElem();
                float focus;
                if (sscanf(oXML.GetAttrib("Value").c_str(), "%f", &focus) == 1)
                {
                    sCameraSettings.fFocus = focus;
                }
                oXML.OutOfElem();
            }
            if (oXML.FindChildElem("Aperture"))
            {
                oXML.IntoElem();
                float aperture;
                if (sscanf(oXML.GetAttrib("Value").c_str(), "%f", &aperture) == 1)
                {
                    sCameraSettings.fAperture = aperture;
                }
                oXML.OutOfElem();
            }
            oXML.OutOfElem();
        }
        else
        {
            sCameraSettings.fFocus = std::numeric_limits<float>::quiet_NaN();
            sCameraSettings.fAperture = std::numeric_limits<float>::quiet_NaN();
        }

        if (oXML.FindChildElem("AutoExposure"))
        {
            oXML.IntoElem();
            if (CompareNoCase(oXML.GetAttrib("Enabled"), "true"))
            {
                sCameraSettings.autoExposureEnabled = true;
            }
            float autoExposureCompensation;
            if (sscanf(oXML.GetAttrib("Compensation").c_str(), "%f", &autoExposureCompensation) == 1)
            {
                sCameraSettings.autoExposureCompensation = autoExposureCompensation;
            }
            oXML.OutOfElem();
        }
        else
        {
            sCameraSettings.autoExposureEnabled = false;
            sCameraSettings.autoExposureCompensation = std::numeric_limits<float>::quiet_NaN();
        }

        if (oXML.FindChildElem("AutoWhiteBalance"))
        {
            sCameraSettings.autoWhiteBalance = CompareNoCase(oXML.GetChildData().c_str(), "true") ? 1 : 0;
        }
        else
        {
            sCameraSettings.autoWhiteBalance = -1;
        }

        oXML.OutOfElem(); // Camera

        msGeneralSettings.vsCameras.push_back(sCameraSettings);
    }

    return true;
} // ReadGeneralSettings


bool ReadXmlFov(std::string name, CMarkup &oXML, CRTProtocol::SCalibrationFov &fov)
{
    if (!oXML.FindChildElem(name.c_str()))
    {
        return false;
    }
    fov.left = std::stoul(oXML.GetChildAttrib("left"));
    fov.top = std::stoul(oXML.GetChildAttrib("top"));
    fov.right = std::stoul(oXML.GetChildAttrib("right"));
    fov.bottom = std::stoul(oXML.GetChildAttrib("bottom"));

    return true;
}


bool CRTProtocol::ReadCalibrationSettings()
{
    if (!SendCommand("GetParameters Calibration"))
    {
        strcpy(maErrorStr, "GetParameters Calibration failed");
        return false;
    }

    return ReceiveCalibrationSettings();
}

bool CRTProtocol::ReceiveCalibrationSettings(int timeout)
{
    CRTPacket::EPacketType  eType;
    CMarkup                 oXML;
    std::string             tStr;
    SCalibration            settings;
    CNetwork::ResponseType  response;
    CRTPacket::EEvent       event = CRTPacket::EventNone;

    do 
    {
        response = Receive(eType, false, timeout);

        if (response == CNetwork::ResponseType::timeout)
        {
            strcat(maErrorStr, " Expected XML packet.");
            return false;
        }
        if (response == CNetwork::ResponseType::error)
        {
            return false;
        }

        if (eType == CRTPacket::PacketEvent)
        {
            mpoRTPacket->GetEvent(event);
        }
        else
        {
            event = CRTPacket::EventNone;
        }
    } while (event != CRTPacket::EventNone && event != CRTPacket::EventConnectionClosed);

    if (eType != CRTPacket::PacketXML)
    {
        if (event != CRTPacket::EventNone)
        {
            sprintf(maErrorStr, "Calibration aborted.");
        }
        else if (eType == CRTPacket::PacketError)
        {
            sprintf(maErrorStr, "%s.", mpoRTPacket->GetErrorString());
        }
        else
        {
            sprintf(maErrorStr, "GetParameters Calibration returned wrong packet type. Got type %d expected type 2.", eType);
        }
        return false;
    }
     
    oXML.SetDoc(mpoRTPacket->GetXMLString());

    if (!oXML.FindChildElem("calibration"))
    {
        sprintf(maErrorStr, "Missing calibration element");
        return false;
    }    
    oXML.IntoElem();

    try
    {
        std::string resultStr = ToLower(oXML.GetAttrib("calibrated"));

        settings.calibrated = (resultStr == "true");
        settings.source = oXML.GetAttrib("source");
        settings.created = oXML.GetAttrib("created");
        settings.qtm_version = oXML.GetAttrib("qtm-version");
        std::string typeStr = oXML.GetAttrib("type");
        if (typeStr == "regular")
        {
            settings.type = ECalibrationType::regular;
        }
        if (typeStr == "refine")
        {
            settings.type = ECalibrationType::refine;
        }
        if (typeStr == "fixed")
        {
            settings.type = ECalibrationType::fixed;
        }

        if (settings.type == ECalibrationType::refine)
        {
            settings.refit_residual = std::stod(oXML.GetAttrib("refit-residual"));
        }
        if (settings.type != ECalibrationType::fixed)
        {
            settings.wand_length = std::stod(oXML.GetAttrib("wandLength"));
            settings.max_frames = std::stoul(oXML.GetAttrib("maximumFrames"));
            settings.short_arm_end = std::stod(oXML.GetAttrib("shortArmEnd"));
            settings.long_arm_end = std::stod(oXML.GetAttrib("longArmEnd"));
            settings.long_arm_middle = std::stod(oXML.GetAttrib("longArmMiddle"));

            if (!oXML.FindChildElem("results"))
            {
                return false;
            }

            settings.result_std_dev = std::stod(oXML.GetChildAttrib("std-dev"));
            settings.result_min_max_diff = std::stod(oXML.GetChildAttrib("min-max-diff"));
            if (settings.type == ECalibrationType::refine)
            {
                settings.result_refit_residual = std::stod(oXML.GetChildAttrib("refit-residual"));
                settings.result_consecutive = std::stoul(oXML.GetChildAttrib("consecutive"));
            }
        }

        if (!oXML.FindChildElem("cameras"))
        {
            return false;
        }
        oXML.IntoElem();

        while (oXML.FindChildElem("camera"))
        {
            oXML.IntoElem();
            SCalibrationCamera camera;
            camera.active = std::stod(oXML.GetAttrib("active")) != 0;

            std::string calibratedStr = ToLower(oXML.GetAttrib("calibrated"));

            camera.calibrated = (calibratedStr == "true");
            camera.message = oXML.GetAttrib("message");

            camera.point_count = std::stoul(oXML.GetAttrib("point-count"));
            camera.avg_residual = std::stod(oXML.GetAttrib("avg-residual"));
            camera.serial = std::stoul(oXML.GetAttrib("serial"));
            camera.model = oXML.GetAttrib("model");
            camera.view_rotation = std::stoul(oXML.GetAttrib("viewrotation"));
            if (!ReadXmlFov("fov_marker", oXML, camera.fov_marker))
            {
                return false;
            }
            if (!ReadXmlFov("fov_marker_max", oXML, camera.fov_marker_max))
            {
                return false;
            }
            if (!ReadXmlFov("fov_video", oXML, camera.fov_video))
            {
                return false;
            }
            if (!ReadXmlFov("fov_video_max", oXML, camera.fov_video_max))
            {
                return false;
            }
            if (!oXML.FindChildElem("transform"))
            {
                return false;
            }
            camera.transform.x = std::stod(oXML.GetChildAttrib("x"));
            camera.transform.y = std::stod(oXML.GetChildAttrib("y"));
            camera.transform.z = std::stod(oXML.GetChildAttrib("z"));
            camera.transform.r11 = std::stod(oXML.GetChildAttrib("r11"));
            camera.transform.r12 = std::stod(oXML.GetChildAttrib("r12"));
            camera.transform.r13 = std::stod(oXML.GetChildAttrib("r13"));
            camera.transform.r21 = std::stod(oXML.GetChildAttrib("r21"));
            camera.transform.r22 = std::stod(oXML.GetChildAttrib("r22"));
            camera.transform.r23 = std::stod(oXML.GetChildAttrib("r23"));
            camera.transform.r31 = std::stod(oXML.GetChildAttrib("r31"));
            camera.transform.r32 = std::stod(oXML.GetChildAttrib("r32"));
            camera.transform.r33 = std::stod(oXML.GetChildAttrib("r33"));

            if (!oXML.FindChildElem("intrinsic"))
            {
                return false;
            }

            auto focalLength = oXML.GetChildAttrib("focallength");
            try
            {
                camera.intrinsic.focal_length = std::stod(focalLength);
            }
            catch (const std::invalid_argument&)
            {
                camera.intrinsic.focal_length = 0;
            }

            camera.intrinsic.sensor_min_u = std::stod(oXML.GetChildAttrib("sensorMinU"));
            camera.intrinsic.sensor_max_u = std::stod(oXML.GetChildAttrib("sensorMaxU"));
            camera.intrinsic.sensor_min_v = std::stod(oXML.GetChildAttrib("sensorMinV"));
            camera.intrinsic.sensor_max_v = std::stod(oXML.GetChildAttrib("sensorMaxV"));
            camera.intrinsic.focal_length_u = std::stod(oXML.GetChildAttrib("focalLengthU"));
            camera.intrinsic.focal_length_v = std::stod(oXML.GetChildAttrib("focalLengthV"));
            camera.intrinsic.center_point_u = std::stod(oXML.GetChildAttrib("centerPointU"));
            camera.intrinsic.center_point_v = std::stod(oXML.GetChildAttrib("centerPointV"));
            camera.intrinsic.skew = std::stod(oXML.GetChildAttrib("skew"));
            camera.intrinsic.radial_distortion_1 = std::stod(oXML.GetChildAttrib("radialDistortion1"));
            camera.intrinsic.radial_distortion_2 = std::stod(oXML.GetChildAttrib("radialDistortion2"));
            camera.intrinsic.radial_distortion_3 = std::stod(oXML.GetChildAttrib("radialDistortion3"));
            camera.intrinsic.tangental_distortion_1 = std::stod(oXML.GetChildAttrib("tangentalDistortion1"));
            camera.intrinsic.tangental_distortion_2 = std::stod(oXML.GetChildAttrib("tangentalDistortion2"));
            oXML.OutOfElem(); // camera
            settings.cameras.push_back(camera);
        }
        oXML.OutOfElem(); // cameras
    }
    catch (...)
    {
        return false;
    }

    oXML.OutOfElem(); // calibration

    mCalibrationSettings = settings;

    return true;
} // ReadCalibrationSettings


bool CRTProtocol::Read3DSettings(bool &bDataAvailable)
{
    CMarkup     oXML;
    std::string tStr;

    bDataAvailable = false;

    ms3DSettings.s3DLabels.clear();
    ms3DSettings.pCalibrationTime[0] = 0;

    if (!ReadSettings("3D", oXML))
    {
        return false;
    }

    if (!oXML.FindChildElem("The_3D"))
    {
        // No 3D data available.
        return true;
    }
    oXML.IntoElem();

    if (!oXML.FindChildElem("AxisUpwards"))
    {
        return false;
    }
    tStr = ToLower(oXML.GetChildData());

    if (tStr == "+x")
    {
        ms3DSettings.eAxisUpwards = XPos;
    }
    else if (tStr == "-x")
    {
        ms3DSettings.eAxisUpwards = XNeg;
    }
    else if (tStr == "+y")
    {
        ms3DSettings.eAxisUpwards = YPos;
    }
    else if (tStr == "-y")
    {
        ms3DSettings.eAxisUpwards = YNeg;
    }
    else if (tStr == "+z")
    {
        ms3DSettings.eAxisUpwards = ZPos;
    }
    else if (tStr == "-z")
    {
        ms3DSettings.eAxisUpwards = ZNeg;
    }
    else
    {
        return false;
    }

    if (!oXML.FindChildElem("CalibrationTime"))
    {
        return false;
    }
    tStr = oXML.GetChildData();
    strcpy(ms3DSettings.pCalibrationTime, tStr.c_str());

    if (!oXML.FindChildElem("Labels"))
    {
        return false;
    }
    unsigned int nNumberOfLabels = atoi(oXML.GetChildData().c_str());

    ms3DSettings.s3DLabels.resize(nNumberOfLabels);
    SSettings3DLabel sLabel;

    for (unsigned int iLabel = 0; iLabel < nNumberOfLabels; iLabel++)
    {
        if (oXML.FindChildElem("Label"))
        {
            oXML.IntoElem();
            if (oXML.FindChildElem("Name"))
            {
                sLabel.oName = oXML.GetChildData();
                if (oXML.FindChildElem("RGBColor"))
                {
                    sLabel.nRGBColor = atoi(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Trajectory_Type"))
                {
                    sLabel.type = oXML.GetChildData();
                }
                ms3DSettings.s3DLabels[iLabel] = sLabel;
            }
            oXML.OutOfElem();
        }
        else
        {
            return false;
        }
    }

    ms3DSettings.sBones.clear();
    if (oXML.FindChildElem("Bones"))
    {
        oXML.IntoElem();
        while (oXML.FindChildElem("Bone"))
        {
            oXML.IntoElem();
            SSettingsBone bone = { };
            bone.fromName = oXML.GetAttrib("From").c_str();
            bone.toName = oXML.GetAttrib("To").c_str();

            auto colorString = oXML.GetAttrib("Color");
            if (!colorString.empty())
            {
                bone.color = atoi(colorString.c_str());
            }
            ms3DSettings.sBones.push_back(bone);
            oXML.OutOfElem();
        }
        oXML.OutOfElem();
    }
    
    bDataAvailable = true;
    return true;
} // Read3DSettings

bool CRTProtocol::Read6DOFSettings(bool &bDataAvailable)
{
    CMarkup oXML;

    bDataAvailable = false;

    m6DOFSettings.clear();

    if (!ReadSettings("6D", oXML))
    {
        return false;
    }
    
    if (oXML.FindChildElem("The_6D"))
    {
        oXML.IntoElem();

        if (mnMajorVersion > 1 || mnMinorVersion > 20)
        {
            while (oXML.FindChildElem("Body"))
            {
                SSettings6DOFBody s6DOFBodySettings;
                SBodyPoint sBodyPoint;

                oXML.IntoElem();

                if (!oXML.FindChildElem("Name"))
                {
                    return false;
                }
                s6DOFBodySettings.name = oXML.GetChildData();

                if (!oXML.FindChildElem("Color"))
                {
                    return false;
                }
                uint32_t colorR = atoi(oXML.GetChildAttrib("R").c_str());
                uint32_t colorG = atoi(oXML.GetChildAttrib("G").c_str());
                uint32_t colorB = atoi(oXML.GetChildAttrib("B").c_str());
                s6DOFBodySettings.color = (colorR & 0xff) | ((colorG << 8) & 0xff00) | ((colorB << 16) & 0xff0000);

                if (!oXML.FindChildElem("MaximumResidual"))
                {
                    return false;
                }
                s6DOFBodySettings.maxResidual = (float)atof(oXML.GetChildData().c_str());

                if (!oXML.FindChildElem("MinimumMarkersInBody"))
                {
                    return false;
                }
                s6DOFBodySettings.minMarkersInBody = atoi(oXML.GetChildData().c_str());

                if (!oXML.FindChildElem("BoneLengthTolerance"))
                {
                    return false;
                }
                s6DOFBodySettings.boneLengthTolerance = (float)atof(oXML.GetChildData().c_str());

                if (!oXML.FindChildElem("Filter"))
                {
                    return false;
                }
                s6DOFBodySettings.filterPreset = oXML.GetChildAttrib("Preset");

                if (oXML.FindChildElem("Mesh"))
                {
                    oXML.IntoElem();
                    if (!oXML.FindChildElem("Name"))
                    {
                        return false;
                    }
                    s6DOFBodySettings.mesh.name = oXML.GetChildData();

                    if (!oXML.FindChildElem("Position"))
                    {
                        return false;
                    }
                    s6DOFBodySettings.mesh.position.fX = (float)atof(oXML.GetChildAttrib("X").c_str());
                    s6DOFBodySettings.mesh.position.fY = (float)atof(oXML.GetChildAttrib("Y").c_str());
                    s6DOFBodySettings.mesh.position.fZ = (float)atof(oXML.GetChildAttrib("Z").c_str());

                    if (!oXML.FindChildElem("Rotation"))
                    {
                        return false;
                    }
                    s6DOFBodySettings.mesh.rotation.fX = (float)atof(oXML.GetChildAttrib("X").c_str());
                    s6DOFBodySettings.mesh.rotation.fY = (float)atof(oXML.GetChildAttrib("Y").c_str());
                    s6DOFBodySettings.mesh.rotation.fZ = (float)atof(oXML.GetChildAttrib("Z").c_str());

                    if (!oXML.FindChildElem("Scale"))
                    {
                        return false;
                    }
                    s6DOFBodySettings.mesh.scale = (float)atof(oXML.GetChildData().c_str());

                    if (!oXML.FindChildElem("Opacity"))
                    {
                        return false;
                    }
                    s6DOFBodySettings.mesh.opacity = (float)atof(oXML.GetChildData().c_str());

                    oXML.OutOfElem(); // Mesh
                }

                if (oXML.FindChildElem("Points"))
                {
                    oXML.IntoElem();

                    while (oXML.FindChildElem("Point"))
                    {
                        sBodyPoint.fX = (float)atof(oXML.GetChildAttrib("X").c_str());
                        sBodyPoint.fY = (float)atof(oXML.GetChildAttrib("Y").c_str());
                        sBodyPoint.fZ = (float)atof(oXML.GetChildAttrib("Z").c_str());

                        sBodyPoint.virtual_ = (0 != atoi(oXML.GetChildAttrib("Virtual").c_str()));
                        sBodyPoint.physicalId = atoi(oXML.GetChildAttrib("PhysicalId").c_str());
                        sBodyPoint.name = oXML.GetChildAttrib("Name");
                        s6DOFBodySettings.points.push_back(sBodyPoint);
                    }
                    oXML.OutOfElem(); // Points
                }

                if (!oXML.FindChildElem("Data_origin"))
                {
                    return false;
                }
                s6DOFBodySettings.origin.type = (EOriginType)atoi(oXML.GetChildData().c_str());
                s6DOFBodySettings.origin.position.fX = (float)atof(oXML.GetChildAttrib("X").c_str());
                s6DOFBodySettings.origin.position.fY = (float)atof(oXML.GetChildAttrib("Y").c_str());
                s6DOFBodySettings.origin.position.fZ = (float)atof(oXML.GetChildAttrib("Z").c_str());
                s6DOFBodySettings.origin.relativeBody = atoi(oXML.GetChildAttrib("Relative_body").c_str());

                if (!oXML.FindChildElem("Data_orientation"))
                {
                    return false;
                }
                if (s6DOFBodySettings.origin.type != atoi(oXML.GetChildData().c_str()) ||
                    s6DOFBodySettings.origin.relativeBody != static_cast<uint32_t>(atoi(oXML.GetChildAttrib("Relative_body").c_str())))
                {
                    return false;
                }

                char tmpStr[10];
                for (uint32_t i = 0; i < 9; i++)
                {
                    sprintf(tmpStr, "R%u%u", (i / 3) + 1, (i % 3) + 1);
                    s6DOFBodySettings.origin.rotation[i] = (float)atof(oXML.GetChildAttrib(tmpStr).c_str());
                }
                m6DOFSettings.push_back(s6DOFBodySettings);
                oXML.OutOfElem(); // Body

                bDataAvailable = true;
            }
        }
        else
        {
            if (!oXML.FindChildElem("Bodies"))
            {
                return false;
            }
            int nBodies = atoi(oXML.GetChildData().c_str());
            SSettings6DOFBody s6DOFBodySettings;
            SBodyPoint sPoint;

            for (int iBody = 0; iBody < nBodies; iBody++)
            {
                if (!oXML.FindChildElem("Body"))
                {
                    return false;
                }
                oXML.IntoElem();

                if (!oXML.FindChildElem("Name"))
                {
                    return false;
                }
                s6DOFBodySettings.name = oXML.GetChildData();

                if (!oXML.FindChildElem("RGBColor"))
                {
                    return false;
                }
                s6DOFBodySettings.color = atoi(oXML.GetChildData().c_str());

                s6DOFBodySettings.points.clear();

                while (oXML.FindChildElem("Point"))
                {
                    oXML.IntoElem();
                    if (!oXML.FindChildElem("X"))
                    {
                        return false;
                    }
                    sPoint.fX = (float)atof(oXML.GetChildData().c_str());

                    if (!oXML.FindChildElem("Y"))
                    {
                        return false;
                    }
                    sPoint.fY = (float)atof(oXML.GetChildData().c_str());

                    if (!oXML.FindChildElem("Z"))
                    {
                        return false;
                    }
                    sPoint.fZ = (float)atof(oXML.GetChildData().c_str());

                    oXML.OutOfElem(); // Point
                    s6DOFBodySettings.points.push_back(sPoint);
                }
                m6DOFSettings.push_back(s6DOFBodySettings);
                oXML.OutOfElem(); // Body
            }
            if (mnMajorVersion > 1 || mnMinorVersion > 15)
            {
                if (oXML.FindChildElem("Euler"))
                {
                    oXML.IntoElem();
                    if (!oXML.FindChildElem("First"))
                    {
                        return false;
                    }
                    msGeneralSettings.eulerRotations[0] = oXML.GetChildData();
                    if (!oXML.FindChildElem("Second"))
                    {
                        return false;
                    }
                    msGeneralSettings.eulerRotations[1] = oXML.GetChildData();
                    if (!oXML.FindChildElem("Third"))
                    {
                        return false;
                    }
                    msGeneralSettings.eulerRotations[2] = oXML.GetChildData();
                    oXML.OutOfElem(); // Euler
                }
            }
            bDataAvailable = true;
        }
    }

    return true;
} // Read6DOFSettings

bool CRTProtocol::ReadGazeVectorSettings(bool &bDataAvailable)
{
    CMarkup oXML;

    bDataAvailable = false;

    mvsGazeVectorSettings.clear();

    if (!ReadSettings("GazeVector", oXML))
    {
        return false;
    }

    //
    // Read gaze vectors
    //
    if (!oXML.FindChildElem("Gaze_Vector"))
    {
        return true; // NO gaze vector data available.
    }
    oXML.IntoElem();

    std::string tGazeVectorName;

    int nGazeVectorCount = 0;

    while (oXML.FindChildElem("Vector")) 
    {
        oXML.IntoElem();

        if (!oXML.FindChildElem("Name"))
        {
            return false;
        }
        tGazeVectorName = oXML.GetChildData();

        float frequency = 0;
        if (oXML.FindChildElem("Frequency"))
        {
            frequency = (float)atof(oXML.GetChildData().c_str());
        }

        bool hwSync = false;
        ReadXmlBool(&oXML, "Hardware_Sync", hwSync);
        bool filter = false;
        ReadXmlBool(&oXML, "Filter", filter);

        mvsGazeVectorSettings.push_back({ tGazeVectorName, frequency, hwSync, filter });
        nGazeVectorCount++;
        oXML.OutOfElem(); // Vector
    }

    bDataAvailable = true;
    return true;
} // ReadGazeVectorSettings

bool CRTProtocol::ReadEyeTrackerSettings(bool &bDataAvailable)
{
    CMarkup oXML;

    bDataAvailable = false;

    mvsEyeTrackerSettings.clear();

    if (!ReadSettings("EyeTracker", oXML))
    {
        return false;
    }

    if (!oXML.FindChildElem("Eye_Tracker"))
    {
        return true; // NO eye tracker data available.
    }
    oXML.IntoElem();

    std::string tEyeTrackerName;

    int nEyeTrackerCount = 0;

    while (oXML.FindChildElem("Device"))
    {
        oXML.IntoElem();

        if (!oXML.FindChildElem("Name"))
        {
            return false;
        }
        tEyeTrackerName = oXML.GetChildData();

        float frequency = 0;
        if (oXML.FindChildElem("Frequency"))
        {
            frequency = (float)atof(oXML.GetChildData().c_str());
        }

        bool hwSync = false;
        ReadXmlBool(&oXML, "Hardware_Sync", hwSync);

        mvsEyeTrackerSettings.push_back({ tEyeTrackerName, frequency, hwSync });
        nEyeTrackerCount++;
        oXML.OutOfElem(); // Vector
    }

    bDataAvailable = true;
    return true;
} // ReadEyeTrackerSettings

bool CRTProtocol::ReadAnalogSettings(bool &bDataAvailable)
{
    CMarkup oXML;

    bDataAvailable = false;
    mvsAnalogDeviceSettings.clear();

    if (!ReadSettings("Analog", oXML))
    {
        return false;
    }

    if (!oXML.FindChildElem("Analog"))
    {
        // No analog data available.
        return true;
    }

    SAnalogDevice sAnalogDevice;

    oXML.IntoElem();

    if (mnMajorVersion == 1 && mnMinorVersion == 0)
    {
        sAnalogDevice.nDeviceID = 1;   // Always channel 1
        sAnalogDevice.oName = "AnalogDevice";
        if (!oXML.FindChildElem("Channels"))
        {
            return false;
        }
        sAnalogDevice.nChannels = atoi(oXML.GetChildData().c_str());
        if (!oXML.FindChildElem("Frequency"))
        {
            return false;
        }
        sAnalogDevice.nFrequency = atoi(oXML.GetChildData().c_str());
        if (!oXML.FindChildElem("Unit"))
        {
            return false;
        }
        sAnalogDevice.oUnit = oXML.GetChildData();
        if (!oXML.FindChildElem("Range"))
        {
            return false;
        }
        oXML.IntoElem();
        if (!oXML.FindChildElem("Min"))
        {
            return false;
        }
        sAnalogDevice.fMinRange = (float)atof(oXML.GetChildData().c_str());
        if (!oXML.FindChildElem("Max"))
        {
            return false;
        }
        sAnalogDevice.fMaxRange = (float)atof(oXML.GetChildData().c_str());
        mvsAnalogDeviceSettings.push_back(sAnalogDevice);
        bDataAvailable = true;
        return true;
    }
    else
    {
        while (oXML.FindChildElem("Device"))
        {
            sAnalogDevice.voLabels.clear();
            sAnalogDevice.voUnits.clear();
            oXML.IntoElem();
            if (!oXML.FindChildElem("Device_ID"))
            {
                oXML.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.nDeviceID = atoi(oXML.GetChildData().c_str());

            if (!oXML.FindChildElem("Device_Name"))
            {
                oXML.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.oName = oXML.GetChildData();

            if (!oXML.FindChildElem("Channels"))
            {
                oXML.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.nChannels = atoi(oXML.GetChildData().c_str());

            if (!oXML.FindChildElem("Frequency"))
            {
                oXML.OutOfElem(); // Device
                continue;
            }
            sAnalogDevice.nFrequency = atoi(oXML.GetChildData().c_str());

            if (mnMajorVersion == 1 && mnMinorVersion < 11)
            {
                if (!oXML.FindChildElem("Unit"))
                {
                    oXML.OutOfElem(); // Device
                    continue;
                }
                sAnalogDevice.oUnit = oXML.GetChildData();
            }
            if (!oXML.FindChildElem("Range"))
            {
                oXML.OutOfElem(); // Device
                continue;
            }
            oXML.IntoElem();

            if (!oXML.FindChildElem("Min"))
            {
                oXML.OutOfElem(); // Device
                oXML.OutOfElem(); // Range
                continue;
            }
            sAnalogDevice.fMinRange = (float)atof(oXML.GetChildData().c_str());

            if (!oXML.FindChildElem("Max"))
            {
                oXML.OutOfElem(); // Device
                oXML.OutOfElem(); // Range
                continue;
            }
            sAnalogDevice.fMaxRange = (float)atof(oXML.GetChildData().c_str());
            oXML.OutOfElem(); // Range

            if (mnMajorVersion == 1 && mnMinorVersion < 11)
            {
                for (unsigned int i = 0; i < sAnalogDevice.nChannels; i++)
                {
                    if (oXML.FindChildElem("Label"))
                    {
                        sAnalogDevice.voLabels.push_back(oXML.GetChildData());
                    }
                }
                if (sAnalogDevice.voLabels.size() != sAnalogDevice.nChannels)
                {
                    oXML.OutOfElem(); // Device
                    continue;
                }
            }
            else
            {
                while (oXML.FindChildElem("Channel"))
                {
                    oXML.IntoElem();
                    if (oXML.FindChildElem("Label"))
                    {
                        sAnalogDevice.voLabels.push_back(oXML.GetChildData());
                    }
                    if (oXML.FindChildElem("Unit"))
                    {
                        sAnalogDevice.voUnits.push_back(oXML.GetChildData());
                    }
                    oXML.OutOfElem(); // Channel
                }
                if (sAnalogDevice.voLabels.size() != sAnalogDevice.nChannels ||
                    sAnalogDevice.voUnits.size() != sAnalogDevice.nChannels)
                {
                    oXML.OutOfElem(); // Device
                    continue;
                }
            }
            oXML.OutOfElem(); // Device
            mvsAnalogDeviceSettings.push_back(sAnalogDevice);
            bDataAvailable = true;
        }
    }
    
    return true;
} // ReadAnalogSettings

bool CRTProtocol::ReadForceSettings(bool &bDataAvailable)
{
    CMarkup oXML;
    
    bDataAvailable = false;

    msForceSettings.vsForcePlates.clear();

    if (!ReadSettings("Force", oXML))
    {
        return false;
    }

    //
    // Read some force plate parameters
    //
    if (!oXML.FindChildElem("Force"))
    {
        return true;
    }

    oXML.IntoElem();

    SForcePlate sForcePlate;
    sForcePlate.bValidCalibrationMatrix = false;
    sForcePlate.nCalibrationMatrixRows = 6;
    sForcePlate.nCalibrationMatrixColumns = 6;

    if (!oXML.FindChildElem("Unit_Length"))
    {
        return false;
    }
    msForceSettings.oUnitLength = oXML.GetChildData();

    if (!oXML.FindChildElem("Unit_Force"))
    {
        return false;
    }
    msForceSettings.oUnitForce = oXML.GetChildData();

    int  iPlate = 1;
    while (oXML.FindChildElem("Plate"))
    {
        //
        // Get name and type of the plates
        //
        oXML.IntoElem(); // "Plate"
        if (oXML.FindChildElem("Force_Plate_Index")) // Version 1.7 and earlier.
        {
            sForcePlate.nID = atoi(oXML.GetChildData().c_str());
        }
        else if (oXML.FindChildElem("Plate_ID")) // Version 1.8 and later.
        {
            sForcePlate.nID = atoi(oXML.GetChildData().c_str());
        }
        else
        {
            return false;
        }

        if (oXML.FindChildElem("Analog_Device_ID"))
        {
            sForcePlate.nAnalogDeviceID = atoi(oXML.GetChildData().c_str());
        }
        else
        {
            sForcePlate.nAnalogDeviceID = 0;
        }

        if (!oXML.FindChildElem("Frequency"))
        {
            return false;
        }
        sForcePlate.nFrequency = atoi(oXML.GetChildData().c_str());

        if (oXML.FindChildElem("Type"))
        {
            sForcePlate.oType = oXML.GetChildData();
        }
        else
        {
            sForcePlate.oType = "unknown";
        }

        if (oXML.FindChildElem("Name"))
        {
            sForcePlate.oName = oXML.GetChildData();
        }
        else
        {
            sForcePlate.oName = CMarkup::Format("#%d", iPlate);
        }

        if (oXML.FindChildElem("Length"))
        {
            sForcePlate.fLength = (float)atof(oXML.GetChildData().c_str());
        }
        if (oXML.FindChildElem("Width"))
        {
            sForcePlate.fWidth  = (float)atof(oXML.GetChildData().c_str());
        }

        if (oXML.FindChildElem("Location"))
        {
            oXML.IntoElem();
            if (oXML.FindChildElem("Corner1"))
            {
                oXML.IntoElem();
                if (oXML.FindChildElem("X"))
                {
                    sForcePlate.asCorner[0].fX = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Y"))
                {
                    sForcePlate.asCorner[0].fY = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Z"))
                {
                    sForcePlate.asCorner[0].fZ = (float)atof(oXML.GetChildData().c_str());
                }
                oXML.OutOfElem();
            }
            if (oXML.FindChildElem("Corner2"))
            {
                oXML.IntoElem();
                if (oXML.FindChildElem("X"))
                {
                    sForcePlate.asCorner[1].fX = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Y"))
                {
                    sForcePlate.asCorner[1].fY = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Z"))
                {
                    sForcePlate.asCorner[1].fZ = (float)atof(oXML.GetChildData().c_str());
                }
                oXML.OutOfElem();
            }
            if (oXML.FindChildElem("Corner3"))
            {
                oXML.IntoElem();
                if (oXML.FindChildElem("X"))
                {
                    sForcePlate.asCorner[2].fX = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Y"))
                {
                    sForcePlate.asCorner[2].fY = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Z"))
                {
                    sForcePlate.asCorner[2].fZ = (float)atof(oXML.GetChildData().c_str());
                }
                oXML.OutOfElem();
            }
            if (oXML.FindChildElem("Corner4"))
            {
                oXML.IntoElem();
                if (oXML.FindChildElem("X"))
                {
                    sForcePlate.asCorner[3].fX = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Y"))
                {
                    sForcePlate.asCorner[3].fY = (float)atof(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("Z"))
                {
                    sForcePlate.asCorner[3].fZ = (float)atof(oXML.GetChildData().c_str());
                }
                oXML.OutOfElem();
            }
            oXML.OutOfElem();
        }

        if (oXML.FindChildElem("Origin"))
        {
            oXML.IntoElem();
            if (oXML.FindChildElem("X"))
            {
                sForcePlate.sOrigin.fX = (float)atof(oXML.GetChildData().c_str());
            }
            if (oXML.FindChildElem("Y"))
            {
                sForcePlate.sOrigin.fY = (float)atof(oXML.GetChildData().c_str());
            }
            if (oXML.FindChildElem("Z"))
            {
                sForcePlate.sOrigin.fZ = (float)atof(oXML.GetChildData().c_str());
            }
            oXML.OutOfElem();
        }

        sForcePlate.vChannels.clear();
        if (oXML.FindChildElem("Channels"))
        {
            oXML.IntoElem();
            SForceChannel sForceChannel;
            while (oXML.FindChildElem("Channel"))
            {
                oXML.IntoElem();
                if (oXML.FindChildElem("Channel_No"))
                {
                    sForceChannel.nChannelNumber = atoi(oXML.GetChildData().c_str());
                }
                if (oXML.FindChildElem("ConversionFactor"))
                {
                    sForceChannel.fConversionFactor = (float)atof(oXML.GetChildData().c_str());
                }
                sForcePlate.vChannels.push_back(sForceChannel);
                oXML.OutOfElem();
            }
            oXML.OutOfElem();
        }

        if (oXML.FindChildElem("Calibration_Matrix"))
        {
            oXML.IntoElem();
            int nRow = 0;

            if (mnMajorVersion == 1 && mnMinorVersion < 12)
            {
                char strRow[16];
                char strCol[16];
                sprintf(strRow, "Row%d", nRow + 1);
                while (oXML.FindChildElem(strRow))
                {
                    oXML.IntoElem();
                    int nCol = 0;
                    sprintf(strCol, "Col%d", nCol + 1);
                    while (oXML.FindChildElem(strCol))
                    {
                        sForcePlate.afCalibrationMatrix[nRow][nCol] = (float)atof(oXML.GetChildData().c_str());
                        nCol++;
                        sprintf(strCol, "Col%d", nCol + 1);
                    }
                    sForcePlate.nCalibrationMatrixColumns = nCol;

                    nRow++;
                    sprintf(strRow, "Row%d", nRow + 1);
                    oXML.OutOfElem(); // RowX
                }
            }
            else
            {
                //<Rows>
                if (oXML.FindChildElem("Rows"))
                {
                    oXML.IntoElem();

                    while (oXML.FindChildElem("Row"))
                    {
                        oXML.IntoElem();

                        //<Columns>
                        if (oXML.FindChildElem("Columns"))
                        {
                            oXML.IntoElem();

                            int nCol = 0;
                            while (oXML.FindChildElem("Column"))
                            {
                                sForcePlate.afCalibrationMatrix[nRow][nCol] = (float)atof(oXML.GetChildData().c_str());
                                nCol++;
                            }
                            sForcePlate.nCalibrationMatrixColumns = nCol;

                            nRow++;
                            oXML.OutOfElem(); // Columns
                        }
                        oXML.OutOfElem(); // Row
                    }
                    oXML.OutOfElem(); // Rows
                }
            }
            sForcePlate.nCalibrationMatrixRows = nRow;
            sForcePlate.bValidCalibrationMatrix = true;

            oXML.OutOfElem(); // "Calibration_Matrix"
        }
        oXML.OutOfElem(); // "Plate"

        bDataAvailable = true;
        msForceSettings.vsForcePlates.push_back(sForcePlate);
    }

    return true;
} // Read force settings

bool CRTProtocol::ReadImageSettings(bool &bDataAvailable)
{
    CMarkup oXML;

    bDataAvailable = false;

    mvsImageSettings.clear();

    if (!ReadSettings("Image", oXML))
    {
        return false;
    }

    //
    // Read some Image parameters
    //
    if (!oXML.FindChildElem("Image"))
    {
        return true;
    }
    oXML.IntoElem();

    while (oXML.FindChildElem("Camera"))
    {
        oXML.IntoElem();

        SImageCamera sImageCamera;

        if (!oXML.FindChildElem("ID"))
        {
            return false;
        }
        sImageCamera.nID = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Enabled"))
        {
            return false;
        }
        std::string tStr;
        tStr = ToLower(oXML.GetChildData());

        if (tStr == "true")
        {
            sImageCamera.bEnabled = true;
        }
        else
        {
            sImageCamera.bEnabled = false;
        }

        if (!oXML.FindChildElem("Format"))
        {
            return false;
        }
        tStr = ToLower(oXML.GetChildData());

        if (tStr == "rawgrayscale")
        {
            sImageCamera.eFormat = CRTPacket::FormatRawGrayscale;
        }
        else if (tStr == "rawbgr")
        {
            sImageCamera.eFormat = CRTPacket::FormatRawBGR;
        }
        else if (tStr == "jpg")
        {
            sImageCamera.eFormat = CRTPacket::FormatJPG;
        }
        else if (tStr == "png")
        {
            sImageCamera.eFormat = CRTPacket::FormatPNG;
        }
        else
        {
            return false;
        }

        if (!oXML.FindChildElem("Width"))
        {
            return false;
        }
        sImageCamera.nWidth = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Height"))
        {
            return false;
        }
        sImageCamera.nHeight = atoi(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Left_Crop"))
        {
            return false;
        }
        sImageCamera.fCropLeft = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Top_Crop"))
        {
            return false;
        }
        sImageCamera.fCropTop = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Right_Crop"))
        {
            return false;
        }
        sImageCamera.fCropRight = (float)atof(oXML.GetChildData().c_str());

        if (!oXML.FindChildElem("Bottom_Crop"))
        {
            return false;
        }
        sImageCamera.fCropBottom = (float)atof(oXML.GetChildData().c_str());

        oXML.OutOfElem(); // "Camera"

        mvsImageSettings.push_back(sImageCamera);
        bDataAvailable = true;
    }

    return true;
} // ReadImageSettings


bool CRTProtocol::ReadSkeletonSettings(bool &dataAvailable, bool skeletonGlobalData)
{
    CMarkup xml;

    dataAvailable = false;

    mSkeletonSettings.clear();
    mSkeletonSettingsHierarchical.clear();

    if (!ReadSettings(skeletonGlobalData ? "Skeleton:global" : "Skeleton", xml))
    {
        return false;
    }

    int segmentIndex;
    std::map<int, int> segmentIdIndexMap;
    xml.ResetPos();

    xml.FindElem();
    xml.IntoElem();

    if (xml.FindElem("Skeletons"))
    {
        xml.IntoElem();

        if (mnMajorVersion > 1 || mnMinorVersion > 20)
        {
            while (xml.FindElem("Skeleton"))
            {
                SSettingsSkeletonHierarchical skeletonHierarchical;
                SSettingsSkeleton skeleton;
                segmentIndex = 0;

                skeletonHierarchical.name = xml.GetAttrib("Name");
                skeleton.name = skeletonHierarchical.name;

                xml.IntoElem();

                if (xml.FindElem("Solver"))
                {
                    skeletonHierarchical.rootSegment.solver = xml.GetData();
                }

                if (xml.FindElem("Scale"))
                {
                    if (!ParseString(xml.GetData(), skeletonHierarchical.scale))
                    {
                        sprintf(maErrorStr, "Scale element parse error");
                        return false;
                    }
                }

                if (xml.FindElem("Segments"))
                {
                    xml.IntoElem();

                    std::function<void(SSettingsSkeletonSegmentHierarchical&, std::vector<SSettingsSkeletonSegment>&, const int)> recurseSegments =
                        [&](SSettingsSkeletonSegmentHierarchical& segmentHierarchical, std::vector<SSettingsSkeletonSegment>& segments, const int parentId)
                    {
                        segmentHierarchical.name = xml.GetAttrib("Name");
                        ParseString(xml.GetAttrib("ID"), segmentHierarchical.id);

                        segmentIdIndexMap[segmentHierarchical.id] = segmentIndex++;

                        xml.IntoElem();

                        if (xml.FindElem("Solver"))
                        {
                            segmentHierarchical.solver = xml.GetData();
                        }

                        if (xml.FindElem("Transform"))
                        {
                            xml.IntoElem();
                            segmentHierarchical.position = ReadXMLPosition(xml, "Position");
                            segmentHierarchical.rotation = ReadXMLRotation(xml, "Rotation");
                            xml.OutOfElem(); // Transform
                        }

                        if (xml.FindElem("DefaultTransform"))
                        {
                            xml.IntoElem();
                            segmentHierarchical.defaultPosition = ReadXMLPosition(xml, "Position");
                            segmentHierarchical.defaultRotation = ReadXMLRotation(xml, "Rotation");
                            xml.OutOfElem(); // DefaultTransform
                        }

                        if (xml.FindElem("DegreesOfFreedom"))
                        {
                            xml.IntoElem();
                            ReadXMLDegreesOfFreedom(xml, "RotationX", segmentHierarchical.degreesOfFreedom);
                            ReadXMLDegreesOfFreedom(xml, "RotationY", segmentHierarchical.degreesOfFreedom);
                            ReadXMLDegreesOfFreedom(xml, "RotationZ", segmentHierarchical.degreesOfFreedom);
                            ReadXMLDegreesOfFreedom(xml, "TranslationX", segmentHierarchical.degreesOfFreedom);
                            ReadXMLDegreesOfFreedom(xml, "TranslationY", segmentHierarchical.degreesOfFreedom);
                            ReadXMLDegreesOfFreedom(xml, "TranslationZ", segmentHierarchical.degreesOfFreedom);
                            xml.OutOfElem(); // DegreesOfFreedom
                        }

                        segmentHierarchical.endpoint = ReadXMLPosition(xml, "Endpoint");

                        if (xml.FindElem("Markers"))
                        {
                            xml.IntoElem();

                            while (xml.FindElem("Marker"))
                            {
                                SMarker marker;

                                marker.name = xml.GetAttrib("Name");
                                marker.weight = 1.0;

                                xml.IntoElem();
                                marker.position = segmentHierarchical.endpoint = ReadXMLPosition(xml, "Position");
                                if (xml.FindElem("Weight"))
                                {
                                    ParseString(xml.GetData(), marker.weight);
                                }

                                xml.OutOfElem(); // Marker

                                segmentHierarchical.markers.push_back(marker);
                            }

                            xml.OutOfElem(); // Markers
                        }

                        if (xml.FindElem("RigidBodies"))
                        {
                            xml.IntoElem();

                            while (xml.FindElem("RigidBody"))
                            {
                                SBody body;

                                body.name = xml.GetAttrib("Name");
                                body.weight = 1.0;

                                xml.IntoElem();

                                if (xml.FindElem("Transform"))
                                {
                                    xml.IntoElem();
                                    body.position = ReadXMLPosition(xml, "Position");
                                    body.rotation = ReadXMLRotation(xml, "Rotation");
                                    xml.OutOfElem(); // Transform
                                }
                                if (xml.FindElem("Weight"))
                                {
                                    ParseString(xml.GetData(), body.weight);
                                }

                                xml.OutOfElem(); // RigidBody

                                segmentHierarchical.bodies.push_back(body);
                            }

                            xml.OutOfElem(); // RigidBodies
                        }
                        SSettingsSkeletonSegment segment;
                        segment.name = segmentHierarchical.name;
                        segment.id = segmentHierarchical.id;
                        segment.parentId = parentId;
                        segment.parentIndex = (parentId != -1) ? segmentIdIndexMap[parentId] : -1;
                        segment.positionX = (float)segmentHierarchical.defaultPosition.x;
                        segment.positionY = (float)segmentHierarchical.defaultPosition.y;
                        segment.positionZ = (float)segmentHierarchical.defaultPosition.z;
                        segment.rotationX = (float)segmentHierarchical.defaultRotation.x;
                        segment.rotationY = (float)segmentHierarchical.defaultRotation.y;
                        segment.rotationZ = (float)segmentHierarchical.defaultRotation.z;
                        segment.rotationW = (float)segmentHierarchical.defaultRotation.w;

                        segments.push_back(segment);

                        while (xml.FindElem("Segment"))
                        {
                            SSettingsSkeletonSegmentHierarchical childSegment;
                            recurseSegments(childSegment, skeleton.segments, segmentHierarchical.id);
                            segmentHierarchical.segments.push_back(childSegment);
                        }
                        xml.OutOfElem();
                    };

                    if (xml.FindElem("Segment"))
                    {
                        recurseSegments(skeletonHierarchical.rootSegment, skeleton.segments, -1);
                    }
                    xml.OutOfElem(); // Segments
                }
                xml.OutOfElem(); // Skeleton
                mSkeletonSettingsHierarchical.push_back(skeletonHierarchical);
                mSkeletonSettings.push_back(skeleton);
            }
            dataAvailable = true;
        }
        else
        {
            while (xml.FindElem("Skeleton"))
            {
                SSettingsSkeleton skeleton;
                segmentIndex = 0;

                skeleton.name = xml.GetAttrib("Name");
                xml.IntoElem();

                while (xml.FindElem("Segment"))
                {
                    SSettingsSkeletonSegment segment;

                    segment.name = xml.GetAttrib("Name");
                    if (segment.name.size() == 0 || sscanf(xml.GetAttrib("ID").c_str(), "%u", &segment.id) != 1)
                    {
                        return false;
                    }

                    segmentIdIndexMap[segment.id] = segmentIndex++;

                    int parentId;
                    if (sscanf(xml.GetAttrib("Parent_ID").c_str(), "%d", &parentId) != 1)
                    {
                        segment.parentId = -1;
                        segment.parentIndex = -1;
                    }
                    else if (segmentIdIndexMap.count(parentId) > 0)
                    {
                        segment.parentId = parentId;
                        segment.parentIndex = segmentIdIndexMap[parentId];
                    }

                    xml.IntoElem();

                    if (xml.FindElem("Position"))
                    {
                        ParseString(xml.GetAttrib("X"), segment.positionX);
                        ParseString(xml.GetAttrib("Y"), segment.positionY);
                        ParseString(xml.GetAttrib("Z"), segment.positionZ);
                    }

                    if (xml.FindElem("Rotation"))
                    {
                        ParseString(xml.GetAttrib("X"), segment.rotationX);
                        ParseString(xml.GetAttrib("Y"), segment.rotationY);
                        ParseString(xml.GetAttrib("Z"), segment.rotationZ);
                        ParseString(xml.GetAttrib("W"), segment.rotationW);
                    }

                    skeleton.segments.push_back(segment);

                    xml.OutOfElem(); // Segment
                }

                mSkeletonSettings.push_back(skeleton);

                xml.OutOfElem(); // Skeleton
            }
        }
        xml.OutOfElem(); // Skeletons
        dataAvailable = true;
    }
    return true;
} // ReadSkeletonSettings


CRTProtocol::SPosition CRTProtocol::ReadXMLPosition(CMarkup& xml, const std::string& element)
{
    SPosition position;

    if (xml.FindElem(element.c_str()))
    {
        ParseString(xml.GetAttrib("X"), position.x);
        ParseString(xml.GetAttrib("Y"), position.y);
        ParseString(xml.GetAttrib("Z"), position.z);
        xml.ResetMainPos();
    }
    return position;
}


CRTProtocol::SRotation CRTProtocol::ReadXMLRotation(CMarkup& xml, const std::string& element)
{
    SRotation rotation;

    if (xml.FindElem(element.c_str()))
    {
        ParseString(xml.GetAttrib("X"), rotation.x);
        ParseString(xml.GetAttrib("Y"), rotation.y);
        ParseString(xml.GetAttrib("Z"), rotation.z);
        ParseString(xml.GetAttrib("W"), rotation.w);
        xml.ResetMainPos();
    }
    return rotation;
}


bool CRTProtocol::ReadXMLDegreesOfFreedom(CMarkup& xml, const std::string& element, std::vector<SDegreeOfFreedom>& degreesOfFreedom)
{
    SDegreeOfFreedom degreeOfFreedom;

    if (xml.FindElem(element.c_str()))
    {
        degreeOfFreedom.type = CRTProtocol::SkeletonStringToDof(element);
        ParseString(xml.GetAttrib("LowerBound"), degreeOfFreedom.lowerBound);
        ParseString(xml.GetAttrib("UpperBound"), degreeOfFreedom.upperBound);
        xml.IntoElem();
        if (xml.FindElem("Constraint"))
        {
            ParseString(xml.GetAttrib("LowerBound"), degreeOfFreedom.lowerBound);
            ParseString(xml.GetAttrib("UpperBound"), degreeOfFreedom.upperBound);
        }
        if (xml.FindElem("Couplings"))
        {
            xml.IntoElem();
            while (xml.FindElem("Coupling"))
            {
                CRTProtocol::SCoupling coupling;
                coupling.segment = xml.GetAttrib("Segment");
                auto dof = xml.GetAttrib("DegreeOfFreedom");
                coupling.degreeOfFreedom = CRTProtocol::SkeletonStringToDof(dof);
                ParseString(xml.GetAttrib("Coefficient"), coupling.coefficient);
                degreeOfFreedom.couplings.push_back(coupling);
            }
            xml.OutOfElem();
        }
        if (xml.FindElem("Goal"))
        {
            ParseString(xml.GetAttrib("Value"), degreeOfFreedom.goalValue);
            ParseString(xml.GetAttrib("Weight"), degreeOfFreedom.goalWeight);
        }
        xml.OutOfElem();
        xml.ResetMainPos();
    
        degreesOfFreedom.push_back(degreeOfFreedom);
        return true;
    }

    return false;
}


void CRTProtocol::GetGeneralSettings(
    unsigned int       &nCaptureFrequency, float &fCaptureTime,
    bool& bStartOnExtTrig, bool& startOnTrigNO, bool& startOnTrigNC, bool& startOnTrigSoftware,
    EProcessingActions &eProcessingActions, EProcessingActions &eRtProcessingActions, EProcessingActions &eReprocessingActions) const
{
    nCaptureFrequency = msGeneralSettings.nCaptureFrequency;
    fCaptureTime = msGeneralSettings.fCaptureTime;
    bStartOnExtTrig = msGeneralSettings.bStartOnExternalTrigger;
    startOnTrigNO = msGeneralSettings.bStartOnTrigNO;
    startOnTrigNC = msGeneralSettings.bStartOnTrigNC;
    startOnTrigSoftware = msGeneralSettings.bStartOnTrigSoftware;
    eProcessingActions = msGeneralSettings.eProcessingActions;
    eRtProcessingActions = msGeneralSettings.eRtProcessingActions;
    eReprocessingActions = msGeneralSettings.eReprocessingActions;
}


void CRTProtocol::GetSystemSettings(
    unsigned int       &nCaptureFrequency, float &fCaptureTime,
    bool& bStartOnExtTrig, bool& startOnTrigNO, bool& startOnTrigNC, bool& startOnTrigSoftware,
    EProcessingActions &eProcessingActions, EProcessingActions &eRtProcessingActions, EProcessingActions &eReprocessingActions) const
{
    GetGeneralSettings(nCaptureFrequency, fCaptureTime, bStartOnExtTrig, startOnTrigNO, startOnTrigNC, startOnTrigSoftware, eProcessingActions, eRtProcessingActions, eReprocessingActions);
}


void CRTProtocol::GetCalibrationSettings(SCalibration &calibrationSettings) const
{
    calibrationSettings = mCalibrationSettings;
}


// External time base settings only available in version 1.10 of the rt protocol and later
void CRTProtocol::GetExtTimeBaseSettings(
    bool &bEnabled,                    ESignalSource &eSignalSource,
    bool &bSignalModePeriodic,         unsigned int &nFreqMultiplier,
    unsigned int &nFreqDivisor,        unsigned int &nFreqTolerance,
    float &fNominalFrequency,          bool &bNegativeEdge,
    unsigned int &nSignalShutterDelay, float &fNonPeriodicTimeout) const
{
    bEnabled            = msGeneralSettings.sExternalTimebase.bEnabled;
    eSignalSource       = msGeneralSettings.sExternalTimebase.eSignalSource;
    bSignalModePeriodic = msGeneralSettings.sExternalTimebase.bSignalModePeriodic;
    nFreqMultiplier     = msGeneralSettings.sExternalTimebase.nFreqMultiplier;
    nFreqDivisor        = msGeneralSettings.sExternalTimebase.nFreqDivisor;
    nFreqTolerance      = msGeneralSettings.sExternalTimebase.nFreqTolerance;
    fNominalFrequency   = msGeneralSettings.sExternalTimebase.fNominalFrequency;
    bNegativeEdge       = msGeneralSettings.sExternalTimebase.bNegativeEdge;
    nSignalShutterDelay = msGeneralSettings.sExternalTimebase.nSignalShutterDelay;
    fNonPeriodicTimeout = msGeneralSettings.sExternalTimebase.fNonPeriodicTimeout;
}

void CRTProtocol::GetExtTimestampSettings(SSettingsGeneralExternalTimestamp& timestamp) const
{
    timestamp = msGeneralSettings.sTimestamp;
}

void CRTProtocol::GetEulerAngles(std::string& first, std::string& second, std::string& third) const
{
    first = msGeneralSettings.eulerRotations[0];
    second = msGeneralSettings.eulerRotations[1];
    third = msGeneralSettings.eulerRotations[2];
}

unsigned int CRTProtocol::GetCameraCount() const
{
    return (unsigned int)msGeneralSettings.vsCameras.size();
}

std::vector<CRTProtocol::SSettingsGeneralCamera> CRTProtocol::GetDevices() const
{
    return msGeneralSettings.vsCameras;
}


bool CRTProtocol::GetCameraSettings(
    unsigned int nCameraIndex, unsigned int &nID,     ECameraModel &eModel,
    bool         &bUnderwater, bool &bSupportsHwSync, unsigned int &nSerial, ECameraMode  &eMode) const
{
    if (nCameraIndex < msGeneralSettings.vsCameras.size())
    {
        nID             = msGeneralSettings.vsCameras[nCameraIndex].nID;
        eModel          = msGeneralSettings.vsCameras[nCameraIndex].eModel;
        bUnderwater     = msGeneralSettings.vsCameras[nCameraIndex].bUnderwater;
        bSupportsHwSync = msGeneralSettings.vsCameras[nCameraIndex].bSupportsHwSync;
        nSerial         = msGeneralSettings.vsCameras[nCameraIndex].nSerial;
        eMode           = msGeneralSettings.vsCameras[nCameraIndex].eMode;
        return true;
    }
    return false;
}


bool CRTProtocol::GetCameraMarkerSettings(
    unsigned int nCameraIndex,   unsigned int &nCurrentExposure, unsigned int &nMinExposure,
    unsigned int &nMaxExposure,  unsigned int &nCurrentThreshold,
    unsigned int &nMinThreshold, unsigned int &nMaxThreshold) const
{
    if (nCameraIndex < msGeneralSettings.vsCameras.size())
    {
        nCurrentExposure  = msGeneralSettings.vsCameras[nCameraIndex].nMarkerExposure;
        nMinExposure      = msGeneralSettings.vsCameras[nCameraIndex].nMarkerExposureMin;
        nMaxExposure      = msGeneralSettings.vsCameras[nCameraIndex].nMarkerExposureMax;
        nCurrentThreshold = msGeneralSettings.vsCameras[nCameraIndex].nMarkerThreshold;
        nMinThreshold     = msGeneralSettings.vsCameras[nCameraIndex].nMarkerThresholdMin;
        nMaxThreshold     = msGeneralSettings.vsCameras[nCameraIndex].nMarkerThresholdMax;
        return true;
    }
    return false;
}


bool CRTProtocol::GetCameraVideoSettings(
    unsigned int nCameraIndex,            EVideoResolution &eVideoResolution,
    EVideoAspectRatio &eVideoAspectRatio, unsigned int &nVideoFrequency,
    unsigned int &nCurrentExposure,       unsigned int &nMinExposure,
    unsigned int &nMaxExposure,           unsigned int &nCurrentFlashTime,
    unsigned int &nMinFlashTime,          unsigned int &nMaxFlashTime) const
{
    if (nCameraIndex < msGeneralSettings.vsCameras.size())
    {
        eVideoResolution   = msGeneralSettings.vsCameras[nCameraIndex].eVideoResolution;
        eVideoAspectRatio  = msGeneralSettings.vsCameras[nCameraIndex].eVideoAspectRatio;
        nVideoFrequency   = msGeneralSettings.vsCameras[nCameraIndex].nVideoFrequency;
        nCurrentExposure  = msGeneralSettings.vsCameras[nCameraIndex].nVideoExposure;
        nMinExposure      = msGeneralSettings.vsCameras[nCameraIndex].nVideoExposureMin;
        nMaxExposure      = msGeneralSettings.vsCameras[nCameraIndex].nVideoExposureMax;
        nCurrentFlashTime = msGeneralSettings.vsCameras[nCameraIndex].nVideoFlashTime;
        nMinFlashTime     = msGeneralSettings.vsCameras[nCameraIndex].nVideoFlashTimeMin;
        nMaxFlashTime     = msGeneralSettings.vsCameras[nCameraIndex].nVideoFlashTimeMax;
        return true;
    }
    return false;
}


bool CRTProtocol::GetCameraSyncOutSettings(
    unsigned int nCameraIndex, unsigned int portNumber, ESyncOutFreqMode &eSyncOutMode,
    unsigned int &nSyncOutValue, float      &fSyncOutDutyCycle,
    bool         &bSyncOutNegativePolarity) const
{
    if (nCameraIndex < msGeneralSettings.vsCameras.size())
    {
        if (portNumber == 1 || portNumber == 2)
        {
            eSyncOutMode = msGeneralSettings.vsCameras[nCameraIndex].eSyncOutMode[portNumber - 1];
            nSyncOutValue = msGeneralSettings.vsCameras[nCameraIndex].nSyncOutValue[portNumber - 1];
            fSyncOutDutyCycle = msGeneralSettings.vsCameras[nCameraIndex].fSyncOutDutyCycle[portNumber - 1];
        }
        if (portNumber > 0 && portNumber < 4)
        {
            bSyncOutNegativePolarity = msGeneralSettings.vsCameras[nCameraIndex].bSyncOutNegativePolarity[portNumber - 1];
        }
        else
        {
            return false;
        }
        return true;
    }
    return false;
}


bool CRTProtocol::GetCameraPosition(
    unsigned int nCameraIndex, SPoint &sPoint, float fvRotationMatrix[3][3]) const
{
    if (nCameraIndex < msGeneralSettings.vsCameras.size())
    {
        sPoint.fX = msGeneralSettings.vsCameras[nCameraIndex].fPositionX;
        sPoint.fY = msGeneralSettings.vsCameras[nCameraIndex].fPositionY;
        sPoint.fZ = msGeneralSettings.vsCameras[nCameraIndex].fPositionZ;
        memcpy(fvRotationMatrix, msGeneralSettings.vsCameras[nCameraIndex].fPositionRotMatrix, 9 * sizeof(float));
        return true;
    }
    return false;
}


bool CRTProtocol::GetCameraOrientation(
    unsigned int nCameraIndex, int &nOrientation) const
{
    if (nCameraIndex < msGeneralSettings.vsCameras.size())
    {
        nOrientation = msGeneralSettings.vsCameras[nCameraIndex].nOrientation;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraResolution(
    unsigned int nCameraIndex, unsigned int &nMarkerWidth, unsigned int &nMarkerHeight,
    unsigned int &nVideoWidth, unsigned int &nVideoHeight) const
{
    if (nCameraIndex < msGeneralSettings.vsCameras.size())
    {
        nMarkerWidth  = msGeneralSettings.vsCameras[nCameraIndex].nMarkerResolutionWidth;
        nMarkerHeight = msGeneralSettings.vsCameras[nCameraIndex].nMarkerResolutionHeight;
        nVideoWidth   = msGeneralSettings.vsCameras[nCameraIndex].nVideoResolutionWidth;
        nVideoHeight  = msGeneralSettings.vsCameras[nCameraIndex].nVideoResolutionHeight;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraFOV(
    unsigned int nCameraIndex,  unsigned int &nMarkerLeft,  unsigned int &nMarkerTop,
    unsigned int &nMarkerRight, unsigned int &nMarkerBottom,
    unsigned int &nVideoLeft,   unsigned int &nVideoTop,
    unsigned int &nVideoRight,  unsigned int &nVideoBottom) const
{
    if (nCameraIndex < msGeneralSettings.vsCameras.size())
    {
        nMarkerLeft   = msGeneralSettings.vsCameras[nCameraIndex].nMarkerFOVLeft;
        nMarkerTop    = msGeneralSettings.vsCameras[nCameraIndex].nMarkerFOVTop;
        nMarkerRight  = msGeneralSettings.vsCameras[nCameraIndex].nMarkerFOVRight;
        nMarkerBottom = msGeneralSettings.vsCameras[nCameraIndex].nMarkerFOVBottom;
        nVideoLeft    = msGeneralSettings.vsCameras[nCameraIndex].nVideoFOVLeft;
        nVideoTop     = msGeneralSettings.vsCameras[nCameraIndex].nVideoFOVTop;
        nVideoRight   = msGeneralSettings.vsCameras[nCameraIndex].nVideoFOVRight;
        nVideoBottom  = msGeneralSettings.vsCameras[nCameraIndex].nVideoFOVBottom;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraLensControlSettings(const unsigned int nCameraIndex, float* focus, float* aperture) const
{
    if (nCameraIndex < msGeneralSettings.vsCameras.size())
    {
        *focus = msGeneralSettings.vsCameras[nCameraIndex].fFocus;
        if (std::isnan(*focus))
            return false;
        *aperture = msGeneralSettings.vsCameras[nCameraIndex].fAperture;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraAutoExposureSettings(const unsigned int nCameraIndex, bool* autoExposureEnabled, float* autoExposureCompensation) const
{
    if (nCameraIndex < msGeneralSettings.vsCameras.size())
    {
        *autoExposureCompensation = msGeneralSettings.vsCameras[nCameraIndex].autoExposureCompensation;
        if (std::isnan(*autoExposureCompensation))
            return false;
        *autoExposureEnabled = msGeneralSettings.vsCameras[nCameraIndex].autoExposureEnabled;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraAutoWhiteBalance(const unsigned int nCameraIndex, bool* autoWhiteBalanceEnabled) const
{
    if (nCameraIndex < msGeneralSettings.vsCameras.size() && msGeneralSettings.vsCameras[nCameraIndex].autoWhiteBalance >= 0)
    {
        *autoWhiteBalanceEnabled = msGeneralSettings.vsCameras[nCameraIndex].autoWhiteBalance == 1;
        return true;
    }
    return false;
}

CRTProtocol::EAxis CRTProtocol::Get3DUpwardAxis() const
{
    return ms3DSettings.eAxisUpwards;
}

const char* CRTProtocol::Get3DCalibrated() const
{
    return ms3DSettings.pCalibrationTime;
}

unsigned int CRTProtocol::Get3DLabeledMarkerCount() const
{
    return (unsigned int)ms3DSettings.s3DLabels.size();
}

const char* CRTProtocol::Get3DLabelName(unsigned int nMarkerIndex) const
{
    if (nMarkerIndex < ms3DSettings.s3DLabels.size())
    {
        return ms3DSettings.s3DLabels[nMarkerIndex].oName.c_str();
    }
    return nullptr;
}

unsigned int CRTProtocol::Get3DLabelColor(unsigned int nMarkerIndex) const
{
    if (nMarkerIndex < ms3DSettings.s3DLabels.size())
    {
        return ms3DSettings.s3DLabels[nMarkerIndex].nRGBColor;
    }
    return 0;
}

const char* CRTProtocol::Get3DTrajectoryType(unsigned int nMarkerIndex) const
{
    if (nMarkerIndex < ms3DSettings.s3DLabels.size())
    {
        return ms3DSettings.s3DLabels[nMarkerIndex].type.c_str();
    }
    return 0;
}


unsigned int CRTProtocol::Get3DBoneCount() const
{
    return (unsigned int)ms3DSettings.sBones.size();
}

const char* CRTProtocol::Get3DBoneFromName(unsigned int boneIndex) const
{
    if (boneIndex < ms3DSettings.sBones.size())
    {
        return ms3DSettings.sBones[boneIndex].fromName.c_str();
    }
    return nullptr;
}

const char* CRTProtocol::Get3DBoneToName(unsigned int boneIndex) const
{
    if (boneIndex < ms3DSettings.sBones.size())
    {
        return ms3DSettings.sBones[boneIndex].toName.c_str();
    }
    return nullptr;
}

void CRTProtocol::Get6DOFEulerNames(std::string &first, std::string &second, std::string &third) const
{
    first = msGeneralSettings.eulerRotations[0];
    second = msGeneralSettings.eulerRotations[1];
    third = msGeneralSettings.eulerRotations[2];
}


unsigned int CRTProtocol::Get6DOFBodyCount() const
{
    return (unsigned int)m6DOFSettings.size();
}


const char* CRTProtocol::Get6DOFBodyName(unsigned int nBodyIndex) const
{
    if (nBodyIndex < m6DOFSettings.size())
    {
        return m6DOFSettings[nBodyIndex].name.c_str();
    }
    return nullptr;
}


unsigned int CRTProtocol::Get6DOFBodyColor(unsigned int nBodyIndex) const
{
    if (nBodyIndex < m6DOFSettings.size())
    {
        return m6DOFSettings[nBodyIndex].color;
    }
    return 0;
}


unsigned int CRTProtocol::Get6DOFBodyPointCount(unsigned int nBodyIndex) const
{
    if (nBodyIndex < m6DOFSettings.size())
    {
        return (unsigned int)m6DOFSettings.at(nBodyIndex).points.size();
    }
    return 0;
}


bool CRTProtocol::Get6DOFBodyPoint(unsigned int nBodyIndex, unsigned int nMarkerIndex, SPoint &sPoint) const
{
    if (nBodyIndex < m6DOFSettings.size())
    {
        if (nMarkerIndex < m6DOFSettings.at(nBodyIndex).points.size())
        {
            sPoint.fX = m6DOFSettings.at(nBodyIndex).points[nMarkerIndex].fX;
            sPoint.fY = m6DOFSettings.at(nBodyIndex).points[nMarkerIndex].fY;
            sPoint.fZ = m6DOFSettings.at(nBodyIndex).points[nMarkerIndex].fZ;
            return true;
        }
    }
    return false;
}


bool CRTProtocol::Get6DOFBodySettings(std::vector<SSettings6DOFBody>& settings)
{
    if (mnMajorVersion == 1 && mnMinorVersion < 21)
    {
        strcpy(maErrorStr, "Get6DOFBodySettings not available before protocol version 1.21");
        return false;
    }
    settings = m6DOFSettings;
    return true;
}

unsigned int CRTProtocol::GetGazeVectorCount() const
{
    return (unsigned int)mvsGazeVectorSettings.size();
}

const char* CRTProtocol::GetGazeVectorName(unsigned int nGazeVectorIndex) const
{
    if (nGazeVectorIndex < mvsGazeVectorSettings.size())
    {
        return mvsGazeVectorSettings[nGazeVectorIndex].name.c_str();
    }
    return nullptr;
}

float CRTProtocol::GetGazeVectorFrequency(unsigned int nGazeVectorIndex) const
{
    if (nGazeVectorIndex < mvsGazeVectorSettings.size())
    {
        return mvsGazeVectorSettings[nGazeVectorIndex].frequency;
    }
    return 0;
}

bool CRTProtocol::GetGazeVectorHardwareSyncUsed(unsigned int nGazeVectorIndex) const
{
    if (nGazeVectorIndex < mvsGazeVectorSettings.size())
    {
        return mvsGazeVectorSettings[nGazeVectorIndex].hwSync;
    }
    return false;
}

bool CRTProtocol::GetGazeVectorFilterUsed(unsigned int nGazeVectorIndex) const
{
    if (nGazeVectorIndex < mvsGazeVectorSettings.size())
    {
        return mvsGazeVectorSettings[nGazeVectorIndex].filter;
    }
    return false;
}


unsigned int CRTProtocol::GetEyeTrackerCount() const
{
    return (unsigned int)mvsEyeTrackerSettings.size();
}

const char* CRTProtocol::GetEyeTrackerName(unsigned int nEyeTrackerIndex) const
{
    if (nEyeTrackerIndex < mvsEyeTrackerSettings.size())
    {
        return mvsEyeTrackerSettings[nEyeTrackerIndex].name.c_str();
    }
    return nullptr;
}

float CRTProtocol::GetEyeTrackerFrequency(unsigned int nEyeTrackerIndex) const
{
    if (nEyeTrackerIndex < mvsEyeTrackerSettings.size())
    {
        return mvsEyeTrackerSettings[nEyeTrackerIndex].frequency;
    }
    return 0;
}

bool CRTProtocol::GetEyeTrackerHardwareSyncUsed(unsigned int nEyeTrackerIndex) const
{
    if (nEyeTrackerIndex < mvsEyeTrackerSettings.size())
    {
        return mvsEyeTrackerSettings[nEyeTrackerIndex].hwSync;
    }
    return false;
}


unsigned int CRTProtocol::GetAnalogDeviceCount() const
{
    return (unsigned int)mvsAnalogDeviceSettings.size();
}


bool CRTProtocol::GetAnalogDevice(unsigned int nDeviceIndex, unsigned int &nDeviceID, unsigned int &nChannels,
                                  char* &pName, unsigned int &nFrequency, char* &pUnit,
                                  float &fMinRange, float &fMaxRange) const
{
    if (nDeviceIndex < mvsAnalogDeviceSettings.size())
    {
        nDeviceID  = mvsAnalogDeviceSettings.at(nDeviceIndex).nDeviceID;
        pName      = (char*)mvsAnalogDeviceSettings.at(nDeviceIndex).oName.c_str();
        nChannels  = mvsAnalogDeviceSettings.at(nDeviceIndex).nChannels;
        nFrequency = mvsAnalogDeviceSettings.at(nDeviceIndex).nFrequency;
        pUnit      = (char*)mvsAnalogDeviceSettings.at(nDeviceIndex).oUnit.c_str();
        fMinRange  = mvsAnalogDeviceSettings.at(nDeviceIndex).fMinRange;
        fMaxRange  = mvsAnalogDeviceSettings.at(nDeviceIndex).fMaxRange;

        return true;
    }
    return false;
}


const char* CRTProtocol::GetAnalogLabel(unsigned int nDeviceIndex, unsigned int nChannelIndex) const
{
    if (nDeviceIndex < mvsAnalogDeviceSettings.size())
    {
        if (nChannelIndex < mvsAnalogDeviceSettings.at(nDeviceIndex).voLabels.size())
        {
            return mvsAnalogDeviceSettings.at(nDeviceIndex).voLabels.at(nChannelIndex).c_str();
        }
    }
    return nullptr;
}


const char* CRTProtocol::GetAnalogUnit(unsigned int nDeviceIndex, unsigned int nChannelIndex) const
{
    if (nDeviceIndex < mvsAnalogDeviceSettings.size())
    {
        if (nChannelIndex < mvsAnalogDeviceSettings.at(nDeviceIndex).voUnits.size())
        {
            return mvsAnalogDeviceSettings.at(nDeviceIndex).voUnits.at(nChannelIndex).c_str();
        }
    }
    return nullptr;
}


void CRTProtocol::GetForceUnits(char* &pLength, char* &pForce) const
{
    pLength = (char*)msForceSettings.oUnitLength.c_str();
    pForce  = (char*)msForceSettings.oUnitForce.c_str();
}


unsigned int CRTProtocol::GetForcePlateCount() const
{
    return (unsigned int)msForceSettings.vsForcePlates.size();
}


bool CRTProtocol::GetForcePlate(unsigned int nPlateIndex, unsigned int &nID, unsigned int &nAnalogDeviceID,
                                unsigned int &nFrequency, char* &pType, char* &pName, float &fLength, float &fWidth) const
{
    if (nPlateIndex < msForceSettings.vsForcePlates.size())
    {
        nID             = msForceSettings.vsForcePlates[nPlateIndex].nID;
        nAnalogDeviceID = msForceSettings.vsForcePlates[nPlateIndex].nAnalogDeviceID;
        nFrequency      = msForceSettings.vsForcePlates[nPlateIndex].nFrequency;
        pType           = (char*)msForceSettings.vsForcePlates[nPlateIndex].oType.c_str();
        pName           = (char*)msForceSettings.vsForcePlates[nPlateIndex].oName.c_str();
        fLength         = msForceSettings.vsForcePlates[nPlateIndex].fLength;
        fWidth          = msForceSettings.vsForcePlates[nPlateIndex].fWidth;
        return true;
    }
    return false;
}


bool CRTProtocol::GetForcePlateLocation(unsigned int nPlateIndex, SPoint sCorner[4]) const
{
    if (nPlateIndex < msForceSettings.vsForcePlates.size())
    {
        memcpy(sCorner, msForceSettings.vsForcePlates[nPlateIndex].asCorner, 3 * 4 * sizeof(float));
        return true;
    }
    return false;
}


bool CRTProtocol::GetForcePlateOrigin(unsigned int nPlateIndex, SPoint &sOrigin) const
{
    if (nPlateIndex < msForceSettings.vsForcePlates.size())
    {
        sOrigin = msForceSettings.vsForcePlates[nPlateIndex].sOrigin;
        return true;
    }
    return false;
}


unsigned int CRTProtocol::GetForcePlateChannelCount(unsigned int nPlateIndex) const
{
    if (nPlateIndex < msForceSettings.vsForcePlates.size())
    {
        return (unsigned int)msForceSettings.vsForcePlates[nPlateIndex].vChannels.size();
    }
    return 0;
}


bool CRTProtocol::GetForcePlateChannel(unsigned int nPlateIndex, unsigned int nChannelIndex,
                                       unsigned int &nChannelNumber, float &fConversionFactor) const
{
    if (nPlateIndex < msForceSettings.vsForcePlates.size())
    {
        if (nChannelIndex < msForceSettings.vsForcePlates[nPlateIndex].vChannels.size())
        {
            nChannelNumber    = msForceSettings.vsForcePlates[nPlateIndex].vChannels[nChannelIndex].nChannelNumber;
            fConversionFactor = msForceSettings.vsForcePlates[nPlateIndex].vChannels[nChannelIndex].fConversionFactor;
            return true;
        }
    }
    return false;
}


bool CRTProtocol::GetForcePlateCalibrationMatrix(unsigned int nPlateIndex, float fvCalMatrix[12][12], unsigned int* rows, unsigned int* columns) const
{
    if (nPlateIndex < msForceSettings.vsForcePlates.size())
    {
        if (msForceSettings.vsForcePlates[nPlateIndex].bValidCalibrationMatrix)
        {
            *rows = msForceSettings.vsForcePlates[nPlateIndex].nCalibrationMatrixRows;
            *columns = msForceSettings.vsForcePlates[nPlateIndex].nCalibrationMatrixColumns;
            memcpy(
                fvCalMatrix,
                msForceSettings.vsForcePlates[nPlateIndex].afCalibrationMatrix,
                msForceSettings.vsForcePlates[nPlateIndex].nCalibrationMatrixRows * msForceSettings.vsForcePlates[nPlateIndex].nCalibrationMatrixColumns * sizeof(float));
            return true;
        }
    }
    return false;
}


unsigned int CRTProtocol::GetImageCameraCount() const
{
    return (unsigned int)mvsImageSettings.size();
}


bool CRTProtocol::GetImageCamera(unsigned int nCameraIndex, unsigned int &nCameraID, bool &bEnabled,
                                 CRTPacket::EImageFormat &eFormat, unsigned int &nWidth, unsigned int &nHeight,
                                 float &fCropLeft, float &fCropTop, float &fCropRight, float &fCropBottom) const
{
    if (nCameraIndex < mvsImageSettings.size())
    {
        nCameraID   = mvsImageSettings[nCameraIndex].nID;
        bEnabled    = mvsImageSettings[nCameraIndex].bEnabled;
        eFormat     = mvsImageSettings[nCameraIndex].eFormat;
        nWidth      = mvsImageSettings[nCameraIndex].nWidth;
        nHeight     = mvsImageSettings[nCameraIndex].nHeight;
        fCropLeft   = mvsImageSettings[nCameraIndex].fCropLeft;
        fCropTop    = mvsImageSettings[nCameraIndex].fCropTop;
        fCropRight  = mvsImageSettings[nCameraIndex].fCropRight;
        fCropBottom = mvsImageSettings[nCameraIndex].fCropBottom;
        return true;
    }
    return false;
}

unsigned int CRTProtocol::GetSkeletonCount() const
{
    return (unsigned int)mSkeletonSettings.size();
}


const char* CRTProtocol::GetSkeletonName(unsigned int skeletonIndex)
{
    if (skeletonIndex < mSkeletonSettings.size())
    {
        return (char*)mSkeletonSettings[skeletonIndex].name.c_str();
    }
    return nullptr;
}


unsigned int CRTProtocol::GetSkeletonSegmentCount(unsigned int skeletonIndex)
{
    if (skeletonIndex < mSkeletonSettings.size())
    {
        return static_cast<long unsigned>(mSkeletonSettings[skeletonIndex].segments.size());
    }
    return 0;
}

bool CRTProtocol::GetSkeleton(unsigned int skeletonIndex, SSettingsSkeleton* skeleton)
{
    if (skeleton == nullptr)
        return false;

    if (skeletonIndex < mSkeletonSettings.size())
    {
        *skeleton = mSkeletonSettings[skeletonIndex];
        return true;
    }
    return false;
}

bool CRTProtocol::GetSkeletonSegment(unsigned int skeletonIndex, unsigned int segmentIndex, SSettingsSkeletonSegment* segment)
{
    if (segment == nullptr)
        return false;

    if (skeletonIndex < mSkeletonSettings.size())
    {
        if (segmentIndex < mSkeletonSettings[skeletonIndex].segments.size())
        {
            *segment = mSkeletonSettings[skeletonIndex].segments[segmentIndex];
            return true;
        }
    }
    return false;
}

bool CRTProtocol::GetSkeleton(unsigned int skeletonIndex, SSettingsSkeletonHierarchical& skeleton)
{
    if (skeletonIndex < mSkeletonSettingsHierarchical.size())
    {
        skeleton = mSkeletonSettingsHierarchical[skeletonIndex];
        return true;
    }
    return false;
}

void CRTProtocol::GetSkeletons(std::vector<SSettingsSkeletonHierarchical>& skeletons)
{
    skeletons = mSkeletonSettingsHierarchical;
}

bool CRTProtocol::SetSystemSettings(
    const unsigned int* pnCaptureFrequency, const float* pfCaptureTime,
    const bool* pbStartOnExtTrig, const bool* startOnTrigNO, const bool* startOnTrigNC, const bool* startOnTrigSoftware,
    const EProcessingActions* peProcessingActions, const EProcessingActions* peRtProcessingActions, const EProcessingActions* peReprocessingActions)
{
    return SetGeneralSettings(pnCaptureFrequency, pfCaptureTime, pbStartOnExtTrig, startOnTrigNO, startOnTrigNC, startOnTrigSoftware, peProcessingActions, peRtProcessingActions, peReprocessingActions);
}

bool CRTProtocol::SetGeneralSettings(
    const unsigned int* pnCaptureFrequency, const float* pfCaptureTime,
    const bool* pbStartOnExtTrig, const bool* startOnTrigNO, const bool* startOnTrigNC, const bool* startOnTrigSoftware,
    const EProcessingActions* peProcessingActions, const EProcessingActions* peRtProcessingActions, const EProcessingActions* peReprocessingActions)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    if (pnCaptureFrequency)
    {
        AddXMLElementUnsignedInt(&oXML, "Frequency", pnCaptureFrequency);
    }
    if (pfCaptureTime)
    {
        AddXMLElementFloat(&oXML, "Capture_Time", pfCaptureTime, 3);
    }
    if (pbStartOnExtTrig)
    {
        AddXMLElementBool(&oXML, "Start_On_External_Trigger", pbStartOnExtTrig);
        if (mnMajorVersion > 1 || mnMinorVersion > 14)
        {
            AddXMLElementBool(&oXML, "Start_On_Trigger_NO", startOnTrigNO);
            AddXMLElementBool(&oXML, "Start_On_Trigger_NC", startOnTrigNC);
            AddXMLElementBool(&oXML, "Start_On_Trigger_Software", startOnTrigSoftware);
        }
    }

    const char* processings[3] = { "Processing_Actions", "RealTime_Processing_Actions", "Reprocessing_Actions" };
    const EProcessingActions* processingActions[3] = { peProcessingActions, peRtProcessingActions, peReprocessingActions };

    auto actionsCount = (mnMajorVersion > 1 || mnMinorVersion > 13) ? 3 : 1;

    for (auto i = 0; i < actionsCount; i++)
    {
        if (processingActions[i])
        {
            oXML.AddElem(processings[i]);
            oXML.IntoElem();

            if (mnMajorVersion > 1 || mnMinorVersion > 13)
            {
                AddXMLElementBool(&oXML, "PreProcessing2D", (*processingActions[i] & ProcessingPreProcess2D) != 0);
            }
            if (*processingActions[i] & ProcessingTracking2D && i != 1) // i != 1 => Not RtProcessingSettings
            {
                oXML.AddElem("Tracking", "2D");
            }
            else if (*processingActions[i] & ProcessingTracking3D)
            {
                oXML.AddElem("Tracking", "3D");
            }
            else
            {
                oXML.AddElem("Tracking", "False");
            }
            if (i != 1) //Not RtProcessingSettings
            {
                AddXMLElementBool(&oXML, "TwinSystemMerge", (*processingActions[i] & ProcessingTwinSystemMerge) != 0);
                AddXMLElementBool(&oXML, "SplineFill", (*processingActions[i] & ProcessingSplineFill) != 0);
            }
            AddXMLElementBool(&oXML, "AIM", (*processingActions[i] & ProcessingAIM) != 0);
            AddXMLElementBool(&oXML, "Track6DOF", (*processingActions[i] & Processing6DOFTracking) != 0);
            AddXMLElementBool(&oXML, "ForceData", (*processingActions[i] & ProcessingForceData) != 0);
            AddXMLElementBool(&oXML, "GazeVector", (*processingActions[i] & ProcessingGazeVector) != 0);
            if (i != 1) //Not RtProcessingSettings
            {
                AddXMLElementBool(&oXML, "ExportTSV", (*processingActions[i] & ProcessingExportTSV) != 0);
                AddXMLElementBool(&oXML, "ExportC3D", (*processingActions[i] & ProcessingExportC3D) != 0);
                AddXMLElementBool(&oXML, "ExportMatlabFile", (*processingActions[i] & ProcessingExportMatlabFile) != 0);
                AddXMLElementBool(&oXML, "ExportAviFile", (*processingActions[i] & ProcessingExportAviFile) != 0);
            }
            oXML.OutOfElem(); // Processing_Actions
        }
    }
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    if (SendXML(oXML.GetDoc().c_str()))
    {
        return true;
    }

    return false;
} // SetGeneral


bool CRTProtocol::SetExtTimeBaseSettings(
    const bool*         pbEnabled,            const ESignalSource* peSignalSource,
    const bool*         pbSignalModePeriodic, const unsigned int*  pnFreqMultiplier,
    const unsigned int* pnFreqDivisor,        const unsigned int*  pnFreqTolerance,
    const float*        pfNominalFrequency,   const bool*          pbNegativeEdge,
    const unsigned int* pnSignalShutterDelay, const float*         pfNonPeriodicTimeout)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();
    oXML.AddElem("External_Time_Base");
    oXML.IntoElem();

    AddXMLElementBool(&oXML, "Enabled", pbEnabled);

    if (peSignalSource)
    {
        switch (*peSignalSource)
        {
            case SourceControlPort :
                oXML.AddElem("Signal_Source", "Control port");
                break;
            case SourceIRReceiver :
                oXML.AddElem("Signal_Source", "IR receiver");
                break;
            case SourceSMPTE :
                oXML.AddElem("Signal_Source", "SMPTE");
                break;
            case SourceVideoSync :
                oXML.AddElem("Signal_Source", "Video sync");
                break;
            case SourceIRIG:
                oXML.AddElem("Signal_Source", "IRIG");
                break;
        }
    }

    AddXMLElementBool(&oXML,        "Signal_Mode",          pbSignalModePeriodic, "Periodic", "Non-periodic");
    AddXMLElementUnsignedInt(&oXML, "Frequency_Multiplier", pnFreqMultiplier);
    AddXMLElementUnsignedInt(&oXML, "Frequency_Divisor",    pnFreqDivisor);
    AddXMLElementUnsignedInt(&oXML, "Frequency_Tolerance",  pnFreqTolerance);

    if (pfNominalFrequency)
    {
        if (*pfNominalFrequency < 0)
        {
            oXML.AddElem("Nominal_Frequency", "None");
        }
        else
        {
            AddXMLElementFloat(&oXML, "Nominal_Frequency", pfNominalFrequency, 3);
        }
    }

    AddXMLElementBool(&oXML, "Signal_Edge", pbNegativeEdge, "Negative", "Positive");
    AddXMLElementUnsignedInt(&oXML, "Signal_Shutter_Delay", pnSignalShutterDelay);
    AddXMLElementFloat(&oXML, "Non_Periodic_Timeout", pfNonPeriodicTimeout, 3);

    oXML.OutOfElem(); // External_Time_Base            
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    if (SendXML(oXML.GetDoc().c_str()))
    {
        return true;
    }

    return false;
} // SetGeneralExtTimeBase


bool CRTProtocol::SetExtTimestampSettings(const CRTProtocol::SSettingsGeneralExternalTimestamp& timestampSettings)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();
    oXML.AddElem("External_Timestamp");
    oXML.IntoElem();

    AddXMLElementBool(&oXML, "Enabled", timestampSettings.bEnabled);

    switch (timestampSettings.nType)
    {
    default:
    case CRTProtocol::Timestamp_SMPTE:
        oXML.AddElem("Type", "SMPTE");
        break;
    case CRTProtocol::Timestamp_IRIG:
        oXML.AddElem("Type", "IRIG");
        break;
    case CRTProtocol::Timestamp_CameraTime:
        oXML.AddElem("Type", "CameraTime");
        break;
    }
    AddXMLElementUnsignedInt(&oXML, "Frequency", timestampSettings.nFrequency);

    oXML.OutOfElem(); // Timestamp
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    if (SendXML(oXML.GetDoc().c_str()))
    {
        return true;
    }
    return false;
}


// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraSettings(
    const unsigned int nCameraID,        const ECameraMode* peMode,
    const float*       pfMarkerExposure, const float*       pfMarkerThreshold,
    const int*         pnOrientation)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &nCameraID);

    if (peMode)
    {
        switch (*peMode)
        {
            case ModeMarker :
                oXML.AddElem("Mode", "Marker");
                break;
            case ModeMarkerIntensity :
                oXML.AddElem("Mode", "Marker Intensity");
                break;
            case ModeVideo :
                oXML.AddElem("Mode", "Video");
                break;
        }
    }
    AddXMLElementFloat(&oXML, "Marker_Exposure",  pfMarkerExposure);
    AddXMLElementFloat(&oXML, "Marker_Threshold", pfMarkerThreshold);
    AddXMLElementInt(&oXML,   "Orientation",      pnOrientation);

    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    if (SendXML(oXML.GetDoc().c_str()))
    {
        return true;
    }

    return false;
} // SetGeneralCamera


// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraVideoSettings(
    const unsigned int nCameraID,                const EVideoResolution* eVideoResolution,
    const EVideoAspectRatio* eVideoAspectRatio, const unsigned int* pnVideoFrequency,
    const float* pfVideoExposure,                const float* pfVideoFlashTime)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &nCameraID);
    if (eVideoResolution)
    {
        switch (*eVideoResolution)
        {
            case VideoResolution1080p:
                oXML.AddElem("Video_Resolution", "1080p");
                break;
            case VideoResolution720p:
                oXML.AddElem("Video_Resolution", "720p");
                break;
            case VideoResolution540p:
                oXML.AddElem("Video_Resolution", "540p");
                break;
            case VideoResolution480p:
                oXML.AddElem("Video_Resolution", "480p");
                break;
            case VideoResolutionNone:
                break;
        }
    }
    if (eVideoAspectRatio)
    {
        switch (*eVideoAspectRatio)
        {
            case VideoAspectRatio16x9:
                oXML.AddElem("Video_Aspect_Ratio", "16x9");
                break;
            case VideoAspectRatio4x3:
                oXML.AddElem("Video_Aspect_Ratio", "4x3");
                break;
            case VideoAspectRatio1x1:
                oXML.AddElem("Video_Aspect_Ratio", "1x1");
                break;
            case VideoAspectRatioNone:
                break;
        }
    }
    AddXMLElementUnsignedInt(&oXML, "Video_Frequency", pnVideoFrequency);
    AddXMLElementFloat(&oXML, "Video_Exposure", pfVideoExposure);
    AddXMLElementFloat(&oXML, "Video_Flash_Time", pfVideoFlashTime);

    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    if (SendXML(oXML.GetDoc().c_str()))
    {
        return true;
    }

    return false;
} // SetGeneralCameraVideo


// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraSyncOutSettings(
    const unsigned int  nCameraID,         const unsigned int portNumber, const ESyncOutFreqMode* peSyncOutMode,
    const unsigned int* pnSyncOutValue, const float*       pfSyncOutDutyCycle,
    const bool*         pbSyncOutNegativePolarity)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &nCameraID);

    int port = portNumber - 1;
    if (((port == 0 || port == 1) && peSyncOutMode) || (port == 2))
    {
        oXML.AddElem(port == 0 ? "Sync_Out" : (port == 1 ? "Sync_Out2" : "Sync_Out_MT"));
        oXML.IntoElem();

        if (port == 0 || port == 1)
        {
            switch (*peSyncOutMode)
            {
            case ModeShutterOut:
                oXML.AddElem("Mode", "Shutter out");
                break;
            case ModeMultiplier:
                oXML.AddElem("Mode", "Multiplier");
                break;
            case ModeDivisor:
                oXML.AddElem("Mode", "Divisor");
                break;
            case ModeIndependentFreq:
                oXML.AddElem("Mode", "Camera independent");
                break;
            case ModeMeasurementTime:
                oXML.AddElem("Mode", "Measurement time");
                break;
            case ModeFixed100Hz:
                oXML.AddElem("Mode", "Continuous 100Hz");
                break;
            default:
                return false; // Should never happen
            }

            if (*peSyncOutMode == ModeMultiplier ||
                *peSyncOutMode == ModeDivisor ||
                *peSyncOutMode == ModeIndependentFreq)
            {
                if (pnSyncOutValue)
                {
                    AddXMLElementUnsignedInt(&oXML, "Value", pnSyncOutValue);
                }
                if (pfSyncOutDutyCycle)
                {
                    AddXMLElementFloat(&oXML, "Duty_Cycle", pfSyncOutDutyCycle, 3);
                }
            }
        }
        if (pbSyncOutNegativePolarity && (port == 2 ||
            (peSyncOutMode && *peSyncOutMode != ModeFixed100Hz)))
        {
            AddXMLElementBool(&oXML, "Signal_Polarity", pbSyncOutNegativePolarity, "Negative", "Positive");
        }
        oXML.OutOfElem(); // Sync_Out
    }
    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    if (SendXML(oXML.GetDoc().c_str()))
    {
        return true;
    }

    return false;
} // SetGeneralCameraSyncOut


  // nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraLensControlSettings(const unsigned int nCameraID, const float focus, const float aperture)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &nCameraID);

    oXML.AddElem("LensControl");
    oXML.IntoElem();

    oXML.AddElem("Focus");
    oXML.AddAttrib("Value", CMarkup::Format("%f", focus).c_str());
    oXML.AddElem("Aperture");
    oXML.AddAttrib("Value", CMarkup::Format("%f", aperture).c_str());

    oXML.OutOfElem(); // LensControl
    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    if (SendXML(oXML.GetDoc().c_str()))
    {
        return true;
    }

    return false;
}

// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraAutoExposureSettings(const unsigned int nCameraID, const bool autoExposure, const float compensation)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &nCameraID);

    oXML.AddElem("LensControl");
    oXML.IntoElem();

    oXML.AddElem("AutoExposure");
    oXML.AddAttrib("Enabled", autoExposure ? "true" : "false");
    oXML.AddAttrib("Compensation", CMarkup::Format("%f", compensation).c_str());

    oXML.OutOfElem(); // AutoExposure
    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    if (SendXML(oXML.GetDoc().c_str()))
    {
        return true;
    }

    return false;
}

// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraAutoWhiteBalance(const unsigned int nCameraID, const bool enable)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("General");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &nCameraID);

    oXML.AddElem("AutoWhiteBalance", enable ? "true" : "false");

    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // General
    oXML.OutOfElem(); // QTM_Settings

    if (SendXML(oXML.GetDoc().c_str()))
    {
        return true;
    }

    return false;
}


bool CRTProtocol::SetImageSettings(
    const unsigned int  nCameraID, const bool*         pbEnable,    const CRTPacket::EImageFormat* peFormat,
    const unsigned int* pnWidth,   const unsigned int* pnHeight,    const float* pfLeftCrop,
    const float*        pfTopCrop, const float*        pfRightCrop, const float* pfBottomCrop)
{
    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("Image");
    oXML.IntoElem();

    oXML.AddElem("Camera");
    oXML.IntoElem();

    AddXMLElementUnsignedInt(&oXML, "ID", &nCameraID);

    AddXMLElementBool(&oXML, "Enabled", pbEnable);

    if (peFormat)
    {
        switch (*peFormat)
        {
        case CRTPacket::FormatRawGrayscale :
            oXML.AddElem("Format", "RAWGrayscale");
            break;
        case CRTPacket::FormatRawBGR :
            oXML.AddElem("Format", "RAWBGR");
            break;
        case CRTPacket::FormatJPG :
            oXML.AddElem("Format", "JPG");
            break;
        case CRTPacket::FormatPNG :
            oXML.AddElem("Format", "PNG");
            break;
        }
    }
    AddXMLElementUnsignedInt(&oXML, "Width",       pnWidth);
    AddXMLElementUnsignedInt(&oXML, "Height",      pnHeight);
    AddXMLElementFloat(&oXML,       "Left_Crop",   pfLeftCrop);
    AddXMLElementFloat(&oXML,       "Top_Crop",    pfTopCrop);
    AddXMLElementFloat(&oXML,       "Right_Crop",  pfRightCrop);
    AddXMLElementFloat(&oXML,       "Bottom_Crop", pfBottomCrop);

    oXML.OutOfElem(); // Camera
    oXML.OutOfElem(); // Image
    oXML.OutOfElem(); // QTM_Settings

    if (SendXML(oXML.GetDoc().c_str()))
    {
        return true;
    }

    return false;
} // SetImageSettings


bool CRTProtocol::SetForceSettings(
    const unsigned int nPlateID,  const SPoint* psCorner1, const SPoint* psCorner2,
    const SPoint*      psCorner3, const SPoint* psCorner4)
{
    CMarkup oXML;

    if (nPlateID > 0)
    {
        oXML.AddElem("QTM_Settings");
        oXML.IntoElem();
        oXML.AddElem("Force");
        oXML.IntoElem();

        oXML.AddElem("Plate");
        oXML.IntoElem();
        
        if (mnMajorVersion > 1 || mnMinorVersion > 7)
        {
            AddXMLElementUnsignedInt(&oXML, "Plate_ID", &nPlateID);
        }
        else
        {
            AddXMLElementUnsignedInt(&oXML, "Force_Plate_Index", &nPlateID);
        }
        if (psCorner1)
        {
            oXML.AddElem("Corner1");
            oXML.IntoElem();
            AddXMLElementFloat(&oXML, "X", &(psCorner1->fX));
            AddXMLElementFloat(&oXML, "Y", &(psCorner1->fY));
            AddXMLElementFloat(&oXML, "Z", &(psCorner1->fZ));
            oXML.OutOfElem(); // Corner1
        }
        if (psCorner2)
        {
            oXML.AddElem("Corner2");
            oXML.IntoElem();
            AddXMLElementFloat(&oXML, "X", &(psCorner2->fX));
            AddXMLElementFloat(&oXML, "Y", &(psCorner2->fY));
            AddXMLElementFloat(&oXML, "Z", &(psCorner2->fZ));
            oXML.OutOfElem(); // Corner2
        }
        if (psCorner3)
        {
            oXML.AddElem("Corner3");
            oXML.IntoElem();
            AddXMLElementFloat(&oXML, "X", &(psCorner3->fX));
            AddXMLElementFloat(&oXML, "Y", &(psCorner3->fY));
            AddXMLElementFloat(&oXML, "Z", &(psCorner3->fZ));
            oXML.OutOfElem(); // Corner3
        }
        if (psCorner4)
        {
            oXML.AddElem("Corner4");
            oXML.IntoElem();
            AddXMLElementFloat(&oXML, "X", &(psCorner4->fX));
            AddXMLElementFloat(&oXML, "Y", &(psCorner4->fY));
            AddXMLElementFloat(&oXML, "Z", &(psCorner4->fZ));
            oXML.OutOfElem(); // Corner4
        }
        oXML.OutOfElem(); // Plate

        oXML.OutOfElem(); // Force
        oXML.OutOfElem(); // QTM_Settings

        if (SendXML(oXML.GetDoc().c_str()))
        {
            return true;
        }
    }
    else
    {
        sprintf(maErrorStr, "Illegal force plate id: %d.", nPlateID);
    }
    return false;
} // SetForceSettings


bool CRTProtocol::Set6DOFBodySettings(std::vector<SSettings6DOFBody> settings)
{
    if (mnMajorVersion == 1 && mnMinorVersion < 21)
    {
        strcpy(maErrorStr, "Set6DOFBodySettings only available for protocol version 1.21 and later.");

        return false;
    }

    CMarkup oXML;

    oXML.AddElem("QTM_Settings");
    oXML.IntoElem();
    oXML.AddElem("The_6D");
    oXML.IntoElem();

    for (auto &body : settings)
    {
        oXML.AddElem("Body");
        oXML.IntoElem();
        oXML.AddElem("Name", body.name.c_str());
        oXML.AddElem("Color");
        oXML.AddAttrib("R", std::to_string(body.color & 0xff).c_str());
        oXML.AddAttrib("G", std::to_string((body.color >> 8) & 0xff).c_str());
        oXML.AddAttrib("B", std::to_string((body.color >> 16) & 0xff).c_str());
        oXML.AddElem("MaximumResidual", std::to_string(body.maxResidual).c_str());
        oXML.AddElem("MinimumMarkersInBody", std::to_string(body.minMarkersInBody).c_str());
        oXML.AddElem("BoneLengthTolerance", std::to_string(body.boneLengthTolerance).c_str());
        oXML.AddElem("Filter");
        oXML.AddAttrib("Preset", body.filterPreset.c_str());

        if (!body.mesh.name.empty())
        {
            oXML.AddElem("Mesh");
            oXML.IntoElem();
            oXML.AddElem("Name", body.mesh.name.c_str());
            oXML.AddElem("Position");
            oXML.AddAttrib("X", std::to_string(body.mesh.position.fX).c_str());
            oXML.AddAttrib("Y", std::to_string(body.mesh.position.fY).c_str());
            oXML.AddAttrib("Z", std::to_string(body.mesh.position.fZ).c_str());
            oXML.AddElem("Rotation");
            oXML.AddAttrib("X", std::to_string(body.mesh.rotation.fX).c_str());
            oXML.AddAttrib("Y", std::to_string(body.mesh.rotation.fY).c_str());
            oXML.AddAttrib("Z", std::to_string(body.mesh.rotation.fZ).c_str());
            oXML.AddElem("Scale", std::to_string(body.mesh.scale).c_str());
            oXML.AddElem("Opacity", std::to_string(body.mesh.opacity).c_str());
            oXML.OutOfElem(); // Mesh
        }

        if (!body.points.empty())
        {
            oXML.AddElem("Points");
            oXML.IntoElem();
            for (auto &point : body.points)
            {
                oXML.AddElem("Point");
                oXML.AddAttrib("X", std::to_string(point.fX).c_str());
                oXML.AddAttrib("Y", std::to_string(point.fY).c_str());
                oXML.AddAttrib("Z", std::to_string(point.fZ).c_str());
                oXML.AddAttrib("Virtual", point.virtual_ ? "1" : "0");
                oXML.AddAttrib("PhysicalId", std::to_string(point.physicalId).c_str());
                oXML.AddAttrib("Name", point.name.c_str());
            }
            oXML.OutOfElem(); // Points
        }
        oXML.AddElem("Data_origin", std::to_string(body.origin.type).c_str());
        oXML.AddAttrib("X", std::to_string(body.origin.position.fX).c_str());
        oXML.AddAttrib("Y", std::to_string(body.origin.position.fY).c_str());
        oXML.AddAttrib("Z", std::to_string(body.origin.position.fZ).c_str());
        oXML.AddAttrib("Relative_body", std::to_string(body.origin.relativeBody).c_str());
        oXML.AddElem("Data_orientation", std::to_string(body.origin.type).c_str());
        for (uint32_t i = 0; i < 9; i++)
        {
            char tmpStr[16];
            sprintf(tmpStr, "R%u%u", (i / 3) + 1, (i % 3) + 1);
            oXML.AddAttrib(tmpStr, std::to_string(body.origin.rotation[i]).c_str());
        }
        oXML.AddAttrib("Relative_body", std::to_string(body.origin.relativeBody).c_str());

        oXML.OutOfElem(); // Body
    }
    oXML.OutOfElem(); // The_6D
    oXML.OutOfElem(); // QTM_Settings

    return SendXML(oXML.GetDoc().c_str());
}

bool CRTProtocol::SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& skeletons)
{
    CMarkup xml;

    xml.AddElem("QTM_Settings");
    xml.IntoElem();
    xml.AddElem("Skeletons");
    xml.IntoElem();

    for (auto& skeleton : skeletons)
    {
        xml.AddElem("Skeleton");
        xml.SetAttrib("Name", skeleton.name.c_str());
        xml.IntoElem();
        if (mnMajorVersion == 1 && mnMinorVersion < 22)
        {
            xml.AddElem("Solver", skeleton.rootSegment.solver.c_str());
        }
        xml.AddElem("Scale", std::to_string(skeleton.scale).c_str());
        xml.AddElem("Segments");
        xml.IntoElem();

        std::function<void(const SSettingsSkeletonSegmentHierarchical)> recurseSegments = [&](const SSettingsSkeletonSegmentHierarchical& segment)
        {
            xml.AddElem("Segment");
            xml.SetAttrib("Name", segment.name.c_str());
            xml.IntoElem();
            {
                if (mnMajorVersion > 1 || mnMinorVersion > 21)
                {
                    xml.AddElem("Solver", segment.solver.c_str());
                }
                if (!std::isnan(segment.position.x))
                {
                    AddXMLElementTransform(xml, "Transform", segment.position, segment.rotation);
                }
                if (!std::isnan(segment.defaultPosition.x))
                {
                    AddXMLElementTransform(xml, "DefaultTransform", segment.defaultPosition, segment.defaultRotation);
                }
                xml.AddElem("DegreesOfFreedom");
                xml.IntoElem();
                for (auto dof : segment.degreesOfFreedom)
                {
                    AddXMLElementDOF(xml, SkeletonDofToString(dof.type), dof);
                }
                xml.OutOfElem(); // DegreesOfFreedom

                xml.AddElem("Endpoint");
                {
                    if (!std::isnan(segment.endpoint.x) && !std::isnan(segment.endpoint.y) && !std::isnan(segment.endpoint.z))
                    {
                        xml.AddAttrib("X", std::to_string(segment.endpoint.x).c_str());
                        xml.AddAttrib("Y", std::to_string(segment.endpoint.y).c_str());
                        xml.AddAttrib("Z", std::to_string(segment.endpoint.z).c_str());
                    }
                }

                xml.AddElem("Markers");
                xml.IntoElem();
                {
                    for (const auto& marker : segment.markers)
                    {
                        xml.AddElem("Marker");
                        xml.AddAttrib("Name", marker.name.c_str());
                        xml.IntoElem();
                        {
                            xml.AddElem("Position");
                            xml.AddAttrib("X", std::to_string(marker.position.x).c_str());
                            xml.AddAttrib("Y", std::to_string(marker.position.y).c_str());
                            xml.AddAttrib("Z", std::to_string(marker.position.z).c_str());
                            xml.AddElem("Weight", std::to_string(marker.weight).c_str());
                        }
                        xml.OutOfElem(); // Marker
                    }
                }
                xml.OutOfElem(); // MarkerS

                xml.AddElem("RigidBodies");
                xml.IntoElem();
                {
                    for (const auto& rigidBody : segment.bodies)
                    {
                        xml.AddElem("RigidBody");
                        xml.AddAttrib("Name", rigidBody.name.c_str());
                        xml.IntoElem();

                        xml.AddElem("Transform");
                        xml.IntoElem();

                        xml.AddElem("Position");
                        xml.AddAttrib("X", std::to_string(rigidBody.position.x).c_str());
                        xml.AddAttrib("Y", std::to_string(rigidBody.position.y).c_str());
                        xml.AddAttrib("Z", std::to_string(rigidBody.position.z).c_str());
                        xml.AddElem("Rotation");
                        xml.AddAttrib("X", std::to_string(rigidBody.rotation.x).c_str());
                        xml.AddAttrib("Y", std::to_string(rigidBody.rotation.y).c_str());
                        xml.AddAttrib("Z", std::to_string(rigidBody.rotation.z).c_str());
                        xml.AddAttrib("W", std::to_string(rigidBody.rotation.w).c_str());
                        xml.OutOfElem(); // Transform

                        xml.AddElem("Weight", std::to_string(rigidBody.weight).c_str());
                        xml.OutOfElem(); // RigidBody
                    }
                }
                xml.OutOfElem(); // RigidBodies
            }
            xml.OutOfElem(); // Segment

            for (const auto& childSegment : segment.segments)
            {
                xml.IntoElem();
                recurseSegments(childSegment);
                xml.OutOfElem();
            }
        };
        recurseSegments(skeleton.rootSegment);

        xml.OutOfElem(); // Segments
        xml.OutOfElem(); // Skeleton
    }
    xml.OutOfElem(); // Skeleton

    return SendXML(xml.GetDoc().c_str());
}


const char* CRTProtocol::SkeletonDofToString(CRTProtocol::EDegreeOfFreedom dof)
{
    auto it = std::find_if(DEGREES_OF_FREEDOM.begin(), DEGREES_OF_FREEDOM.end(), [&](const auto& DEGREE_OF_FREEDOM) { return (DEGREE_OF_FREEDOM.first == dof); });

    if (it == DEGREES_OF_FREEDOM.end())
    {
        throw std::runtime_error("Unknown degree of freedom");
    }

    return it->second;
}


CRTProtocol::EDegreeOfFreedom CRTProtocol::SkeletonStringToDof(const std::string& str)
{
    auto it = std::find_if(DEGREES_OF_FREEDOM.begin(), DEGREES_OF_FREEDOM.end(), [&](const auto& DEGREE_OF_FREEDOM) { return (DEGREE_OF_FREEDOM.second == str); });

    if (it == DEGREES_OF_FREEDOM.end())
    {
        throw std::runtime_error("Unknown degree of freedom");
    }

    return it->first;
}


char* CRTProtocol::GetErrorString()
{
    return maErrorStr;
}


bool CRTProtocol::SendString(const char* pCmdStr, int nType)
{
    uint32_t nCmdStrLen = (int)strlen(pCmdStr);
    uint32_t nSize = 8 + nCmdStrLen + 1; // Header size + length of the string + terminating null char

    if (nSize > mSendBuffer.size())
    {
        mSendBuffer.resize(nSize);
    }
    
    memcpy(mSendBuffer.data() + 8, pCmdStr, nCmdStrLen + 1);

    if ((mnMajorVersion == 1 && mnMinorVersion == 0) || mbBigEndian)
    {
        *((unsigned int*)mSendBuffer.data())       = htonl(nSize);
        *((unsigned int*)(mSendBuffer.data() + 4)) = htonl(nType);
    }
    else
    {
        *((unsigned int*)mSendBuffer.data())       = nSize;
        *((unsigned int*)(mSendBuffer.data() + 4)) = nType;
    }

    if (mpoNetwork->Send(mSendBuffer.data(), nSize) == false)
    {
        strcpy(maErrorStr, mpoNetwork->GetErrorString());
        return false;
    }

    return true;
} // SendString


bool CRTProtocol::SendCommand(const char* pCmdStr)
{
    return SendString(pCmdStr, CRTPacket::PacketCommand);
} // SendCommand


bool CRTProtocol::SendCommand(const char* pCmdStr, char* pCommandResponseStr, unsigned int timeout)
{
    CRTPacket::EPacketType eType;

    if (SendString(pCmdStr, CRTPacket::PacketCommand))
    {
        while (Receive(eType, true, timeout) == CNetwork::ResponseType::success)
        {
            if (eType == CRTPacket::PacketCommand)
            {
                strcpy(pCommandResponseStr, mpoRTPacket->GetCommandString());
                return true;
            }
            if (eType == CRTPacket::PacketError)
            {
                strcpy(pCommandResponseStr, mpoRTPacket->GetErrorString());
                strcpy(maErrorStr, pCommandResponseStr);
                return false;
            }
        }
    }
    else
    {
        char pTmpStr[256];
        strcpy(pTmpStr, maErrorStr);
        sprintf(maErrorStr, "\'%s\' command failed. %s", pCmdStr, pTmpStr);
    }
    pCommandResponseStr[0] = 0;
    return false;
} // SendCommand


bool CRTProtocol::SendXML(const char* pCmdStr)
{
    CRTPacket::EPacketType eType;

    if (SendString(pCmdStr, CRTPacket::PacketXML))
    {
        if (Receive(eType, true) == CNetwork::ResponseType::success)
        {
            if (eType == CRTPacket::PacketCommand)
            {
                if (strcmp(mpoRTPacket->GetCommandString(), "Setting parameters succeeded") == 0)
                {
                    return true;
                }
                else
                {
                    sprintf(maErrorStr,
                        "Expected command response \"Setting parameters succeeded\". Got \"%s\".",
                        mpoRTPacket->GetCommandString());
                }
            }
            else if (eType == CRTPacket::PacketError)
            {
                strcpy(maErrorStr, mpoRTPacket->GetErrorString());
            }
            else
            {
                sprintf(maErrorStr, "Expected command response packet. Got packet type %d.", (int)eType);
            }
        }
        else
        {        
            strcpy(maErrorStr, "Missing command response packet.");
        }
    }
    else
    {
        char pTmpStr[256];
        strcpy(pTmpStr, maErrorStr);
        sprintf(maErrorStr, "Failed to send XML string. %s", pTmpStr);
    }
    return false;
} // SendXML


void CRTProtocol::AddXMLElementBool(CMarkup* oXML, const char* tTag, const bool* pbValue, const char* tTrue, const char* tFalse)
{
    if (pbValue)
    {
        oXML->AddElem(tTag, *pbValue ? tTrue : tFalse);
    }
}


void CRTProtocol::AddXMLElementBool(CMarkup* oXML, const char* tTag, const bool pbValue, const char* tTrue, const char* tFalse)
{
    oXML->AddElem(tTag, pbValue ? tTrue : tFalse);
}


void CRTProtocol::AddXMLElementInt(CMarkup* oXML, const char* tTag, const int* pnValue)
{
    if (pnValue)
    {
        std::string tVal;

        tVal = CMarkup::Format("%d", *pnValue);
        oXML->AddElem(tTag, tVal.c_str());
    }
}


void CRTProtocol::AddXMLElementUnsignedInt(CMarkup* oXML, const char* tTag, const unsigned int value)
{
    std::string tVal = CMarkup::Format("%u", value);
    oXML->AddElem(tTag, tVal.c_str());
}

void CRTProtocol::AddXMLElementUnsignedInt(CMarkup* oXML, const char* tTag, const unsigned int* pnValue)
{
    if (pnValue)
    {
        AddXMLElementUnsignedInt(oXML, tTag, *pnValue);
    }
}

void CRTProtocol::AddXMLElementFloat(CMarkup* oXML, const char* tTag, const float* pfValue, unsigned int pnDecimals)
{
    if (pfValue)
    {
        std::string tVal;
        char fFormat[10];

        sprintf(fFormat, "%%.%df", pnDecimals);
        tVal = CMarkup::Format(fFormat, *pfValue);
        oXML->AddElem(tTag, tVal.c_str());
    }
}

void CRTProtocol::AddXMLElementTransform(CMarkup& xml, const std::string& name, const SPosition& position, const SRotation& rotation)
{
    xml.AddElem(name.c_str());
    xml.IntoElem();

    xml.AddElem("Position");
    xml.AddAttrib("X", std::to_string(position.x).c_str());
    xml.AddAttrib("Y", std::to_string(position.y).c_str());
    xml.AddAttrib("Z", std::to_string(position.z).c_str());

    xml.AddElem("Rotation");
    xml.AddAttrib("X", std::to_string(rotation.x).c_str());
    xml.AddAttrib("Y", std::to_string(rotation.y).c_str());
    xml.AddAttrib("Z", std::to_string(rotation.z).c_str());
    xml.AddAttrib("W", std::to_string(rotation.w).c_str());

    xml.OutOfElem();
}

void CRTProtocol::AddXMLElementDOF(CMarkup& xml, const std::string& name, SDegreeOfFreedom degreeOfFreedoms)
{
    xml.AddElem(name.c_str());
    if (!std::isnan(degreeOfFreedoms.lowerBound) && !std::isnan(degreeOfFreedoms.upperBound))
    {
        if (mnMajorVersion > 1 || mnMinorVersion > 21)
        {
            xml.IntoElem();
            xml.AddElem("Constraint");
        }
        xml.AddAttrib("LowerBound", std::to_string(degreeOfFreedoms.lowerBound).c_str());
        xml.AddAttrib("UpperBound", std::to_string(degreeOfFreedoms.upperBound).c_str());
    }

    if (std::isnan(degreeOfFreedoms.lowerBound) || std::isnan(degreeOfFreedoms.upperBound) || (mnMajorVersion == 1 && mnMinorVersion < 22))
    {
        xml.IntoElem();
    }
    
    if (!degreeOfFreedoms.couplings.empty())
    {
        xml.AddElem("Couplings");
        xml.IntoElem();
        {
            for (const auto& coupling : degreeOfFreedoms.couplings)
            {
                xml.AddElem("Coupling");
                xml.AddAttrib("Segment", coupling.segment.c_str());
                xml.AddAttrib("DegreeOfFreedom", SkeletonDofToString(coupling.degreeOfFreedom));
                xml.AddAttrib("Coefficient", std::to_string(coupling.coefficient).c_str());
            }
        }
        xml.OutOfElem(); // Couplings
    }

    if (!std::isnan(degreeOfFreedoms.goalValue) && !std::isnan(degreeOfFreedoms.goalWeight))
    {
        xml.AddElem("Goal");
        xml.AddAttrib("Value", std::to_string(degreeOfFreedoms.goalValue).c_str());
        xml.AddAttrib("Weight", std::to_string(degreeOfFreedoms.goalWeight).c_str());
    }
    xml.OutOfElem();
}

bool CRTProtocol::CompareNoCase(std::string tStr1, const char* tStr2) const
{
    tStr1 = ToLower(tStr1);
    return tStr1.compare(tStr2) == 0;
}

std::string CRTProtocol::ToLower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
    return str;
}

bool CRTProtocol::ParseString(const std::string& str, uint32_t& value)
{
    try
    {
        value = std::stoul(str);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool CRTProtocol::ParseString(const std::string& str, int32_t& value)
{
    try
    {
        value = std::stol(str);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool CRTProtocol::ParseString(const std::string& str, float& value)
{
    try
    {
        value = std::stof(str);
    }
    catch (...)
    {
        value = std::numeric_limits<float>::quiet_NaN();
        return false;
    }
    return true;
}

bool CRTProtocol::ParseString(const std::string& str, double& value)
{
    try
    {
        value = std::stod(str);
    }
    catch (...)
    {
        value = std::numeric_limits<double>::quiet_NaN();
        return false;
    }
    return true;
}

bool CRTProtocol::ParseString(const std::string& str, bool& value)
{
    std::string strLower = ToLower(str);

    if (strLower == "true" || strLower == "1")
    {
        value = true;
        return true;
    }
    else if (strLower == "false" || strLower == "0")
    {
        value = false;
        return true;
    }
    return false;
}
