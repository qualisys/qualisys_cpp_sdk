#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include "RTProtocol.h"
#include "Tinyxml2Serializer.h"

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

#include "Network.h"
#include <stdexcept>

#ifdef _WIN32
#include <iphlpapi.h>
// import the internet protocol helper library.
#pragma comment(lib, "IPHLPAPI.lib")
#else
#include <arpa/inet.h>

#endif

using namespace CRTProtocolNs;

namespace
{
    const int qtmPacketHeaderSize = 8; // 8 bytes

    std::string ToLower(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
        return str;
    }

    struct RTVersion
    {
        int major = MAJOR_VERSION;
        int minor = MINOR_VERSION;

        // Returns a list of rt protocol version which are supported by the current SDK, 
        // including an initial version by the function argument. (The initial version is not checked for compatibility)
        // The list is sorted high to low and contains only unique elements.
        static std::vector<RTVersion> VersionList(const RTVersion& initial)
        {
            // Valid versions added from high to low
            std::vector<RTVersion> versions {
                {MAJOR_VERSION, MINOR_VERSION},
                {1, 26},
                {1, 25},
                {1, 24},
                {1, 23},
            };

            // Remove higher versions
            versions.erase(
                std::remove_if(versions.begin(), versions.end(), [&](const RTVersion& v) {
                    return v.major > initial.major || (v.major == initial.major && v.minor >= initial.minor);
                }),
                versions.end()
            );

            versions.insert(versions.begin(), initial);

            return versions;
        }
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
                          int nMajorVersion, int nMinorVersion, bool bBigEndian, bool bNegotiateVersion)
{
    CRTPacket::EPacketType eType;
    std::string            tempStr;
    std::string            responseStr;

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
                    std::vector<RTVersion> versionList;
                    if (bNegotiateVersion) 
                    {
                        versionList = RTVersion::VersionList({nMajorVersion, nMinorVersion});
                    } 
                    else 
                    {
                        versionList = std::vector<RTVersion>(1, {nMajorVersion, nMinorVersion});
                    }
                    
                    for(RTVersion& version : versionList) 
                    {
                        // Set protocol version
                        if (SetVersion(version.major, version.minor))
                        {
                            // Set byte order.
                            // Unless we use protocol version 1.0, we have set the byte order by selecting the correct port.
                            if ((mnMajorVersion == 1) && (mnMinorVersion == 0))
                            {
                                if (mbBigEndian)
                                {
                                    tempStr = "ByteOrder BigEndian";
                                }
                                else
                                {
                                    tempStr = "ByteOrder LittleEndian";
                                }

                                if (SendCommand(tempStr, responseStr))
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
    std::string responseStr;
    std::string tempStr = "Version " + std::to_string(nMajorVersion) + "." + std::to_string(nMinorVersion);

    if (SendCommand(tempStr, responseStr))
    {
        tempStr = "Version set to " + std::to_string(nMajorVersion) + "." + std::to_string(nMinorVersion);

        if (responseStr == tempStr)
        {
            mnMajorVersion = nMajorVersion;
            mnMinorVersion = nMinorVersion;
            mpoRTPacket->SetVersion(mnMajorVersion, mnMinorVersion);
            return true;
        }

        if (!responseStr.empty())
        {
            sprintf(maErrorStr, "%s.", responseStr.c_str());
        }
        else
        {
            strcpy(maErrorStr, "Set Version failed.");
        }
    }
    else
    {
        tempStr = std::string(maErrorStr);
        sprintf(maErrorStr, "Send Version failed. %s.", tempStr.c_str());
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


bool CRTProtocol::GetQTMVersion(std::string& verStr)
{
    if (SendCommand("QTMVersion", verStr))
    {
        return true;
    }
    strcpy(maErrorStr, "Get QTM Version failed.");
    return false;
}


bool CRTProtocol::GetByteOrder(bool &bBigEndian)
{
    std::string responseStr;

    if (SendCommand("ByteOrder", responseStr))
    {
        bBigEndian = (responseStr == "Byte order is big endian");
        return true;
    }
    strcpy(maErrorStr, "Get Byte order failed.");
    return false;
}


bool CRTProtocol::CheckLicense(const std::string& licenseCode)
{
    std::string responseStr;
    std::string tempStr = "CheckLicense ";

    tempStr += licenseCode;

    if (SendCommand(tempStr, responseStr))
    {
        if (responseStr == "License pass")
        {
            return true;
        }
        strcpy(maErrorStr, "Wrong license code.");
    }
    else
    {
        strcpy(maErrorStr, "CheckLicense failed.");
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

bool CRTProtocol::GetCurrentFrame(const std::string& components)
{
    std::string cmdStr = "GetCurrentFrame ";
    cmdStr += components;

    if (SendCommand(cmdStr.c_str()))
    {
        return true;
    }
    strcpy(maErrorStr, "GetCurrentFrame failed.");

    return false;
}


bool CRTProtocol::GetCurrentFrame(unsigned int nComponentType, const SComponentOptions& componentOptions)
{
    std::string components;

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
    return StreamFrames(EStreamRate::RateAllFrames, 0, 0, nullptr, nComponentType);
}

bool CRTProtocol::StreamFrames(EStreamRate eRate, unsigned int nRateArg, unsigned short nUDPPort, const char* pUDPAddr, const char* components)
{
    std::ostringstream commandString;

    if (eRate == EStreamRate::RateFrequencyDivisor)
    {
        commandString << "StreamFrames FrequencyDivisor:" << nRateArg << " ";
    }
    else if (eRate == EStreamRate::RateFrequency)
    {
        commandString << "StreamFrames Frequency:" << nRateArg << " ";
    }
    else if (eRate == EStreamRate::RateAllFrames)
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

double CRTProtocol::SMPTENormalizedSubFrame(unsigned int captureFrequency, unsigned int timestampFrequency, unsigned int subFrame) 
{
    if(captureFrequency < timestampFrequency || !timestampFrequency || !captureFrequency) 
    {
        return 0.0;
    }

    const auto subFramesPerFrame = captureFrequency / timestampFrequency;

    return static_cast<double>(subFrame) / static_cast<double>(subFramesPerFrame);
}

bool CRTProtocol::StreamFrames(EStreamRate eRate, unsigned int nRateArg, unsigned short nUDPPort, const char* pUDPAddr,
                               unsigned int nComponentType, const SComponentOptions& componentOptions)
{
    std::string components;

    if (GetComponentString(components, nComponentType, componentOptions))
    {
        return StreamFrames(eRate, nRateArg, nUDPPort, pUDPAddr, components.c_str());
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
    std::string            responseStr;

    mpFileBuffer = fopen(pFileName, "wb");
    if (mpFileBuffer != nullptr)
    {
        if (bC3D)
        {
            // C3D file
            if (SendCommand((mnMajorVersion > 1 || mnMinorVersion > 7) ? "GetCaptureC3D" : "GetCapture", responseStr))
            {
                if (responseStr == "Sending capture")
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
            if (SendCommand("GetCaptureQTM", responseStr))
            {
                if (responseStr == "Sending capture")
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
    std::string responseStr;

    if (SendCommand("Trig", responseStr))
    {
        if (responseStr == "Trig ok")
        {
            return true;
        }
    }
    if (!responseStr.empty())
    {
        sprintf(maErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(maErrorStr, "Trig failed.");
    }
    return false;
}


bool CRTProtocol::SetQTMEvent(const std::string& label)
{
    std::string responseStr;
    std::string tempStr = (mnMajorVersion > 1 || mnMinorVersion > 7) ? "SetQTMEvent " : "Event ";

    tempStr += label;

    if (SendCommand(tempStr, responseStr))
    {
        if (responseStr == "Event set")
        {
            return true;
        }
    }
    if (!responseStr.empty())
    {
        sprintf(maErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        sprintf(maErrorStr, "%s failed.", (mnMajorVersion > 1 || mnMinorVersion > 7) ? "SetQTMEvent" : "Event");
    }

    return false;
}


bool CRTProtocol::TakeControl(const std::string& password)
{
    std::string responseStr;
    std::string cmdStr = "TakeControl";

    // Add password
    if (!password.empty())
    {
        cmdStr += " ";
        cmdStr += password;
    }
    if (SendCommand(cmdStr, responseStr))
    {
        if (responseStr == "You are now master" ||
            responseStr == "You are already master")
        {
            mbIsMaster = true;
            return true;
        }
    }
    if (!responseStr.empty())
    {
        sprintf(maErrorStr, "%s.", responseStr.c_str());
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
    std::string responseStr;

    if (SendCommand("ReleaseControl", responseStr))
    {
        if (responseStr == "You are now a regular client" ||
            responseStr == "You are already a regular client")
        {
            mbIsMaster = false;
            return true;
        }
    }
    if (!responseStr.empty())
    {
        sprintf(maErrorStr, "%s.", responseStr.c_str());
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
    std::string responseStr;

    if (SendCommand("New", responseStr))
    {
        if (responseStr == "Creating new connection" ||
            responseStr == "Already connected")
        {
            return true;
        }
    }
    if (!responseStr.empty())
    {
        sprintf(maErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(maErrorStr, "New failed.");
    }
    return false;
}

bool CRTProtocol::CloseMeasurement()
{
    std::string responseStr;

    if (SendCommand("Close", responseStr))
    {
        if (responseStr == "Closing connection" ||
            responseStr == "File closed" ||
            responseStr == "Closing file" ||
            responseStr == "No connection to close")
        {
            return true;
        }
    }
    if (!responseStr.empty())
    {
        sprintf(maErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(maErrorStr, "Close failed.");
    }
    return false;
}


bool CRTProtocol::StartCapture()
{
    std::string responseStr;

    if (SendCommand("Start", responseStr))
    {
        if (responseStr == "Starting measurement")
        {
            return true;
        }
    }
    if (!responseStr.empty())
    {
        sprintf(maErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(maErrorStr, "Start failed.");
    }
    return false;
}


bool CRTProtocol::StartRTOnFile()
{
    std::string responseStr;

    if (SendCommand("Start rtfromfile", responseStr))
    {
        if (responseStr == "Starting RT from file")
        {
            return true;
        }
    }
    if (!responseStr.empty())
    {
        if (responseStr == "RT from file already running")
        {
            return true;
        }
        sprintf(maErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(maErrorStr, "Starting RT from file failed.");
    }
    return false;
}


bool CRTProtocol::StopCapture()
{
    std::string responseStr;

    if (SendCommand("Stop", responseStr))
    {
        if (responseStr == "Stopping measurement")
        {
            return true;
        }
    }
    if (!responseStr.empty())
    {
        sprintf(maErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(maErrorStr, "Stop failed.");
    }
    return false;
}


bool CRTProtocol::Calibrate(const bool refine, SCalibration &calibrationResult, int timeout)
{
    std::string responseStr;

    if (SendCommand(refine ? "calibrate refine" : "calibrate", responseStr))
    {
        if (responseStr == "Starting calibration")
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
    if (!responseStr.empty())
    {
        sprintf(maErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(maErrorStr, "Calibrate failed.");
    }
    return false;
}


bool CRTProtocol::LoadCapture(const std::string& fileName)
{
    std::string sendStr = "Load \"";
    std::string responseStr;

    sendStr += fileName;
    sendStr += "\"";

    if (SendCommand(sendStr.c_str(), responseStr, 20000000)) // Set timeout to 20 s for Load command.
    {
        if (responseStr == "Measurement loaded")
        {
            return true;
        }
    }
    if (!responseStr.empty())
    {
        sprintf(maErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(maErrorStr, "Load failed.");
    }

    return false;
}


bool CRTProtocol::SaveCapture(const std::string& fileName, bool bOverwrite, std::string* pNewFileName, int nSizeOfNewFileName)
{
    std::string responseStr;
    std::string tempNewFileNameStr;
    std::string tempStr = "Save ";

    tempStr += fileName;
    tempStr += (bOverwrite ? " Overwrite" : "");

    if (SendCommand(tempStr, responseStr))
    {
        if (responseStr == "Measurement saved")
        {
            if (pNewFileName && !pNewFileName->empty())
            {
                pNewFileName->clear();
            }
            return true;
        }
        tempNewFileNameStr.resize(responseStr.size());
        if (sscanf(responseStr.c_str(), "Measurement saved as '%[^']'", &tempNewFileNameStr[0]) == 1)
        {
            if (pNewFileName)
            {
                *pNewFileName = tempNewFileNameStr;
            }
            return true;
        }
    }
    if (!responseStr.empty())
    {
        sprintf(maErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(maErrorStr, "Save failed.");
    }

    return false;
}


bool CRTProtocol::LoadProject(const std::string& fileName)
{
    std::string responseStr;
    std::string tempStr = "LoadProject ";

    tempStr += fileName;

    if (SendCommand(tempStr, responseStr, 20000000)) // Timeout 20 s
    {
        if (responseStr == "Project loaded")
        {
            return true;
        }
    }
    if (!responseStr.empty())
    {
        sprintf(maErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(maErrorStr, "Load project failed.");
    }

    return false;
}


bool CRTProtocol::Reprocess()
{
    std::string responseStr;

    if (SendCommand("Reprocess", responseStr))
    {
        if (responseStr == "Reprocessing file")
        {
            return true;
        }
    }
    if (!responseStr.empty())
    {
        sprintf(maErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(maErrorStr, "Reprocess failed.");
    }
    return false;
}

void CRTProtocol::OverrideNetwork(INetwork* network)
{
    delete mpoNetwork;
    mpoNetwork = network;
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

    eRate = EStreamRate::RateNone;

    if (rateString.compare(0, 9, "allframes", 9) == 0)
    {
        eRate = EStreamRate::RateAllFrames;
    }
    else if (rateString.compare(0, 10, "frequency:") == 0)
    {
        nRateArg = atoi(rateString.substr(10).c_str());
        if (nRateArg > 0)
        {
            eRate = EStreamRate::RateFrequency;
        }
    }
    else if (rateString.compare(0, 17, "frequencydivisor:") == 0)
    {
        nRateArg = atoi(rateString.substr(17).c_str());
        if (nRateArg > 0)
        {
            eRate = EStreamRate::RateFrequencyDivisor;
        }
    }

    return eRate != EStreamRate::RateNone;
}

std::vector<std::pair<unsigned int, std::string>> CRTProtocol::GetComponents(const std::string& componentsString)
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

bool CRTProtocol::GetComponent(std::string& componentStr, unsigned int &component, std::string& option)
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


unsigned int CRTProtocol::ConvertComponentString(const std::string& componentsString)
{
    auto components = GetComponents(componentsString);

    unsigned int componentTypes = 0;

    for (auto const& component : components)
    {
        componentTypes += component.first;
    }

    return componentTypes;
}


bool CRTProtocol::GetComponentString(std::string& componentStr, unsigned int nComponentType, const SComponentOptions& options)
{
    std::string tempStr = "";

    if (nComponentType & cComponent2d)
    {
        tempStr += "2D ";
    }
    if (nComponentType & cComponent2dLin)
    {
        tempStr += "2DLin ";
    }
    if (nComponentType & cComponent3d)
    {
        tempStr += "3D ";
    }
    if (nComponentType & cComponent3dRes)
    {
        tempStr += "3DRes ";
    }
    if (nComponentType & cComponent3dNoLabels)
    {
        tempStr += "3DNoLabels ";
    }
    if (nComponentType & cComponent3dNoLabelsRes)
    {
        tempStr += "3DNoLabelsRes ";
    }
    if (nComponentType & cComponent6d)
    {
        tempStr += "6D ";
    }
    if (nComponentType & cComponent6dRes)
    {
        tempStr += "6DRes ";
    }
    if (nComponentType & cComponent6dEuler)
    {
        tempStr += "6DEuler ";
    }
    if (nComponentType & cComponent6dEulerRes)
    {
        tempStr += "6DEulerRes ";
    }
    if (nComponentType & cComponentAnalog)
    {
        tempStr += "Analog";

        if (options.mAnalogChannels != nullptr)
        {
            tempStr += ":";
            tempStr += options.mAnalogChannels;
        }

        tempStr += " ";
    }
    if (nComponentType & cComponentAnalogSingle)
    {
        tempStr += "AnalogSingle";

        if (options.mAnalogChannels != nullptr)
        {
            tempStr += ":";
            tempStr += options.mAnalogChannels;
        }

        tempStr += " ";
    }
    if (nComponentType & cComponentForce)
    {
        tempStr += "Force ";
    }
    if (nComponentType & cComponentForceSingle)
    {
        tempStr += "ForceSingle ";
    }
    if (nComponentType & cComponentGazeVector)
    {
        tempStr += "GazeVector ";
    }
    if (nComponentType & cComponentEyeTracker)
    {
        tempStr += "EyeTracker ";
    }
    if (nComponentType & cComponentImage)
    {
        tempStr += "Image ";
    }
    if (nComponentType & cComponentTimecode)
    {
        tempStr += "Timecode ";
    }
    if (nComponentType & cComponentSkeleton)
    {
        tempStr += "Skeleton";

        if (options.mSkeletonGlobalData)
        {
            tempStr += ":global";
        }

        tempStr += " ";
    }

    componentStr += tempStr;
    return (!tempStr.empty());
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


const char * CRTProtocol::ReadSettings(const std::string& settingsType)
{
    CRTPacket::EPacketType eType;

    mvsAnalogDeviceSettings.clear();
    auto sendStr = std::string("GetParameters ") + settingsType;
    if (!SendCommand(sendStr.c_str()))
    {
        sprintf(maErrorStr, "GetParameters %s failed", settingsType.c_str());
        return nullptr;
    }

retry:
    auto received = Receive(eType, true);

    if (received == CNetwork::ResponseType::timeout)
    {
        strcat(maErrorStr, " Expected XML packet.");
        return nullptr;
    }
    if (received == CNetwork::ResponseType::error)
    {
        return nullptr;
    }

    if (eType != CRTPacket::PacketXML)
    {
        if (eType == CRTPacket::PacketError)
        {
            sprintf(maErrorStr, "%s.", mpoRTPacket->GetErrorString());
            return nullptr;
        }
        else
        {
            goto retry;
            //sprintf(maErrorStr, "GetParameters %s returned wrong packet type. Got type %d expected type 2.", settingsType.c_str(), eType);
        }
    }

    return mpoRTPacket->GetXMLString();
}



bool CRTProtocol::ReadCameraSystemSettings()
{
    return ReadGeneralSettings();
}


bool CRTProtocol::ReadGeneralSettings()
{
    std::string             tStr;

    msGeneralSettings.vsCameras.clear();

    const char* data = ReadSettings("General");

    if (!data)
    {
        return false;
    }

    auto serializer = CMarkupDeserializer(data, mnMajorVersion, mnMinorVersion);

    return serializer.DeserializeGeneralSettings(msGeneralSettings);

} // ReadGeneralSettings

bool CRTProtocol::ReadCalibrationSettings()
{
    if (!SendCommand("GetParameters Calibration"))
    {
        strcpy(maErrorStr, "GetParameters Calibration failed");
        return false;
    }

    return ReceiveCalibrationSettings();
}

bool CRTProtocol::Read3DSettings(bool& bDataAvailable)
{
    bDataAvailable = false;
    ms3DSettings.s3DLabels.clear();
    ms3DSettings.pCalibrationTime[0] = 0;

    const char* data = ReadSettings("3D");
    if (!data)
    {
        return false;
    }

    auto deserializer = CMarkupDeserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.Deserialize3DSettings(ms3DSettings, bDataAvailable);
}

bool CRTProtocol::Read6DOFSettings(bool& bDataAvailable)
{
    m6DOFSettings.clear();

    const auto* data = ReadSettings("6D");
    if(!data)
    {
        return false;
    }

    auto deserializer = CMarkupDeserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.Deserialize6DOFSettings(m6DOFSettings, msGeneralSettings, bDataAvailable);
}

bool CRTProtocol::ReadGazeVectorSettings(bool& bDataAvailable)
{
    bDataAvailable = false;

    mvsGazeVectorSettings.clear();

    const auto* data = ReadSettings("GazeVector");
    if(!data)
    {
        return false;
    }

    auto deserializer = CMarkupDeserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.DeserializeGazeVectorSettings(mvsGazeVectorSettings, bDataAvailable);
}

bool CRTProtocol::ReadEyeTrackerSettings(bool& bDataAvailable)
{
    bDataAvailable = false;

    mvsEyeTrackerSettings.clear();

    const auto* data = ReadSettings("EyeTracker");
    if(!data)
    {
        return false;
    }

    auto deserializer = CMarkupDeserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.DeserializeEyeTrackerSettings(mvsEyeTrackerSettings, bDataAvailable);
}

bool CRTProtocol::ReadAnalogSettings(bool& bDataAvailable)
{
    bDataAvailable = false;

    mvsAnalogDeviceSettings.clear();

    const auto* data = ReadSettings("Analog");
    if(!data)
    {
        return false;
    }
    auto deserializer = CMarkupDeserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.DeserializeAnalogSettings(mvsAnalogDeviceSettings, bDataAvailable);
}

bool CRTProtocol::ReadForceSettings(bool& bDataAvailable)
{
    bDataAvailable = false;

    msForceSettings.vsForcePlates.clear();

    const auto* data = ReadSettings("Force");
    if(!data)
    {
        return false;
    }

    auto deserializer = CMarkupDeserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.DeserializeForceSettings(msForceSettings, bDataAvailable);
}

bool CRTProtocol::ReadImageSettings(bool& bDataAvailable)
{
    bDataAvailable = false;

    mvsImageSettings.clear();

    const auto* data = ReadSettings("Image");
    if(!data)
    {
        return false;
    }

    auto deserializer = CMarkupDeserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.DeserializeImageSettings(mvsImageSettings, bDataAvailable);
}

bool CRTProtocol::ReadSkeletonSettings(bool& bDataAvailable, bool skeletonGlobalData)
{
    bDataAvailable = false;

    mSkeletonSettings.clear();
    mSkeletonSettingsHierarchical.clear();

    const auto* data = ReadSettings(skeletonGlobalData ? "Skeleton:global" : "Skeleton");
    if (!data)
    {
        return false;
    }

    auto deserializer = CMarkupDeserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.DeserializeSkeletonSettings(skeletonGlobalData, mSkeletonSettingsHierarchical, mSkeletonSettings, bDataAvailable);
}

bool CRTProtocol::ReceiveCalibrationSettings(int timeout)
{
    CRTPacket::EPacketType  eType;
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

    auto data = mpoRTPacket->GetXMLString();
    auto deserializer = CMarkupDeserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.DeserializeCalibrationSettings(mCalibrationSettings);
} // ReadCalibrationSettings

void CRTProtocol::Get3DSettings(EAxis& axisUpwards, std::string& calibrationTime, std::vector<SSettings3DLabel>& labels3D, std::vector<SSettingsBone>& bones)
{
    axisUpwards = ms3DSettings.eAxisUpwards;
    calibrationTime = static_cast<std::string>(ms3DSettings.pCalibrationTime);

    labels3D = ms3DSettings.s3DLabels;
    bones = ms3DSettings.sBones;
}

void CRTProtocol::GetGazeVectorSettings(std::vector<SGazeVector>& gazeVectorSettings)
{
    gazeVectorSettings = mvsGazeVectorSettings;
}

void CRTProtocol::GetEyeTrackerSettings(std::vector<SEyeTracker>& eyeTrackerSettings)
{
    eyeTrackerSettings = mvsEyeTrackerSettings;
}

void CRTProtocol::GetAnalogSettings(std::vector<SAnalogDevice>& analogSettings)
{
    analogSettings = mvsAnalogDeviceSettings;
}

void CRTProtocol::GetForceSettings(SSettingsForce& forceSettings)
{
    forceSettings = msForceSettings;
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
    auto serializer = CMarkupSerializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetGeneralSettings(pnCaptureFrequency, pfCaptureTime, pbStartOnExtTrig,startOnTrigNO, startOnTrigNC, startOnTrigSoftware, peProcessingActions, peRtProcessingActions, peReprocessingActions);
    
    if (SendXML(message.data()))
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
    auto serializer = CMarkupSerializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetExtTimeBaseSettings(
        pbEnabled, peSignalSource,
        pbSignalModePeriodic, pnFreqMultiplier,
        pnFreqDivisor, pnFreqTolerance,
        pfNominalFrequency, pbNegativeEdge,
        pnSignalShutterDelay, pfNonPeriodicTimeout
    );

    return SendXML(message.data());
} // SetGeneralExtTimeBase


bool CRTProtocol::SetExtTimestampSettings(const CRTProtocol::SSettingsGeneralExternalTimestamp& timestampSettings)
{
    auto serializer = CMarkupSerializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetExtTimestampSettings(timestampSettings);

    return SendXML(message.data());
}


// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraSettings(
    const unsigned int nCameraID,        const ECameraMode* peMode,
    const float*       pfMarkerExposure, const float*       pfMarkerThreshold,
    const int*         pnOrientation)
{
    auto serializer = CMarkupSerializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetCameraSettings(
        nCameraID, peMode,
        pfMarkerExposure, pfMarkerThreshold,
        pnOrientation);

    return SendXML(message.data());
} // SetGeneralCamera


// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraVideoSettings(
    const unsigned int nCameraID,                const EVideoResolution* eVideoResolution,
    const EVideoAspectRatio* eVideoAspectRatio, const unsigned int* pnVideoFrequency,
    const float* pfVideoExposure,                const float* pfVideoFlashTime)
{
    auto serializer = CMarkupSerializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetCameraVideoSettings(
        nCameraID, eVideoResolution,
        eVideoAspectRatio, pnVideoFrequency,
        pfVideoExposure, pfVideoFlashTime
    );

    return SendXML(message.data());

} // SetGeneralCameraVideo


// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraSyncOutSettings(
    const unsigned int  nCameraID,         const unsigned int portNumber, const ESyncOutFreqMode* peSyncOutMode,
    const unsigned int* pnSyncOutValue, const float*       pfSyncOutDutyCycle,
    const bool*         pbSyncOutNegativePolarity)
{
    auto serializer = CMarkupSerializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetCameraSyncOutSettings(
        nCameraID, portNumber, peSyncOutMode,
        pnSyncOutValue, pfSyncOutDutyCycle,
        pbSyncOutNegativePolarity
    );

    return SendXML(message.data());
} // SetGeneralCameraSyncOut


  // nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraLensControlSettings(const unsigned int nCameraID, const float focus, const float aperture)
{
    auto serializer = CMarkupSerializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetCameraLensControlSettings(nCameraID, focus, aperture);
    return SendXML(message.data());

}

// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraAutoExposureSettings(const unsigned int nCameraID, const bool autoExposure, const float compensation)
{
    auto serializer = CMarkupSerializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetCameraAutoExposureSettings(nCameraID, autoExposure, compensation);
    return SendXML(message.data());
}

// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraAutoWhiteBalance(const unsigned int nCameraID, const bool enable)
{
    auto serializer = CMarkupSerializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetCameraAutoWhiteBalance(nCameraID, enable);
    return SendXML(message.data());
}


bool CRTProtocol::SetImageSettings(
    const unsigned int  nCameraID, const bool*         pbEnable,    const CRTPacket::EImageFormat* peFormat,
    const unsigned int* pnWidth,   const unsigned int* pnHeight,    const float* pfLeftCrop,
    const float*        pfTopCrop, const float*        pfRightCrop, const float* pfBottomCrop)
{
    auto serializer = CMarkupSerializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetImageSettings(
        nCameraID, pbEnable, peFormat,
        pnWidth, pnHeight, pfLeftCrop,
        pfTopCrop, pfRightCrop, pfBottomCrop
    );

    return SendXML(message.data());
} // SetImageSettings


bool CRTProtocol::SetForceSettings(
    const unsigned int nPlateID,  const SPoint* psCorner1, const SPoint* psCorner2,
    const SPoint*      psCorner3, const SPoint* psCorner4)
{
    if (nPlateID > 0)
    {
        CTinyxml2Serializer serializer(mnMajorVersion, mnMinorVersion);
        auto message = serializer.SetForceSettings(nPlateID, psCorner1, psCorner2,
            psCorner3, psCorner4);
        return SendXML(message.data());
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

    CTinyxml2Serializer serializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.Set6DOFBodySettings(settings);

    return SendXML(message.data());
}

bool CRTProtocol::SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& skeletons)
{
    CTinyxml2Serializer serializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetSkeletonSettings(skeletons);

    return SendXML(message.data());
}

const char* CRTProtocol::SkeletonDofToString(EDegreeOfFreedom dof)
{
    return CRTProtocolNs::SkeletonDofToStringSettings(dof);
}

EDegreeOfFreedom CRTProtocol::SkeletonStringToDof(const std::string& str)
{
    return CRTProtocolNs::SkeletonStringToDofSettings(str);
}


char* CRTProtocol::GetErrorString()
{
    return maErrorStr;
}


bool CRTProtocol::SendString(const char* pCmdStr, int nType)
{
    std::uint32_t nCmdStrLen = (int)strlen(pCmdStr);
    std::uint32_t nSize = 8 + nCmdStrLen + 1; // Header size + length of the string + terminating null char

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


bool CRTProtocol::SendCommand(const std::string& cmdStr, std::string& commandResponseStr, unsigned int timeout)
{
    if (SendString(cmdStr.c_str(), CRTPacket::PacketCommand))
    {
        CRTPacket::EPacketType eType;

        while (Receive(eType, true, timeout) == CNetwork::ResponseType::success)
        {
            if (eType == CRTPacket::PacketCommand)
            {
                const auto commandResponseArr = mpoRTPacket->GetCommandString();
                commandResponseStr = (commandResponseArr != nullptr ? std::string(commandResponseArr) : "");
                return true;
            }
            if (eType == CRTPacket::PacketError)
            {
                const auto commandResponseArr = mpoRTPacket->GetErrorString();
                commandResponseStr = (commandResponseArr != nullptr ? std::string(commandResponseArr) : "");
                strcpy(maErrorStr, commandResponseStr.c_str());
                return false;
            }
        }
    }
    else
    {
        std::string tmpStr;
        tmpStr = std::string(maErrorStr);
        sprintf(maErrorStr, "\'%s\' command failed. %s", cmdStr.c_str(), tmpStr.c_str());
    }
    commandResponseStr.clear();
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


