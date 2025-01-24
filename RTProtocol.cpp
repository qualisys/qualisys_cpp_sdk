#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include "RTProtocol.h"

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


bool CRTProtocol::ReadXmlBool(tinyxml2::XMLElement* xml, const std::string& element, bool& value) const
{
    if (!xml)
    {
        return false;
    }

    // Find the child element
    tinyxml2::XMLElement* child = xml->FirstChildElement(element.c_str());
    if (!child)
    {
        return false;
    }

    // Get the text content
    const char* text = child->GetText();
    if (!text)
    {
        return false;
    }

    std::string str(text);
    RemoveInvalidChars(str);
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });

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
        return false;
    }

    return true;
}


bool CRTProtocol::ReadSettings(std::string settingsType, tinyxml2::XMLDocument& oXML)
{
    CRTPacket::EPacketType eType;

    mvsAnalogDeviceSettings.clear();
    auto sendStr = "GetParameters " + settingsType;
    if (!SendCommand(sendStr.c_str()))
    {
        snprintf(maErrorStr, sizeof(maErrorStr), "GetParameters %s failed", settingsType.c_str());
        return false;
    }

retry:
    auto received = Receive(eType, true);

    if (received == CNetwork::ResponseType::timeout)
    {
        strncat(maErrorStr, " Expected XML packet.", sizeof(maErrorStr) - strlen(maErrorStr) - 1);
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
            snprintf(maErrorStr, sizeof(maErrorStr), "%s.", mpoRTPacket->GetErrorString());
            return false;
        }
        else
        {
            goto retry;
        }
    }

    const char* xmlString = mpoRTPacket->GetXMLString();
    if (!xmlString)
    {
        snprintf(maErrorStr, sizeof(maErrorStr), "Failed to retrieve XML string.");
        return false;
    }

    tinyxml2::XMLError error = oXML.Parse(xmlString);
    if (error != tinyxml2::XML_SUCCESS)
    {
        snprintf(maErrorStr, sizeof(maErrorStr), "Failed to parse XML: %s", oXML.ErrorStr());
        return false;
    }

    return true;
}


bool CRTProtocol::ReadCameraSystemSettings()
{
    return ReadGeneralSettings();
}


bool CRTProtocol::ReadGeneralSettings()
{
    tinyxml2::XMLDocument   oXML;
    std::string             tStr;

    msGeneralSettings.vsCameras.clear();

    if (!ReadSettings("General", oXML))
    {
        return false;
    }

    // Root element
    auto* root = oXML.RootElement();
    if (!root)
    {
        return false;
    }

    // ==================== General ====================
    auto* generalElem = root->FirstChildElement("General");
    if (!generalElem)
    {
        return false;
    }

    auto* frequencyElem = generalElem->FirstChildElement("Frequency");
    if (!frequencyElem || !frequencyElem->GetText())
    {
        return false;
    }
    msGeneralSettings.nCaptureFrequency = std::atoi(frequencyElem->GetText());

    auto* captureTimeElem = generalElem->FirstChildElement("Capture_Time");
    if (!captureTimeElem || !captureTimeElem->GetText())
    {
        return false;
    }
    msGeneralSettings.fCaptureTime = static_cast<float>(std::atof(captureTimeElem->GetText()));

    // Refactored variant of all this copy/paste code. TODO: Refactor everything else.
    if (!ReadXmlBool(generalElem, "Start_On_External_Trigger", msGeneralSettings.bStartOnExternalTrigger))
    {
        return false;
    }
    if (mnMajorVersion > 1 || mnMinorVersion > 14)
    {
        if (!ReadXmlBool(generalElem, "Start_On_Trigger_NO", msGeneralSettings.bStartOnTrigNO) ||
            !ReadXmlBool(generalElem, "Start_On_Trigger_NC", msGeneralSettings.bStartOnTrigNC) ||
            !ReadXmlBool(generalElem, "Start_On_Trigger_Software", msGeneralSettings.bStartOnTrigSoftware))
        {
            return false;
        }
    }

    // ==================== External time base ====================
    auto* extTimeBaseElem = generalElem->FirstChildElement("External_Time_Base");
    if (!extTimeBaseElem)
    {
        return false;
    }

    auto* enabledElem = extTimeBaseElem->FirstChildElement("Enabled");
    if (!enabledElem || !enabledElem->GetText())
    {
        return false;
    }
    tStr = ToLower(enabledElem->GetText());
    msGeneralSettings.sExternalTimebase.bEnabled = (tStr == "true");

    auto* signalSourceElem = extTimeBaseElem->FirstChildElement("Signal_Source");
    if (!signalSourceElem || !signalSourceElem->GetText())
    {
        return false;
    }
    tStr = ToLower(signalSourceElem->GetText());
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

    auto* signalModeElem = extTimeBaseElem->FirstChildElement("Signal_Mode");
    if (!signalModeElem || !signalModeElem->GetText())
    {
        return false;
    }
    tStr = ToLower(signalModeElem->GetText());
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

    auto multiplierElem = extTimeBaseElem->FirstChildElement("Frequency_Multiplier");
    if (!multiplierElem || !multiplierElem->GetText() || sscanf(multiplierElem->GetText(), "%u", &msGeneralSettings.sExternalTimebase.nFreqMultiplier) != 1) 
    {
        return false;
    }

    auto divisorElem = extTimeBaseElem->FirstChildElement("Frequency_Divisor");
    if (!divisorElem || !divisorElem->GetText() || sscanf(divisorElem->GetText(), "%u", &msGeneralSettings.sExternalTimebase.nFreqDivisor) != 1)
    {
        return false;
    }

    auto toleranceElem = extTimeBaseElem->FirstChildElement("Frequency_Tolerance");
    if (!toleranceElem || !toleranceElem->GetText() || sscanf(toleranceElem->GetText(), "%u", &msGeneralSettings.sExternalTimebase.nFreqTolerance) != 1)
    {
        return false;
    }

    auto nominalElem = extTimeBaseElem->FirstChildElement("Nominal_Frequency");
    if (!nominalElem || !nominalElem->GetText())
    {
        return false;
    }
    tStr = ToLower(nominalElem->GetText());
    if (tStr == "none")
    {
        msGeneralSettings.sExternalTimebase.fNominalFrequency = -1.0f; // Disabled
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

    auto signalEdgeElem = extTimeBaseElem->FirstChildElement("Signal_Edge");
    if (!signalEdgeElem || !signalEdgeElem->GetText())
    {
        return false;
    }
    tStr = ToLower(signalEdgeElem->GetText());
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

    auto signalShutterDelayElem = extTimeBaseElem->FirstChildElement("Signal_Shutter_Delay");
    if (!signalShutterDelayElem || !signalShutterDelayElem->GetText() ||
        sscanf(signalShutterDelayElem->GetText(), "%u", &msGeneralSettings.sExternalTimebase.nSignalShutterDelay) != 1)
    {
        return false;
    }

    // Parse Non_Periodic_Timeout
    auto nonPeriodicTimeoutElem = extTimeBaseElem->FirstChildElement("Non_Periodic_Timeout");
    if (!nonPeriodicTimeoutElem || !nonPeriodicTimeoutElem->GetText() ||
        sscanf(nonPeriodicTimeoutElem->GetText(), "%f", &msGeneralSettings.sExternalTimebase.fNonPeriodicTimeout) != 1)
    {
        return false;
    }

    auto extTimestampElem = root->FirstChildElement("External_Timestamp"); // External_Time_Base
    if (extTimestampElem)
    {
        // Parse children of External_Timestamp
        auto enabledElem = extTimestampElem->FirstChildElement("Enabled");
        if (enabledElem && enabledElem->GetText())
        {
            msGeneralSettings.sTimestamp.bEnabled = (ToLower(enabledElem->GetText()) == "true");
        }

        auto typeElem = extTimestampElem->FirstChildElement("Type");
        if (typeElem && typeElem->GetText())
        {
            std::string typeStr = ToLower(typeElem->GetText());
            if (typeStr == "smpte")
            {
                msGeneralSettings.sTimestamp.nType = Timestamp_SMPTE;
            }
            else if (typeStr == "irig")
            {
                msGeneralSettings.sTimestamp.nType = Timestamp_IRIG;
            }
            else
            {
                msGeneralSettings.sTimestamp.nType = Timestamp_CameraTime;
            }
        }

        auto frequencyElem = extTimestampElem->FirstChildElement("Frequency");
        if (frequencyElem && frequencyElem->GetText())
        {
            unsigned int timestampFrequency;
            if (sscanf(frequencyElem->GetText(), "%u", &timestampFrequency) == 1)
            {
                msGeneralSettings.sTimestamp.nFrequency = timestampFrequency;
            }
        }
    }

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
        auto* processingElem = root->FirstChildElement(processings[i]);
        if (!processingElem)
        {
            return false;
        }

        *processingActions[i] = ProcessingNone;

        if (mnMajorVersion > 1 || mnMinorVersion > 13)
        {
            auto* preProcessingElem = processingElem->FirstChildElement("PreProcessing2D");
            if (!preProcessingElem || !preProcessingElem->GetText())
            {
                return false;
            }
            if (CompareNoCase(preProcessingElem->GetText(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingPreProcess2D);
            }
        }

        auto* trackingElem = processingElem->FirstChildElement("Tracking");
        if (!trackingElem || !trackingElem->GetText())
        {
            return false;
        }
        tStr = ToLower(trackingElem->GetText());
        if (tStr == "3d")
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingTracking3D);
        }
        else if (tStr == "2d" && i != 1)
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingTracking2D);
        }

        if (i != 1)
        {
            auto* twinSystemMergeElem = processingElem->FirstChildElement("TwinSystemMerge");
            if (!twinSystemMergeElem || !twinSystemMergeElem->GetText())
            {
                return false;
            }
            if (CompareNoCase(twinSystemMergeElem->GetText(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingTwinSystemMerge);
            }

            auto* splineFillElem = processingElem->FirstChildElement("SplineFill");
            if (!splineFillElem || !splineFillElem->GetText())
            {
                return false;
            }
            if (CompareNoCase(splineFillElem->GetText(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingSplineFill);
            }
        }

        auto* aimElem = processingElem->FirstChildElement("AIM");
        if (!aimElem || !aimElem->GetText())
        {
            return false;
        }
        if (CompareNoCase(aimElem->GetText(), "true"))
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingAIM);
        }

        auto* track6DOFElem = processingElem->FirstChildElement("Track6DOF");
        if (!track6DOFElem || !track6DOFElem->GetText())
        {
            return false;
        }
        if (CompareNoCase(track6DOFElem->GetText(), "true"))
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + Processing6DOFTracking);
        }

        auto* forceDataElem = processingElem->FirstChildElement("ForceData");
        if (!forceDataElem || !forceDataElem->GetText())
        {
            return false;
        }
        if (CompareNoCase(forceDataElem->GetText(), "true"))
        {
            *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingForceData);
        }

        if (mnMajorVersion > 1 || mnMinorVersion > 11)
        {
            auto* gazeVectorElem = processingElem->FirstChildElement("GazeVector");
            if (!gazeVectorElem || !gazeVectorElem->GetText())
            {
                return false;
            }
            if (CompareNoCase(gazeVectorElem->GetText(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingGazeVector);
            }
        }

        if (i != 1)
        {
            auto* exportTSVElem = processingElem->FirstChildElement("ExportTSV");
            if (!exportTSVElem || !exportTSVElem->GetText())
            {
                return false;
            }
            if (CompareNoCase(exportTSVElem->GetText(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportTSV);
            }

            auto* exportC3DElem = processingElem->FirstChildElement("ExportC3D");
            if (!exportC3DElem || !exportC3DElem->GetText())
            {
                return false;
            }
            if (CompareNoCase(exportC3DElem->GetText(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportC3D);
            }

            auto* exportMatlabFileElem = processingElem->FirstChildElement("ExportMatlabFile");
            if (!exportMatlabFileElem || !exportMatlabFileElem->GetText())
            {
                return false;
            }
            if (CompareNoCase(exportMatlabFileElem->GetText(), "true"))
            {
                *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportMatlabFile);
            }

            if (mnMajorVersion > 1 || mnMinorVersion > 11)
            {
                auto* exportAviFileElem = processingElem->FirstChildElement("ExportAviFile");
                if (!exportAviFileElem || !exportAviFileElem->GetText())
                {
                    return false;
                }
                if (CompareNoCase(exportAviFileElem->GetText(), "true"))
                {
                    *processingActions[i] = (EProcessingActions)(*processingActions[i] + ProcessingExportAviFile);
                }
            }
        }
    }

    auto* eulerAnglesElem = root->FirstChildElement("EulerAngles");
    if (eulerAnglesElem)
    {
        msGeneralSettings.eulerRotations[0] = eulerAnglesElem->Attribute("First");
        msGeneralSettings.eulerRotations[1] = eulerAnglesElem->Attribute("Second");
        msGeneralSettings.eulerRotations[2] = eulerAnglesElem->Attribute("Third");
    }

    // Parse Cameras
    auto* cameraElem = root->FirstChildElement("Camera");
    SSettingsGeneralCamera sCameraSettings;
    while (cameraElem)
    {
        // Parse ID
        auto* idElem = cameraElem->FirstChildElement("ID");
        if (!idElem || !idElem->GetText()) {
            return false;
        }
        sCameraSettings.nID = std::atoi(idElem->GetText());

        // Parse Model
        auto* modelElem = cameraElem->FirstChildElement("Model");
        if (!modelElem || !modelElem->GetText())
        {
            return false;
        }
        tStr = ToLower(modelElem->GetText());

        if (tStr == "macreflex") {
            sCameraSettings.eModel = ModelMacReflex;
        } else if (tStr == "proreflex 120") {
            sCameraSettings.eModel = ModelProReflex120;
        } else if (tStr == "proreflex 240") {
            sCameraSettings.eModel = ModelProReflex240;
        } else if (tStr == "proreflex 500") {
            sCameraSettings.eModel = ModelProReflex500;
        } else if (tStr == "proreflex 1000") {
            sCameraSettings.eModel = ModelProReflex1000;
        } else if (tStr == "oqus 100") {
            sCameraSettings.eModel = ModelOqus100;
        } else if (tStr == "oqus 200" || tStr == "oqus 200 c") {
            sCameraSettings.eModel = ModelOqus200C;
        } else if (tStr == "oqus 300") {
            sCameraSettings.eModel = ModelOqus300;
        } else if (tStr == "oqus 300 plus") {
            sCameraSettings.eModel = ModelOqus300Plus;
        } else if (tStr == "oqus 400") {
            sCameraSettings.eModel = ModelOqus400;
        } else if (tStr == "oqus 500") {
            sCameraSettings.eModel = ModelOqus500;
        } else if (tStr == "oqus 500 plus") {
            sCameraSettings.eModel = ModelOqus500Plus;
        } else if (tStr == "oqus 700") {
            sCameraSettings.eModel = ModelOqus700;
        } else if (tStr == "oqus 700 plus") {
            sCameraSettings.eModel = ModelOqus700Plus;
        } else if (tStr == "oqus 600 plus") {
            sCameraSettings.eModel = ModelOqus600Plus;
        } else if (tStr == "miqus m1") {
            sCameraSettings.eModel = ModelMiqusM1;
        } else if (tStr == "miqus m3") {
            sCameraSettings.eModel = ModelMiqusM3;
        } else if (tStr == "miqus m5") {
            sCameraSettings.eModel = ModelMiqusM5;
        } else if (tStr == "miqus sync unit") {
            sCameraSettings.eModel = ModelMiqusSyncUnit;
        } else if (tStr == "miqus video") {
            sCameraSettings.eModel = ModelMiqusVideo;
        } else if (tStr == "miqus video color") {
            sCameraSettings.eModel = ModelMiqusVideoColor;
        } else if (tStr == "miqus hybrid") {
            sCameraSettings.eModel = ModelMiqusHybrid;
        } else if (tStr == "miqus video color plus") {
            sCameraSettings.eModel = ModelMiqusVideoColorPlus;
        } else if (tStr == "arqus a5") {
            sCameraSettings.eModel = ModelArqusA5;
        } else if (tStr == "arqus a9") {
            sCameraSettings.eModel = ModelArqusA9;
        } else if (tStr == "arqus a12") {
            sCameraSettings.eModel = ModelArqusA12;
        } else if (tStr == "arqus a26") {
            sCameraSettings.eModel = ModelArqusA26;
        } else {
            sCameraSettings.eModel = ModelUnknown;
        }

        // Only available from protocol version 1.10 and later.
        auto* underwaterElem = cameraElem->FirstChildElement("Underwater");
        if (underwaterElem && underwaterElem->GetText())
        {
            tStr = ToLower(underwaterElem->GetText());
            sCameraSettings.bUnderwater = (tStr == "true");
        }

        auto* supportsHwSyncElem = cameraElem->FirstChildElement("Supports_HW_Sync");
        if (supportsHwSyncElem && supportsHwSyncElem->GetText())
        {
            tStr = ToLower(supportsHwSyncElem->GetText());
            sCameraSettings.bSupportsHwSync = (tStr == "true");
        }

        auto* serialElem = cameraElem->FirstChildElement("Serial");
        if (!serialElem || !serialElem->GetText())
        {
            return false;
        }
        sCameraSettings.nSerial = std::atoi(serialElem->GetText());

        // ==================== Camera Mode ====================
        auto* modeElem = cameraElem->FirstChildElement("Mode");
        if (!modeElem || !modeElem->GetText())
        {
            return false;
        }
        tStr = ToLower(modeElem->GetText());
        if (tStr == "marker") {
            sCameraSettings.eMode = ModeMarker;
        } else if (tStr == "marker intensity") {
            sCameraSettings.eMode = ModeMarkerIntensity;
        } else if (tStr == "video") {
            sCameraSettings.eMode = ModeVideo;
        } else {
            return false;
        }

        if (mnMajorVersion > 1 || mnMinorVersion > 11)
        {
            auto* videoFrequencyElem = cameraElem->FirstChildElement("Video_Frequency");
            if (!videoFrequencyElem || !videoFrequencyElem->GetText())
            {
                return false;
            }
            sCameraSettings.nVideoFrequency = std::atoi(videoFrequencyElem->GetText());
        }

        // ==================== Video Resolution ====================
        auto* videoResolutionElem = cameraElem->FirstChildElement("Video_Resolution");
        if (videoResolutionElem && videoResolutionElem->GetText())
        {
            tStr = ToLower(videoResolutionElem->GetText());
            if (tStr == "1440p") {
                sCameraSettings.eVideoResolution = VideoResolution1440p;
            } else if (tStr == "1080p") {
                sCameraSettings.eVideoResolution = VideoResolution1080p;
            } else if (tStr == "720p") {
                sCameraSettings.eVideoResolution = VideoResolution720p;
            } else if (tStr == "540p") {
                sCameraSettings.eVideoResolution = VideoResolution540p;
            } else if (tStr == "480p") {
                sCameraSettings.eVideoResolution = VideoResolution480p;
            } else {
                return false;
            }
        }
        else
        {
            sCameraSettings.eVideoResolution = VideoResolutionNone;
        }

        // ==================== Video AspectRatio ====================
        auto* videoAspectRatioElem = cameraElem->FirstChildElement("Video_Aspect_Ratio");
        if (videoAspectRatioElem && videoAspectRatioElem->GetText())
        {
            tStr = ToLower(videoAspectRatioElem->GetText());
            if (tStr == "16x9") {
                sCameraSettings.eVideoAspectRatio = VideoAspectRatio16x9;
            } else if (tStr == "4x3") {
                sCameraSettings.eVideoAspectRatio = VideoAspectRatio4x3;
            } else if (tStr == "1x1") {
                sCameraSettings.eVideoAspectRatio = VideoAspectRatio1x1;
            }
        }
        else
        {
            sCameraSettings.eVideoAspectRatio = VideoAspectRatioNone;
        }

        // ==================== Video exposure ====================
        auto* videoExposureElem = cameraElem->FirstChildElement("Video_Exposure");
        if (!videoExposureElem)
        {
            return false;
        }

        auto* currentExposureElem = videoExposureElem->FirstChildElement("Current");
        if (!currentExposureElem || !currentExposureElem->GetText())
        {
            return false;
        }
        sCameraSettings.nVideoExposure = std::atoi(currentExposureElem->GetText());

        auto* minExposureElem = videoExposureElem->FirstChildElement("Min");
        if (!minExposureElem || !minExposureElem->GetText())
        {
            return false;
        }
        sCameraSettings.nVideoExposureMin = std::atoi(minExposureElem->GetText());

        auto* maxExposureElem = videoExposureElem->FirstChildElement("Max");
        if (!maxExposureElem || !maxExposureElem->GetText())
        {
            return false;
        }
        sCameraSettings.nVideoExposureMax = std::atoi(maxExposureElem->GetText());

        // ==================== Video flash time ====================
        auto* videoFlashTimeElem = cameraElem->FirstChildElement("Video_Flash_Time");
        if (!videoFlashTimeElem)
        {
            return false;
        }

        auto* currentFlashTimeElem = videoFlashTimeElem->FirstChildElement("Current");
        if (!currentFlashTimeElem || !currentFlashTimeElem->GetText())
        {
            return false;
        }
        sCameraSettings.nVideoFlashTime = std::atoi(currentFlashTimeElem->GetText());

        auto* minFlashTimeElem = videoFlashTimeElem->FirstChildElement("Min");
        if (!minFlashTimeElem || !minFlashTimeElem->GetText())
        {
            return false;
        }
        sCameraSettings.nVideoFlashTimeMin = std::atoi(minFlashTimeElem->GetText());

        auto* maxFlashTimeElem = videoFlashTimeElem->FirstChildElement("Max");
        if (!maxFlashTimeElem || !maxFlashTimeElem->GetText())
        {
            return false;
        }
        sCameraSettings.nVideoFlashTimeMax = std::atoi(maxFlashTimeElem->GetText());

        // ==================== Marker exposure ====================
        auto* markerExposureElem = cameraElem->FirstChildElement("Marker_Exposure");
        if (!markerExposureElem)
        {
            return false;
        }

        auto* currentMarkerExposureElem = markerExposureElem->FirstChildElement("Current");
        if (!currentMarkerExposureElem || !currentMarkerExposureElem->GetText())
        {
            return false;
        }
        sCameraSettings.nMarkerExposure = std::atoi(currentMarkerExposureElem->GetText());

        auto* minMarkerExposureElem = markerExposureElem->FirstChildElement("Min");
        if (!minMarkerExposureElem || !minMarkerExposureElem->GetText())
        {
            return false;
        }
        sCameraSettings.nMarkerExposureMin = std::atoi(minMarkerExposureElem->GetText());

        auto* maxMarkerExposureElem = markerExposureElem->FirstChildElement("Max");
        if (!maxMarkerExposureElem || !maxMarkerExposureElem->GetText())
        {
            return false;
        }
        sCameraSettings.nMarkerExposureMax = std::atoi(maxMarkerExposureElem->GetText());

        // ==================== Marker threshold ====================
        auto* markerThresholdElem = cameraElem->FirstChildElement("Marker_Threshold");
        if (!markerThresholdElem)
        {
            return false;
        }

        auto* currentMarkerThresholdElem = markerThresholdElem->FirstChildElement("Current");
        if (!currentMarkerThresholdElem || !currentMarkerThresholdElem->GetText())
        {
            return false;
        }
        sCameraSettings.nMarkerThreshold = std::atoi(currentMarkerThresholdElem->GetText());

        auto* minMarkerThresholdElem = markerThresholdElem->FirstChildElement("Min");
        if (!minMarkerThresholdElem || !minMarkerThresholdElem->GetText())
        {
            return false;
        }
        sCameraSettings.nMarkerThresholdMin = std::atoi(minMarkerThresholdElem->GetText());

        auto* maxMarkerThresholdElem = markerThresholdElem->FirstChildElement("Max");
        if (!maxMarkerThresholdElem || !maxMarkerThresholdElem->GetText())
        {
            return false;
        }
        sCameraSettings.nMarkerThresholdMax = std::atoi(maxMarkerThresholdElem->GetText());

        // ==================== Position ====================
        auto* positionElem = cameraElem->FirstChildElement("Position");
        if (!positionElem)
        {
            return false;
        }

        auto* xElem = positionElem->FirstChildElement("X");
        if (!xElem || !xElem->GetText())
        {
            return false;
        }
        sCameraSettings.fPositionX = static_cast<float>(std::atof(xElem->GetText()));

        auto* yElem = positionElem->FirstChildElement("Y");
        if (!yElem || !yElem->GetText())
        {
            return false;
        }
        sCameraSettings.fPositionY = static_cast<float>(std::atof(yElem->GetText()));

        auto* zElem = positionElem->FirstChildElement("Z");
        if (!zElem || !zElem->GetText())
        {
            return false;
        }
        sCameraSettings.fPositionZ = static_cast<float>(std::atof(zElem->GetText()));

        auto* rotElem = positionElem;
        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                char rotName[10];
                sprintf(rotName, "Rot_%d_%d", row + 1, col + 1);
                auto* rotValueElem = rotElem->FirstChildElement(rotName);
                if (!rotValueElem || !rotValueElem->GetText())
                {
                    return false;
                }
                sCameraSettings.fPositionRotMatrix[row][col] = static_cast<float>(std::atof(rotValueElem->GetText()));
            }
        }
        // ==================== Orientation ====================
        auto* orientationElem = cameraElem->FirstChildElement("Orientation");
        if (!orientationElem || !orientationElem->GetText())
        {
            return false;
        }
        sCameraSettings.nOrientation = std::atoi(orientationElem->GetText());

        // ==================== Marker resolution ====================
        auto* markerResElem = cameraElem->FirstChildElement("Marker_Res");
        if (!markerResElem)
        {
            return false;
        }

        auto* markerWidthElem = markerResElem->FirstChildElement("Width");
        if (!markerWidthElem || !markerWidthElem->GetText())
        {
            return false;
        }
        sCameraSettings.nMarkerResolutionWidth = std::atoi(markerWidthElem->GetText());

        auto* markerHeightElem = markerResElem->FirstChildElement("Height");
        if (!markerHeightElem || !markerHeightElem->GetText())
        {
            return false;
        }
        sCameraSettings.nMarkerResolutionHeight = std::atoi(markerHeightElem->GetText());

        // ==================== Video resolution ====================
        auto* videoResElem = cameraElem->FirstChildElement("Video_Res");
        if (!videoResElem)
        {
            return false;
        }

        auto* videoWidthElem = videoResElem->FirstChildElement("Width");
        if (!videoWidthElem || !videoWidthElem->GetText())
        {
            return false;
        }
        sCameraSettings.nVideoResolutionWidth = std::atoi(videoWidthElem->GetText());

        auto* videoHeightElem = videoResElem->FirstChildElement("Height");
        if (!videoHeightElem || !videoHeightElem->GetText())
        {
            return false;
        }
        sCameraSettings.nVideoResolutionHeight = std::atoi(videoHeightElem->GetText());

        // ==================== Marker FOV ====================
        auto* markerFOVElem = cameraElem->FirstChildElement("Marker_FOV");
        if (!markerFOVElem)
        {
            return false;
        }

        auto* markerFOVLeftElem = markerFOVElem->FirstChildElement("Left");
        if (!markerFOVLeftElem || !markerFOVLeftElem->GetText())
        {
            return false;
        }
        sCameraSettings.nMarkerFOVLeft = std::atoi(markerFOVLeftElem->GetText());

        auto* markerFOVTopElem = markerFOVElem->FirstChildElement("Top");
        if (!markerFOVTopElem || !markerFOVTopElem->GetText())
        {
            return false;
        }
        sCameraSettings.nMarkerFOVTop = std::atoi(markerFOVTopElem->GetText());

        auto* markerFOVRightElem = markerFOVElem->FirstChildElement("Right");
        if (!markerFOVRightElem || !markerFOVRightElem->GetText())
        {
            return false;
        }
        sCameraSettings.nMarkerFOVRight = std::atoi(markerFOVRightElem->GetText());

        auto* markerFOVBottomElem = markerFOVElem->FirstChildElement("Bottom");
        if (!markerFOVBottomElem || !markerFOVBottomElem->GetText())
        {
            return false;
        }
        sCameraSettings.nMarkerFOVBottom = std::atoi(markerFOVBottomElem->GetText());

        // ==================== Video FOV ====================
        auto* videoFOVElem = cameraElem->FirstChildElement("Video_FOV");
        if (!videoFOVElem)
        {
            return false;
        }

        auto* videoFOVLeftElem = videoFOVElem->FirstChildElement("Left");
        if (!videoFOVLeftElem || !videoFOVLeftElem->GetText())
        {
            return false;
        }
        sCameraSettings.nVideoFOVLeft = std::atoi(videoFOVLeftElem->GetText());

        auto* videoFOVTopElem = videoFOVElem->FirstChildElement("Top");
        if (!videoFOVTopElem || !videoFOVTopElem->GetText())
        {
            return false;
        }
        sCameraSettings.nVideoFOVTop = std::atoi(videoFOVTopElem->GetText());

        auto* videoFOVRightElem = videoFOVElem->FirstChildElement("Right");
        if (!videoFOVRightElem || !videoFOVRightElem->GetText())
        {
            return false;
        }
        sCameraSettings.nVideoFOVRight = std::atoi(videoFOVRightElem->GetText());

        auto* videoFOVBottomElem = videoFOVElem->FirstChildElement("Bottom");
        if (!videoFOVBottomElem || !videoFOVBottomElem->GetText())
        {
            return false;
        }
        sCameraSettings.nVideoFOVBottom = std::atoi(videoFOVBottomElem->GetText());

        // ==================== Sync out ====================
        // Only available from protocol version 1.10 and later.
        for (int port = 0; port < 3; ++port)
        {
            char syncOutStr[16];
            sprintf(syncOutStr, "Sync_Out%s", port == 0 ? "" : (port == 1 ? "2" : "_MT"));
            auto* syncOutElem = cameraElem->FirstChildElement(syncOutStr);

            if (syncOutElem)
            {
                if (port < 2)
                {
                    auto* modeElem = syncOutElem->FirstChildElement("Mode");
                    if (!modeElem || !modeElem->GetText())
                    {
                        return false;
                    }
                    tStr = ToLower(modeElem->GetText());
                    if (tStr == "shutter out") {
                        sCameraSettings.eSyncOutMode[port] = ModeShutterOut;
                    } else if (tStr == "multiplier") {
                        sCameraSettings.eSyncOutMode[port] = ModeMultiplier;
                    } else if (tStr == "divisor") {
                        sCameraSettings.eSyncOutMode[port] = ModeDivisor;
                    } else if (tStr == "camera independent") {
                        sCameraSettings.eSyncOutMode[port] = ModeIndependentFreq;
                    } else if (tStr == "measurement time") {
                        sCameraSettings.eSyncOutMode[port] = ModeMeasurementTime;
                    } else if (tStr == "continuous 100hz") {
                        sCameraSettings.eSyncOutMode[port] = ModeFixed100Hz;
                    } else if (tStr == "system live time") {
                        sCameraSettings.eSyncOutMode[port] = ModeSystemLiveTime;
                    } else {
                        return false;
                    }

                    if (sCameraSettings.eSyncOutMode[port] == ModeMultiplier ||
                        sCameraSettings.eSyncOutMode[port] == ModeDivisor ||
                        sCameraSettings.eSyncOutMode[port] == ModeIndependentFreq)
                    {
                        auto* valueElem = syncOutElem->FirstChildElement("Value");
                        if (!valueElem || !valueElem->GetText())
                        {
                            return false;
                        }
                        sCameraSettings.nSyncOutValue[port] = std::atoi(valueElem->GetText());

                        auto* dutyCycleElem = syncOutElem->FirstChildElement("Duty_Cycle");
                        if (!dutyCycleElem || !dutyCycleElem->GetText())
                        {
                            return false;
                        }
                        sCameraSettings.fSyncOutDutyCycle[port] = static_cast<float>(std::atof(dutyCycleElem->GetText()));
                    }
                }
                if (port == 2 || sCameraSettings.eSyncOutMode[port] != ModeFixed100Hz)
                {
                    auto* polarityElem = syncOutElem->FirstChildElement("Signal_Polarity");
                    if (!polarityElem || !polarityElem->GetText())
                    {
                        return false;
                    }
                    if (CompareNoCase(polarityElem->GetText(), "negative"))
                    {
                        sCameraSettings.bSyncOutNegativePolarity[port] = true;
                    }
                    else
                    {
                        sCameraSettings.bSyncOutNegativePolarity[port] = false;
                    }
                }
            }
            else
            {
                sCameraSettings.eSyncOutMode[port] = ModeIndependentFreq;
                sCameraSettings.nSyncOutValue[port] = 0;
                sCameraSettings.fSyncOutDutyCycle[port] = 0.0f;
                sCameraSettings.bSyncOutNegativePolarity[port] = false;
            }
        }

        auto* lensControlElem = cameraElem->FirstChildElement("LensControl");
        if (lensControlElem)
        {
            auto* focusElem = lensControlElem->FirstChildElement("Focus");
            if (focusElem)
            {
                float focus;
                if (sscanf(focusElem->Attribute("Value"), "%f", &focus) == 1)
                {
                    sCameraSettings.fFocus = focus;
                }
            }

            auto* apertureElem = lensControlElem->FirstChildElement("Aperture");
            if (apertureElem)
            {
                float aperture;
                if (sscanf(apertureElem->Attribute("Value"), "%f", &aperture) == 1)
                {
                    sCameraSettings.fAperture = aperture;
                }
            }
        }
        else
        {
            sCameraSettings.fFocus = std::numeric_limits<float>::quiet_NaN();
            sCameraSettings.fAperture = std::numeric_limits<float>::quiet_NaN();
        }

        // ==================== Auto Exposure ====================
        auto* autoExposureElem = cameraElem->FirstChildElement("AutoExposure");
        if (autoExposureElem)
        {
            const char* enabledAttrib = autoExposureElem->Attribute("Enabled");
            sCameraSettings.autoExposureEnabled = (enabledAttrib && CompareNoCase(enabledAttrib, "true"));

            float compensation;
            const char* compensationAttrib = autoExposureElem->Attribute("Compensation");
            if (compensationAttrib && sscanf(compensationAttrib, "%f", &compensation) == 1)
            {
                sCameraSettings.autoExposureCompensation = compensation;
            }
            else
            {
                sCameraSettings.autoExposureCompensation = std::numeric_limits<float>::quiet_NaN();
            }
        }
        else
        {
            sCameraSettings.autoExposureEnabled = false;
            sCameraSettings.autoExposureCompensation = std::numeric_limits<float>::quiet_NaN();
        }

        // ==================== Auto White Balance ====================
        auto* autoWhiteBalanceElem = cameraElem->FirstChildElement("AutoWhiteBalance");
        if (autoWhiteBalanceElem && autoWhiteBalanceElem->GetText())
        {
            sCameraSettings.autoWhiteBalance = CompareNoCase(autoWhiteBalanceElem->GetText(), "true") ? 1 : 0;
        }
        else
        {
            sCameraSettings.autoWhiteBalance = -1;
        }

        // Finalize Camera Parsing
        msGeneralSettings.vsCameras.push_back(sCameraSettings);
        // Move to the next Camera element
        cameraElem = cameraElem->NextSiblingElement("Camera");
    } // while-loop

    return true;
} // ReadGeneralSettings


bool ReadXmlFov(const std::string& name, tinyxml2::XMLElement* oXML, CRTProtocol::SCalibrationFov& fov)
{
    auto* fovElem = oXML->FirstChildElement(name.c_str());
    if (!fovElem)
    {
        return false;
    }

    fov.left = std::stoul(fovElem->Attribute("left"));
    fov.top = std::stoul(fovElem->Attribute("top"));
    fov.right = std::stoul(fovElem->Attribute("right"));
    fov.bottom = std::stoul(fovElem->Attribute("bottom"));

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
    tinyxml2::XMLDocument   oXML;
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
        if (event != CRTPacket::EventNone) {
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
     
    const char* xmlString = mpoRTPacket->GetXMLString();
    if (!xmlString || oXML.Parse(xmlString) != tinyxml2::XML_SUCCESS)
    {
        snprintf(maErrorStr, sizeof(maErrorStr), "Failed to parse calibration XML.");
        return false;
    }

    auto* calibrationElem = oXML.FirstChildElement("calibration");
    if (!calibrationElem)
    {
        snprintf(maErrorStr, sizeof(maErrorStr), "Missing calibration element.");
        return false;
    }

    try
    {
        std::string resultStr = ToLower(calibrationElem->Attribute("calibrated", ""));
        settings.calibrated = (resultStr == "true");
        settings.source = calibrationElem->Attribute("source", "");
        settings.created = calibrationElem->Attribute("created", "");
        settings.qtm_version = calibrationElem->Attribute("qtm-version", "");

        std::string typeStr = calibrationElem->Attribute("type", "");
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
            settings.refit_residual = std::stod(calibrationElem->Attribute("refit-residual", "0"));
        }
        if (settings.type != ECalibrationType::fixed)
        {
            settings.wand_length = std::stod(calibrationElem->Attribute("wandLength", "0"));
            settings.max_frames = std::stoul(calibrationElem->Attribute("maximumFrames", "0"));
            settings.short_arm_end = std::stod(calibrationElem->Attribute("shortArmEnd", "0"));
            settings.long_arm_end = std::stod(calibrationElem->Attribute("longArmEnd", "0"));
            settings.long_arm_middle = std::stod(calibrationElem->Attribute("longArmMiddle", "0"));

            auto* resultsElem = calibrationElem->FirstChildElement("results");
            if (!resultsElem)
            {
                return false;
            }

            settings.result_std_dev = std::stod(resultsElem->Attribute("std-dev", "0"));
            settings.result_min_max_diff = std::stod(resultsElem->Attribute("min-max-diff", "0"));
            if (settings.type == ECalibrationType::refine)
            {
                settings.result_refit_residual = std::stod(resultsElem->Attribute("refit-residual", "0"));
                settings.result_consecutive = std::stoul(resultsElem->Attribute("consecutive", "0"));
            }
        }

        auto* camerasElem = calibrationElem->FirstChildElement("cameras");
        if (!camerasElem)
        {
            return false;
        }

        auto* cameraElem = camerasElem->FirstChildElement("camera");
        while (cameraElem)
        {
            SCalibrationCamera camera;

            // Parse camera attributes
            camera.active = std::stoi(cameraElem->Attribute("active", "0")) != 0;
            camera.calibrated = (ToLower(cameraElem->Attribute("calibrated", "")) == "true");
            camera.message = cameraElem->Attribute("message", "");
            camera.point_count = std::stoul(cameraElem->Attribute("point-count", "0"));
            camera.avg_residual = std::stod(cameraElem->Attribute("avg-residual", "0"));
            camera.serial = std::stoul(cameraElem->Attribute("serial", "0"));
            camera.model = cameraElem->Attribute("model", "");
            camera.view_rotation = std::stoul(cameraElem->Attribute("viewrotation", "0"));

            // Parse FOVs
            if (!ReadXmlFov("fov_marker", cameraElem, camera.fov_marker) ||
                !ReadXmlFov("fov_marker_max", cameraElem, camera.fov_marker_max) ||
                !ReadXmlFov("fov_video", cameraElem, camera.fov_video) ||
                !ReadXmlFov("fov_video_max", cameraElem, camera.fov_video_max))
            {
                return false;
            }

            // Parse transform
            auto* transformElem = cameraElem->FirstChildElement("transform");
            if (!transformElem)
            {
                return false;
            }
            camera.transform.x = std::stod(transformElem->Attribute("x", "0"));
            camera.transform.y = std::stod(transformElem->Attribute("y", "0"));
            camera.transform.z = std::stod(transformElem->Attribute("z", "0"));
            camera.transform.r11 = std::stod(transformElem->Attribute("r11", "0"));
            camera.transform.r12 = std::stod(transformElem->Attribute("r12", "0"));
            camera.transform.r13 = std::stod(transformElem->Attribute("r13", "0"));
            camera.transform.r21 = std::stod(transformElem->Attribute("r21", "0"));
            camera.transform.r22 = std::stod(transformElem->Attribute("r22", "0"));
            camera.transform.r23 = std::stod(transformElem->Attribute("r23", "0"));
            camera.transform.r31 = std::stod(transformElem->Attribute("r31", "0"));
            camera.transform.r32 = std::stod(transformElem->Attribute("r32", "0"));
            camera.transform.r33 = std::stod(transformElem->Attribute("r33", "0"));

            auto* intrinsicElem = cameraElem->FirstChildElement("intrinsic");
            if (!intrinsicElem)
            {
                return false;
            }

            const char* focalLength = intrinsicElem->Attribute("focallength");
            camera.intrinsic.focal_length = focalLength ? std::stod(focalLength) : 0.0;

            camera.intrinsic.sensor_min_u = std::stod(intrinsicElem->Attribute("sensorMinU", "0"));
            camera.intrinsic.sensor_max_u = std::stod(intrinsicElem->Attribute("sensorMaxU", "0"));
            camera.intrinsic.sensor_min_v = std::stod(intrinsicElem->Attribute("sensorMinV", "0"));
            camera.intrinsic.sensor_max_v = std::stod(intrinsicElem->Attribute("sensorMaxV", "0"));
            camera.intrinsic.focal_length_u = std::stod(intrinsicElem->Attribute("focalLengthU", "0"));
            camera.intrinsic.focal_length_v = std::stod(intrinsicElem->Attribute("focalLengthV", "0"));
            camera.intrinsic.center_point_u = std::stod(intrinsicElem->Attribute("centerPointU", "0"));
            camera.intrinsic.center_point_v = std::stod(intrinsicElem->Attribute("centerPointV", "0"));
            camera.intrinsic.skew = std::stod(intrinsicElem->Attribute("skew", "0"));
            camera.intrinsic.radial_distortion_1 = std::stod(intrinsicElem->Attribute("radialDistortion1", "0"));
            camera.intrinsic.radial_distortion_2 = std::stod(intrinsicElem->Attribute("radialDistortion2", "0"));
            camera.intrinsic.radial_distortion_3 = std::stod(intrinsicElem->Attribute("radialDistortion3", "0"));
            camera.intrinsic.tangental_distortion_1 = std::stod(intrinsicElem->Attribute("tangentalDistortion1", "0"));
            camera.intrinsic.tangental_distortion_2 = std::stod(intrinsicElem->Attribute("tangentalDistortion2", "0"));

            // Add the parsed camera to the settings
            settings.cameras.push_back(camera);

            // Move to the next camera element
            cameraElem = cameraElem->NextSiblingElement("Camera");
        }
    }
    catch (const std::exception&)
    {
        return false;
    }

    mCalibrationSettings = settings;

    return true;
} // ReadCalibrationSettings


bool CRTProtocol::Read3DSettings(bool &bDataAvailable)
{
    tinyxml2::XMLDocument oXML;
    bDataAvailable = false;

    ms3DSettings.s3DLabels.clear();
    ms3DSettings.pCalibrationTime[0] = '\0';

    if (!ReadSettings("3D", oXML))
    {
        return false;
    }

    auto* root = oXML.RootElement();
    if (!root)
    {
        return false;
    }

    auto* the3DElem = root->FirstChildElement("The_3D");
    if (!the3DElem)
    {
        // No 3D data available.
        return true;
    }

    // AxisUpwards
    auto* axisUpwardsElem = the3DElem->FirstChildElement("AxisUpwards");
    if (!axisUpwardsElem || !axisUpwardsElem->GetText())
    {
        return false;
    }
    std::string axisUpwards = ToLower(axisUpwardsElem->GetText());

    if (axisUpwards == "+x") {
        ms3DSettings.eAxisUpwards = XPos;
    } else if (axisUpwards == "-x") {
        ms3DSettings.eAxisUpwards = XNeg;
    } else if (axisUpwards == "+y") {
        ms3DSettings.eAxisUpwards = YPos;
    } else if (axisUpwards == "-y") {
        ms3DSettings.eAxisUpwards = YNeg;
    } else if (axisUpwards == "+z") {
        ms3DSettings.eAxisUpwards = ZPos;
    } else if (axisUpwards == "-z") {
        ms3DSettings.eAxisUpwards = ZNeg;
    } else {
        return false;
    }

    // CalibrationTime
    auto* calibrationTimeElem = the3DElem->FirstChildElement("CalibrationTime");
    if (!calibrationTimeElem || !calibrationTimeElem->GetText())
    {
        return false;
    }
    strncpy(ms3DSettings.pCalibrationTime, calibrationTimeElem->GetText(), sizeof(ms3DSettings.pCalibrationTime) - 1);

    // Find the "Labels" element
    auto* labelsElem = the3DElem->FirstChildElement("Labels");
    if (!labelsElem || !labelsElem->GetText())
    {
        return false;
    }
    // Parse the number of labels
    unsigned int nNumberOfLabels = std::atoi(labelsElem->GetText());
    ms3DSettings.s3DLabels.resize(nNumberOfLabels);
    SSettings3DLabel sLabel;
    // Process each label
    auto* labelElem = labelsElem->FirstChildElement("Label");
    for (unsigned int iLabel = 0; iLabel < nNumberOfLabels; iLabel++)
    {
        if (!labelElem)
        {
            return false; // Missing label element
        }

        // Process the current label
        auto* nameElem = labelElem->FirstChildElement("Name");
        if (nameElem && nameElem->GetText())
        {
            sLabel.oName = nameElem->GetText();
        }

        auto* colorElem = labelElem->FirstChildElement("RGBColor");
        if (colorElem && colorElem->GetText())
        {
            sLabel.nRGBColor = std::atoi(colorElem->GetText());
        }

        auto* trajectoryElem = labelElem->FirstChildElement("Trajectory_Type");
        if (trajectoryElem && trajectoryElem->GetText())
        {
            sLabel.type = trajectoryElem->GetText();
        }

        ms3DSettings.s3DLabels[iLabel] = sLabel;

        // Move to the next label element
        labelElem = labelElem->NextSiblingElement("Label");
    }

    // Bones
    ms3DSettings.sBones.clear();
    auto* bonesElem = the3DElem->FirstChildElement("Bones");
    if (bonesElem)
    {
        auto* boneElem = bonesElem->FirstChildElement("Bone");
        while (boneElem)
        {
            SSettingsBone bone;
            bone.fromName = boneElem->Attribute("From", "");
            bone.toName = boneElem->Attribute("To", "");

            auto colorAttr = boneElem->Attribute("Color");
            if (colorAttr)
            {
                bone.color = std::atoi(colorAttr);
            }

            ms3DSettings.sBones.push_back(bone);
            boneElem = boneElem->NextSiblingElement("Bone");
        }
    }

    bDataAvailable = true;
    return true;
} // Read3DSettings

namespace
{
    bool TryReadSetEnabled(const int nMajorVer, const int nMinorVer, tinyxml2::XMLElement* oXML, bool& bTarget)
    {
        if (nMajorVer > 1 || nMinorVer > 23)
        {
            if (!oXML)
            {
                return false;
            }

            auto* enabledElem = oXML->FirstChildElement("Enabled");
            if (!enabledElem || !enabledElem->GetText())
            {
                bTarget = true; // 'true' if "Enabled" element is missing
                return true;
            }

            bTarget = (std::string(enabledElem->GetText()) == "true");
            return true;
        }

        return false;
    }

    bool TryReadSetName(tinyxml2::XMLElement* oXML, std::string& sTarget)
    {
        if (!oXML)
        {
            return false;
        }

        auto* nameElem = oXML->FirstChildElement("Name");
        if (!nameElem || !nameElem->GetText())
        {
            return false;
        }

        sTarget = nameElem->GetText();
        return true;
    }

    bool TryReadSetColor(tinyxml2::XMLElement* oXML, std::uint32_t& nTarget)
    {
        if (!oXML)
        {
            return false;
        }

        auto* colorElem = oXML->FirstChildElement("Color");
        if (!colorElem)
        {
            return false;
        }
        const char* rAttrib = colorElem->Attribute("R");
        const char* gAttrib = colorElem->Attribute("G");
        const char* bAttrib = colorElem->Attribute("B");

        std::uint32_t colorR = std::atoi(rAttrib);
        std::uint32_t colorG = std::atoi(gAttrib);
        std::uint32_t colorB = std::atoi(bAttrib);

        nTarget = (colorR & 0xff) | ((colorG << 8) & 0xff00) | ((colorB << 16) & 0xff0000);
        return true;
    }

    bool TryReadSetMaxResidual(tinyxml2::XMLElement* oXML, float& fTarget)
    {
        if (!oXML)
        {
            return false;
        }

        auto* maxResidualElem = oXML->FirstChildElement("MaximumResidual");
        if (!maxResidualElem || !maxResidualElem->GetText())
        {
            return false;
        }

        fTarget = static_cast<float>(std::atof(maxResidualElem->GetText()));
        return true;
    }

    bool TryReadSetMinMarkersInBody(tinyxml2::XMLElement* oXML, std::uint32_t& nTarget)
    {
        if (!oXML)
        {
            return false;
        }

        auto* minMarkersElem = oXML->FirstChildElement("MinimumMarkersInBody");
        if (!minMarkersElem || !minMarkersElem->GetText())
        {
            return false;
        }

        nTarget = std::atoi(minMarkersElem->GetText());
        return true;
    }

    bool TryReadSetBoneLenTolerance(tinyxml2::XMLElement* oXML, float& fTarget)
    {
        if (!oXML)
        {
            return false;
        }

        auto* boneLenToleranceElem = oXML->FirstChildElement("BoneLengthTolerance");
        if (!boneLenToleranceElem || !boneLenToleranceElem->GetText())
        {
            return false;
        }

        fTarget = static_cast<float>(std::atof(boneLenToleranceElem->GetText()));
        return true;
    }

    bool TryReadSetFilter(tinyxml2::XMLElement* oXML, std::string& sTarget)
    {
        if (!oXML)
        {
            return false;
        }

        auto* filterElem = oXML->FirstChildElement("Filter");
        if (!filterElem)
        {
            return false;
        }

        const char* presetAttrib = filterElem->Attribute("Preset");
        if (!presetAttrib)
        {
            return false;
        }

        sTarget = presetAttrib;
        return true;
    }

    bool TryReadSetPos(tinyxml2::XMLElement* oXML, float& fTargetX, float& fTargetY, float& fTargetZ)
    {
        if (!oXML)
        {
            return false;
        }

        auto* positionElem = oXML->FirstChildElement("Position");
        if (!positionElem)
        {
            return false;
        }

        const char* xAttrib = positionElem->Attribute("X");
        const char* yAttrib = positionElem->Attribute("Y");
        const char* zAttrib = positionElem->Attribute("Z");

        fTargetX = static_cast<float>(std::atof(xAttrib));
        fTargetY = static_cast<float>(std::atof(yAttrib));
        fTargetZ = static_cast<float>(std::atof(zAttrib));

        return true;
    }

    bool TryReadSetRotation(tinyxml2::XMLElement* oXML, float& fTargetX, float& fTargetY, float& fTargetZ)
    {
        if (!oXML)
        {
            return false;
        }

        auto* rotationElem = oXML->FirstChildElement("Rotation");
        if (!rotationElem)
        {
            return false;
        }

        const char* xAttrib = rotationElem->Attribute("X");
        const char* yAttrib = rotationElem->Attribute("Y");
        const char* zAttrib = rotationElem->Attribute("Z");

        fTargetX = static_cast<float>(std::atof(xAttrib));
        fTargetY = static_cast<float>(std::atof(yAttrib));
        fTargetZ = static_cast<float>(std::atof(zAttrib));

        return true;
    }

    bool TryReadSetScale(tinyxml2::XMLElement* oXML, float& fTarget)
    {
        if (!oXML)
        {
            return false;
        }

        auto* scaleElem = oXML->FirstChildElement("Scale");
        if (!scaleElem || !scaleElem->GetText())
        {
            return false;
        }

        fTarget = static_cast<float>(std::atof(scaleElem->GetText()));
        return true;
    }

    bool TryReadSetOpacity(tinyxml2::XMLElement* oXML, float& fTarget)
    {
        if (!oXML)
        {
            return false;
        }

        auto* opacityElem = oXML->FirstChildElement("Opacity");
        if (!opacityElem || !opacityElem->GetText())
        {
            return false;
        }

        fTarget = static_cast<float>(std::atof(opacityElem->GetText()));
        return true;
    }

    bool TryReadSetPoints(tinyxml2::XMLElement* oXML, std::vector<CRTProtocol::SBodyPoint>& vTarget)
    {
        if (!oXML)
        {
            return false;
        }

        auto* pointsElem = oXML->FirstChildElement("Points");
        if (!pointsElem)
        {
            return false;
        }

        auto* pointElem = pointsElem->FirstChildElement("Point");
        while (pointElem)
        {
            CRTProtocol::SBodyPoint sBodyPoint;

            // Extract attributes
            sBodyPoint.fX = pointElem->Attribute("X") ? static_cast<float>(std::atof(pointElem->Attribute("X"))) : 0.0f;
            sBodyPoint.fY = pointElem->Attribute("Y") ? static_cast<float>(std::atof(pointElem->Attribute("Y"))) : 0.0f;
            sBodyPoint.fZ = pointElem->Attribute("Z") ? static_cast<float>(std::atof(pointElem->Attribute("Z"))) : 0.0f;
            sBodyPoint.virtual_ = pointElem->Attribute("Virtual") ? (std::atoi(pointElem->Attribute("Virtual")) != 0) : false;
            sBodyPoint.physicalId = pointElem->Attribute("PhysicalId") ? std::atoi(pointElem->Attribute("PhysicalId")) : 0;
            sBodyPoint.name = pointElem->Attribute("Name") ? pointElem->Attribute("Name") : "";
            // Add the parsed point to the target vector
            vTarget.push_back(sBodyPoint);
            // Move to the next "Point" element
            pointElem = pointElem->NextSiblingElement("Point");
        }

        return true;
    }

    bool TryReadSetDataOrigin(tinyxml2::XMLElement* oXML, CRTProtocol::SOrigin& oTarget)
    {
        if (!oXML)
        {
            return false;
        }

        auto* dataOriginElem = oXML->FirstChildElement("Data_origin");
        if (!dataOriginElem || !dataOriginElem->GetText())
        {
            return false;
        }

        oTarget.type = static_cast<CRTProtocol::EOriginType>(std::atoi(dataOriginElem->GetText()));
        oTarget.position.fX = dataOriginElem->Attribute("X") ? static_cast<float>(std::atof(dataOriginElem->Attribute("X"))) : 0.0f;
        oTarget.position.fY = dataOriginElem->Attribute("Y") ? static_cast<float>(std::atof(dataOriginElem->Attribute("Y"))) : 0.0f;
        oTarget.position.fZ = dataOriginElem->Attribute("Z") ? static_cast<float>(std::atof(dataOriginElem->Attribute("Z"))) : 0.0f;
        oTarget.relativeBody = dataOriginElem->Attribute("Relative_body") ? std::atoi(dataOriginElem->Attribute("Relative_body")) : 0;

        return true;
    }

    void ReadSetRotations(tinyxml2::XMLElement* oXML, CRTProtocol::SOrigin& oTarget)
    {
        if (!oXML)
        {
            return;
        }

        char tmpStr[10];
        for (std::uint32_t i = 0; i < 9; i++)
        {
            sprintf(tmpStr, "R%u%u", (i / 3) + 1, (i % 3) + 1);
            oTarget.rotation[i] = oXML->Attribute(tmpStr) ? static_cast<float>(std::atof(oXML->Attribute(tmpStr))) : 0.0f;
        }
    }

    bool TryReadSetRGBColor(tinyxml2::XMLElement* oXML, std::uint32_t& oTarget)
    {
        if (!oXML)
        {
            return false;
        }

        auto* rgbColorElem = oXML->FirstChildElement("RGBColor");
        if (!rgbColorElem || !rgbColorElem->GetText())
        {
            return false;
        }
        oTarget = std::atoi(rgbColorElem->GetText());

        return true;
    }

    bool TryReadSetPointsOld(tinyxml2::XMLElement* oXML, std::vector<CRTProtocol::SBodyPoint>& vTarget)
    {
        if (!oXML)
        {
            return false;
        }

        vTarget.clear();

        auto* pointElem = oXML->FirstChildElement("Point");
        while (pointElem)
        {
            CRTProtocol::SBodyPoint sPoint;

            auto* xElem = pointElem->FirstChildElement("X");
            if (!xElem || !xElem->GetText())
            {
                return false;
            }
            sPoint.fX = static_cast<float>(std::atof(xElem->GetText()));

            auto* yElem = pointElem->FirstChildElement("Y");
            if (!yElem || !yElem->GetText())
            {
                return false;
            }
            sPoint.fY = static_cast<float>(std::atof(yElem->GetText()));

            auto* zElem = pointElem->FirstChildElement("Z");
            if (!zElem || !zElem->GetText())
            {
                return false;
            }
            sPoint.fZ = static_cast<float>(std::atof(zElem->GetText()));

            vTarget.push_back(sPoint);
            pointElem = pointElem->NextSiblingElement("Point");
        }

        return true;
    }

    bool TryReadSetEuler(tinyxml2::XMLElement* oXML, std::string& sTargetFirst, std::string& sTargetSecond, std::string& sTargetThird)
    {
        if (!oXML)
        {
            return false;
        }

        auto* eulerElem = oXML->FirstChildElement("Euler");
        if (!eulerElem)
        {
            return false;
        }

        auto* firstElem = eulerElem->FirstChildElement("First");
        if (!firstElem || !firstElem->GetText())
        {
            return false;
        }
        sTargetFirst = firstElem->GetText();

        auto* secondElem = eulerElem->FirstChildElement("Second");
        if (!secondElem || !secondElem->GetText())
        {
            return false;
        }
        sTargetSecond = secondElem->GetText();

        auto* thirdElem = eulerElem->FirstChildElement("Third");
        if (!thirdElem || !thirdElem->GetText())
        {
            return false;
        }
        sTargetThird = thirdElem->GetText();

        return true;
    }
}

bool CRTProtocol::Read6DOFSettings(bool& bDataAvailable)
{
    tinyxml2::XMLDocument oXML;

    bDataAvailable = false;

    m6DOFSettings.clear();

    if (!ReadSettings("6D", oXML))
    {
        return false;
    }

    auto* root = oXML.RootElement();
    if (!root)
    {
        return false;
    }

    auto* the6DElem = root->FirstChildElement("The_6D");
    if (!the6DElem)
    {
        return true; // No 6D data available
    }

    if (mnMajorVersion > 1 || mnMinorVersion > 20)
    {
        auto* bodyElem = the6DElem->FirstChildElement("Body");
        while (bodyElem)
        {
            SSettings6DOFBody s6DOFBodySettings;

            // NOTE: READ-ORDER MATTERS!!!
            if (!TryReadSetName(bodyElem, s6DOFBodySettings.name))
            { // Name --- REQUIRED
                return false;
            }
            // Enabled --- NOT(!) REQUIRED
            TryReadSetEnabled(mnMajorVersion, mnMinorVersion, bodyElem, s6DOFBodySettings.enabled);
            if (!TryReadSetColor(bodyElem, s6DOFBodySettings.color) ||
                !TryReadSetMaxResidual(bodyElem, s6DOFBodySettings.maxResidual) ||
                !TryReadSetMinMarkersInBody(bodyElem, s6DOFBodySettings.minMarkersInBody) ||
                !TryReadSetBoneLenTolerance(bodyElem, s6DOFBodySettings.boneLengthTolerance) ||
                !TryReadSetFilter(bodyElem, s6DOFBodySettings.filterPreset))
            { // Color, MaxResidual, MinMarkersInBody, BoneLengthTolerance, Filter --- REQUIRED
                return false;
            }

            auto* meshElem = bodyElem->FirstChildElement("Mesh");
            if (meshElem)
            {
                if (!TryReadSetName(meshElem, s6DOFBodySettings.mesh.name) ||
                    !TryReadSetPos(meshElem, s6DOFBodySettings.mesh.position.fX, s6DOFBodySettings.mesh.position.fY, s6DOFBodySettings.mesh.position.fZ) ||
                    !TryReadSetRotation(meshElem, s6DOFBodySettings.mesh.rotation.fX, s6DOFBodySettings.mesh.rotation.fY, s6DOFBodySettings.mesh.rotation.fZ) ||
                    !TryReadSetScale(meshElem, s6DOFBodySettings.mesh.scale) ||
                    !TryReadSetOpacity(meshElem, s6DOFBodySettings.mesh.opacity))
                { // Name, Position, Rotation, Scale, Opacity --- REQUIRED
                    return false;
                }
            }

            // Points --- REQUIRED
            TryReadSetPoints(bodyElem, s6DOFBodySettings.points);

            auto* dataOriginElem = bodyElem->FirstChildElement("Data_origin");
            auto* dataOrientationElem = bodyElem->FirstChildElement("Data_orientation");
            if (!dataOriginElem || !dataOrientationElem ||
                !TryReadSetDataOrigin(dataOriginElem, s6DOFBodySettings.origin) ||
                s6DOFBodySettings.origin.type != std::atoi(dataOrientationElem->GetText()) ||
                s6DOFBodySettings.origin.relativeBody != static_cast<std::uint32_t>(std::atoi(dataOrientationElem->Attribute("Relative_body", "0"))))
            { // Data Orientation, Origin Type / Relative Body --- REQUIRED
                return false;
            }

            // Rotation values --- NOTE : Does NOT(!) 'Try'; just reads and sets (no boolean return)
            ReadSetRotations(bodyElem, s6DOFBodySettings.origin);

            m6DOFSettings.push_back(s6DOFBodySettings);
            bodyElem = bodyElem->NextSiblingElement("Body");

            bDataAvailable = true;
        }
    }
    else
    {
        auto* bodiesElem = the6DElem->FirstChildElement("Bodies");
        if (!bodiesElem || !bodiesElem->GetText())
        {
            return false;
        }

        SSettings6DOFBody s6DOFBodySettings;

        int nBodies = std::atoi(bodiesElem->GetText());
        for (int iBody = 0; iBody < nBodies; iBody++)
        {
            auto* bodyElem = bodiesElem->FirstChildElement("Body");
            if (!bodyElem)
            {
                return false;
            }

            SSettings6DOFBody s6DOFBodySettings;

            if (!TryReadSetName(bodyElem, s6DOFBodySettings.name) ||
                !TryReadSetRGBColor(bodyElem, s6DOFBodySettings.color) ||
                !TryReadSetPointsOld(bodyElem, s6DOFBodySettings.points))
            { // Name, RGBColor, Points(OLD) --- REQUIRED
                return false;
            }

            m6DOFSettings.push_back(s6DOFBodySettings);
            bodyElem = bodyElem->NextSiblingElement("Body");
        }

        if (mnMajorVersion > 1 || mnMinorVersion > 15)
        {
            if (!TryReadSetEuler(the6DElem, msGeneralSettings.eulerRotations[0], msGeneralSettings.eulerRotations[1], msGeneralSettings.eulerRotations[2])) // Euler --- REQUIRED
            {
                return false;
            }
        }

        bDataAvailable = true;
    }

    return true;
} // Read6DOFSettings

bool CRTProtocol::ReadGazeVectorSettings(bool& bDataAvailable)
{
    tinyxml2::XMLDocument oXML;
    bDataAvailable = false;
    mvsGazeVectorSettings.clear();

    if (!ReadSettings("GazeVector", oXML))
    {
        return false;
    }

    // Root element
    auto* root = oXML.RootElement();
    if (!root)
    {
        return false;
    }

    // Read gaze vectors
    auto* gazeVectorElem = root->FirstChildElement("Gaze_Vector");
    if (!gazeVectorElem)
    {
        return true; // No gaze vector data available
    }

    auto* vectorElem = gazeVectorElem->FirstChildElement("Vector");
    while (vectorElem)
    {
        auto* nameElem = vectorElem->FirstChildElement("Name");
        if (!nameElem || !nameElem->GetText())
        {
            return false;
        }
        std::string tGazeVectorName = nameElem->GetText();

        float frequency = 0.0f;
        auto* frequencyElem = vectorElem->FirstChildElement("Frequency");
        if (frequencyElem && frequencyElem->GetText())
        {
            frequency = static_cast<float>(std::atof(frequencyElem->GetText()));
        }

        bool hwSync = false;
        ReadXmlBool(vectorElem, "Hardware_Sync", hwSync);

        bool filter = false;
        ReadXmlBool(vectorElem, "Filter", filter);

        // Add to gaze vector settings
        mvsGazeVectorSettings.push_back({ tGazeVectorName, frequency, hwSync, filter });
        // Move to the next "Vector" element
        vectorElem = vectorElem->NextSiblingElement("Vector");
    }

    bDataAvailable = true;
    return true;
} // ReadGazeVectorSettings

bool CRTProtocol::ReadEyeTrackerSettings(bool& bDataAvailable)
{
    tinyxml2::XMLDocument oXML;
    bDataAvailable = false;
    mvsEyeTrackerSettings.clear();

    if (!ReadSettings("EyeTracker", oXML))
    {
        return false;
    }

    auto* root = oXML.RootElement();
    if (!root)
    {
        return false;
    }

    auto* eyeTrackerElem = root->FirstChildElement("Eye_Tracker");
    if (!eyeTrackerElem)
    {
        return true; // No eye tracker data available
    }

    auto* deviceElem = eyeTrackerElem->FirstChildElement("Device");
    while (deviceElem)
    {
        auto* nameElem = deviceElem->FirstChildElement("Name");
        if (!nameElem || !nameElem->GetText())
        {
            return false;
        }
        std::string tEyeTrackerName = nameElem->GetText();

        float frequency = 0.0f;
        auto* frequencyElem = deviceElem->FirstChildElement("Frequency");
        if (frequencyElem && frequencyElem->GetText())
        {
            frequency = static_cast<float>(std::atof(frequencyElem->GetText()));
        }

        bool hwSync = false;
        ReadXmlBool(deviceElem, "Hardware_Sync", hwSync);

        // Add to eye tracker settings
        mvsEyeTrackerSettings.push_back({ tEyeTrackerName, frequency, hwSync });
        // Move to the next "Device" element
        deviceElem = deviceElem->NextSiblingElement("Device");
    }

    bDataAvailable = true;
    return true;
} // ReadEyeTrackerSettings

bool CRTProtocol::ReadAnalogSettings(bool& bDataAvailable)
{
    tinyxml2::XMLDocument oXML;
    bDataAvailable = false;
    mvsAnalogDeviceSettings.clear();

    auto* root = oXML.RootElement();
    if (!root)
    {
        return false;
    }

    if (!ReadSettings("Analog", oXML))
    {
        return false;
    }

    auto* analogElem = root->FirstChildElement("Analog");
    if (!analogElem)
    {
        return true;
    }

    SAnalogDevice sAnalogDevice;

    if (mnMajorVersion == 1 && mnMinorVersion == 0)
    {
        // For protocol version 1.0
        sAnalogDevice.nDeviceID = 1;   // Always channel 1
        sAnalogDevice.oName = "AnalogDevice";

        auto* channelsElem = analogElem->FirstChildElement("Channels");
        if (!channelsElem || !channelsElem->GetText())
        {
            return false;
        }
        sAnalogDevice.nChannels = std::atoi(channelsElem->GetText());

        auto* frequencyElem = analogElem->FirstChildElement("Frequency");
        if (!frequencyElem || !frequencyElem->GetText())
        {
            return false;
        }
        sAnalogDevice.nFrequency = std::atoi(frequencyElem->GetText());

        auto* unitElem = analogElem->FirstChildElement("Unit");
        if (!unitElem || !unitElem->GetText())
        {
            return false;
        }
        sAnalogDevice.oUnit = unitElem->GetText();

        auto* rangeElem = analogElem->FirstChildElement("Range");
        if (!rangeElem)
        {
            return false;
        }

        auto* minElem = rangeElem->FirstChildElement("Min");
        if (!minElem || !minElem->GetText())
        {
            return false;
        }
        sAnalogDevice.fMinRange = static_cast<float>(std::atof(minElem->GetText()));

        auto* maxElem = rangeElem->FirstChildElement("Max");
        if (!maxElem || !maxElem->GetText())
        {
            return false;
        }
        sAnalogDevice.fMaxRange = static_cast<float>(std::atof(maxElem->GetText()));

        mvsAnalogDeviceSettings.push_back(sAnalogDevice);
        bDataAvailable = true;
        return true;
    }
    else
    {
        auto* deviceElem = analogElem->FirstChildElement("Device");
        while (deviceElem)
        {
            sAnalogDevice.voLabels.clear();
            sAnalogDevice.voUnits.clear();

            auto* deviceIdElem = deviceElem->FirstChildElement("Device_ID");
            if (!deviceIdElem || !deviceIdElem->GetText())
            {
                deviceElem = deviceElem->NextSiblingElement("Device");
                continue;
            }
            sAnalogDevice.nDeviceID = std::atoi(deviceIdElem->GetText());

            auto* deviceNameElem = deviceElem->FirstChildElement("Device_Name");
            if (!deviceNameElem || !deviceNameElem->GetText())
            {
                deviceElem = deviceElem->NextSiblingElement("Device");
                continue;
            }
            sAnalogDevice.oName = deviceNameElem->GetText();

            auto* channelsElem = deviceElem->FirstChildElement("Channels");
            if (!channelsElem || !channelsElem->GetText())
            {
                deviceElem = deviceElem->NextSiblingElement("Device");
                continue;
            }
            sAnalogDevice.nChannels = std::atoi(channelsElem->GetText());

            auto* frequencyElem = deviceElem->FirstChildElement("Frequency");
            if (!frequencyElem || !frequencyElem->GetText())
            {
                deviceElem = deviceElem->NextSiblingElement("Device");
                continue;
            }
            sAnalogDevice.nFrequency = std::atoi(frequencyElem->GetText());

            if (mnMajorVersion == 1 && mnMinorVersion < 11)
            {
                auto* unitElem = deviceElem->FirstChildElement("Unit");
                if (!unitElem || !unitElem->GetText())
                {
                    deviceElem = deviceElem->NextSiblingElement("Device");
                    continue;
                }
                sAnalogDevice.oUnit = unitElem->GetText();
            }

            auto* rangeElem = deviceElem->FirstChildElement("Range");
            if (!rangeElem)
            {
                deviceElem = deviceElem->NextSiblingElement("Device");
                continue;
            }

            auto* minElem = rangeElem->FirstChildElement("Min");
            if (!minElem || !minElem->GetText())
            {
                deviceElem = deviceElem->NextSiblingElement("Device");
                continue;
            }
            sAnalogDevice.fMinRange = static_cast<float>(std::atof(minElem->GetText()));

            auto* maxElem = rangeElem->FirstChildElement("Max");
            if (!maxElem || !maxElem->GetText())
            {
                deviceElem = deviceElem->NextSiblingElement("Device");
                continue;
            }
            sAnalogDevice.fMaxRange = static_cast<float>(std::atof(maxElem->GetText()));

            if (mnMajorVersion == 1 && mnMinorVersion < 11)
            {
                for (unsigned int i = 0; i < sAnalogDevice.nChannels; ++i)
                {
                    auto* labelElem = deviceElem->FirstChildElement("Label");
                    if (labelElem && labelElem->GetText())
                    {
                        sAnalogDevice.voLabels.push_back(labelElem->GetText());
                    }
                }
                if (sAnalogDevice.voLabels.size() != sAnalogDevice.nChannels)
                {
                    deviceElem = deviceElem->NextSiblingElement("Device");
                    continue;
                }
            }
            else
            {
                auto* channelElem = deviceElem->FirstChildElement("Channel");
                while (channelElem)
                {
                    auto* labelElem = channelElem->FirstChildElement("Label");
                    if (labelElem && labelElem->GetText())
                    {
                        sAnalogDevice.voLabels.push_back(labelElem->GetText());
                    }

                    auto* unitElem = channelElem->FirstChildElement("Unit");
                    if (unitElem && unitElem->GetText())
                    {
                        sAnalogDevice.voUnits.push_back(unitElem->GetText());
                    }

                    channelElem = channelElem->NextSiblingElement("Channel");
                }

                if (sAnalogDevice.voLabels.size() != sAnalogDevice.nChannels ||
                    sAnalogDevice.voUnits.size() != sAnalogDevice.nChannels)
                {
                    deviceElem = deviceElem->NextSiblingElement("Device");
                    continue;
                }
            }

            mvsAnalogDeviceSettings.push_back(sAnalogDevice);
            bDataAvailable = true;

            deviceElem = deviceElem->NextSiblingElement("Device");
        }
    }

    return true;
} // ReadAnalogSettings

bool CRTProtocol::ReadForceSettings(bool& bDataAvailable)
{
    tinyxml2::XMLDocument oXML;

    bDataAvailable = false;
    msForceSettings.vsForcePlates.clear();

    auto* root = oXML.RootElement();
    if (!root)
    {
        return false;
    }

    if (!ReadSettings("Force", oXML))
    {
        return false;
    }

    auto* forceElem = root->FirstChildElement("Force");
    if (!forceElem)
    {
        return true;
    }

    auto* unitLengthElem = forceElem->FirstChildElement("Unit_Length");
    if (!unitLengthElem || !unitLengthElem->GetText())
    {
        return false;
    }
    msForceSettings.oUnitLength = unitLengthElem->GetText();

    auto* unitForceElem = forceElem->FirstChildElement("Unit_Force");
    if (!unitForceElem || !unitForceElem->GetText())
    {
        return false;
    }
    msForceSettings.oUnitForce = unitForceElem->GetText();

    auto* plateElem = forceElem->FirstChildElement("Plate");
    while (plateElem)
    {
        SForcePlate sForcePlate = {};
        sForcePlate.bValidCalibrationMatrix = false;
        sForcePlate.nCalibrationMatrixRows = 6;
        sForcePlate.nCalibrationMatrixColumns = 6;

        // Get name and type of the plates
        auto* indexElem = plateElem->FirstChildElement("Force_Plate_Index");
        if (!indexElem)
        {
            indexElem = plateElem->FirstChildElement("Plate_ID");
        }
        if (!indexElem || !indexElem->GetText())
        {
            return false;
        }
        sForcePlate.nID = std::atoi(indexElem->GetText());

        auto* analogDeviceElem = plateElem->FirstChildElement("Analog_Device_ID");
        if (analogDeviceElem && analogDeviceElem->GetText())
        {
            sForcePlate.nAnalogDeviceID = std::atoi(analogDeviceElem->GetText());
        }
        else
        {
            sForcePlate.nAnalogDeviceID = 0;
        }

        auto* frequencyElem = plateElem->FirstChildElement("Frequency");
        if (!frequencyElem || !frequencyElem->GetText())
        {
            return false;
        }
        sForcePlate.nFrequency = std::atoi(frequencyElem->GetText());

        auto* typeElem = plateElem->FirstChildElement("Type");
        if (typeElem && typeElem->GetText())
        {
            sForcePlate.oType = typeElem->GetText();
        }
        else
        {
            sForcePlate.oType = "unknown";
        }

        auto* nameElem = plateElem->FirstChildElement("Name");
        if (nameElem && nameElem->GetText())
        {
            sForcePlate.oName = nameElem->GetText();
        }
        else
        {
            sForcePlate.oName = "Unnamed Plate";
        }

        auto* lengthElem = plateElem->FirstChildElement("Length");
        if (lengthElem && lengthElem->GetText())
        {
            sForcePlate.fLength = static_cast<float>(std::atof(lengthElem->GetText()));
        }

        auto* widthElem = plateElem->FirstChildElement("Width");
        if (widthElem && widthElem->GetText())
        {
            sForcePlate.fWidth = static_cast<float>(std::atof(widthElem->GetText()));
        }

        auto* locationElem = plateElem->FirstChildElement("Location");
        if (locationElem)
        {
            for (int i = 0; i < 4; ++i)
            {
                std::string cornerName = "Corner" + std::to_string(i + 1);
                auto* cornerElem = locationElem->FirstChildElement(cornerName.c_str());
                if (cornerElem)
                {
                    auto* xElem = cornerElem->FirstChildElement("X");
                    auto* yElem = cornerElem->FirstChildElement("Y");
                    auto* zElem = cornerElem->FirstChildElement("Z");

                    sForcePlate.asCorner[i].fX = xElem && xElem->GetText() ? static_cast<float>(std::atof(xElem->GetText())) : 0.0f;
                    sForcePlate.asCorner[i].fY = yElem && yElem->GetText() ? static_cast<float>(std::atof(yElem->GetText())) : 0.0f;
                    sForcePlate.asCorner[i].fZ = zElem && zElem->GetText() ? static_cast<float>(std::atof(zElem->GetText())) : 0.0f;
                }
            }
        }

        auto* originElem = plateElem->FirstChildElement("Origin");
        if (originElem)
        {
            auto* xElem = originElem->FirstChildElement("X");
            auto* yElem = originElem->FirstChildElement("Y");
            auto* zElem = originElem->FirstChildElement("Z");

            sForcePlate.sOrigin.fX = xElem && xElem->GetText() ? static_cast<float>(std::atof(xElem->GetText())) : 0.0f;
            sForcePlate.sOrigin.fY = yElem && yElem->GetText() ? static_cast<float>(std::atof(yElem->GetText())) : 0.0f;
            sForcePlate.sOrigin.fZ = zElem && zElem->GetText() ? static_cast<float>(std::atof(zElem->GetText())) : 0.0f;
        }
        
        sForcePlate.vChannels.clear();
        if (auto* channelsElem = plateElem->FirstChildElement("Channels"))
        {
            auto* channelElem = channelsElem->FirstChildElement("Channel");

            while (channelElem)
            {
                SForceChannel sForceChannel;
                auto* channelNoElem = channelElem->FirstChildElement("Channel_No");
                if (channelNoElem && channelNoElem->GetText())
                {
                    sForceChannel.nChannelNumber = std::atoi(channelNoElem->GetText());
                }

                auto* conversionFactorElem = channelElem->FirstChildElement("ConversionFactor");
                if (conversionFactorElem && conversionFactorElem->GetText())
                {
                    sForceChannel.fConversionFactor = static_cast<float>(std::atof(conversionFactorElem->GetText()));
                }

                sForcePlate.vChannels.push_back(sForceChannel);
                channelElem = channelElem->NextSiblingElement("Channel");
            }
        }

        if (auto* calibrationMatrixElem = plateElem->FirstChildElement("Calibration_Matrix"))
        {
            int nRow = 0;

            if (mnMajorVersion == 1 && mnMinorVersion < 12)
            {
                char strRow[16];
                char strCol[16];
                sprintf(strRow, "Row%d", nRow + 1);

                auto* rowElem = calibrationMatrixElem->FirstChildElement(strRow);
                while (rowElem)
                {
                    int nCol = 0;
                    sprintf(strCol, "Col%d", nCol + 1);

                    auto* colElem = rowElem->FirstChildElement(strCol);
                    while (colElem)
                    {
                        if (colElem->GetText())
                        {
                            sForcePlate.afCalibrationMatrix[nRow][nCol] = static_cast<float>(std::atof(colElem->GetText()));
                        }
                        nCol++;
                        sprintf(strCol, "Col%d", nCol + 1);
                        colElem = rowElem->FirstChildElement(strCol);
                    }

                    sForcePlate.nCalibrationMatrixColumns = nCol;
                    nRow++;
                    sprintf(strRow, "Row%d", nRow + 1);
                    rowElem = calibrationMatrixElem->FirstChildElement(strRow);
                }
            }
            else
            {
                auto* rowsElem = calibrationMatrixElem->FirstChildElement("Rows");
                if (rowsElem)
                {
                    auto* rowElem = rowsElem->FirstChildElement("Row");
                    while (rowElem)
                    {
                        auto* columnsElem = rowElem->FirstChildElement("Columns");
                        if (columnsElem)
                        {
                            int nCol = 0;
                            auto* colElem = columnsElem->FirstChildElement("Column");
                            while (colElem)
                            {
                                if (colElem->GetText())
                                {
                                    sForcePlate.afCalibrationMatrix[nRow][nCol] = static_cast<float>(std::atof(colElem->GetText()));
                                }
                                nCol++;
                                colElem = colElem->NextSiblingElement("Column");
                            }

                            sForcePlate.nCalibrationMatrixColumns = nCol;
                        }
                        nRow++;
                        rowElem = rowElem->NextSiblingElement("Row");
                    }
                }
            }

            sForcePlate.nCalibrationMatrixRows = nRow;
            sForcePlate.bValidCalibrationMatrix = true;
        }

        // End of processing for this force plate
        bDataAvailable = true;
        msForceSettings.vsForcePlates.push_back(sForcePlate);
    }

    return true;
} // Read force settings

bool CRTProtocol::ReadImageSettings(bool &bDataAvailable)
{
    tinyxml2::XMLDocument oXML;

    bDataAvailable = false;

    mvsImageSettings.clear();

    if (!ReadSettings("Image", oXML))
    {
        return false;
    }

    tinyxml2::XMLElement* root = oXML.RootElement();
    if (!root || std::string(root->Name()) != "Image")
    {
        return true;
    }

    for (tinyxml2::XMLElement* cameraElem = root->FirstChildElement("Camera");
        cameraElem != nullptr;
        cameraElem = cameraElem->NextSiblingElement("Camera"))
    {
        SImageCamera sImageCamera;

        tinyxml2::XMLElement* idElem = cameraElem->FirstChildElement("ID");
        if (!idElem || !idElem->GetText())
        {
            return false;
        }
        sImageCamera.nID = atoi(idElem->GetText());

        tinyxml2::XMLElement* enabledElem = cameraElem->FirstChildElement("Enabled");
        if (!enabledElem || !enabledElem->GetText())
        {
            return false;
        }
        std::string tStr = ToLower(enabledElem->GetText());
        sImageCamera.bEnabled = (tStr == "true");

        tinyxml2::XMLElement* formatElem = cameraElem->FirstChildElement("Format");
        if (!formatElem || !formatElem->GetText())
        {
            return false;
        }
        tStr = ToLower(formatElem->GetText());
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

        tinyxml2::XMLElement* widthElem = cameraElem->FirstChildElement("Width");
        if (!widthElem || !widthElem->GetText())
        {
            return false;
        }
        sImageCamera.nWidth = atoi(widthElem->GetText());

        tinyxml2::XMLElement* heightElem = cameraElem->FirstChildElement("Height");
        if (!heightElem || !heightElem->GetText())
        {
            return false;
        }
        sImageCamera.nHeight = atoi(heightElem->GetText());

        tinyxml2::XMLElement* leftCropElem = cameraElem->FirstChildElement("Left_Crop");
        if (!leftCropElem || !leftCropElem->GetText())
        {
            return false;
        }
        sImageCamera.fCropLeft = static_cast<float>(atof(leftCropElem->GetText()));

        tinyxml2::XMLElement* topCropElem = cameraElem->FirstChildElement("Top_Crop");
        if (!topCropElem || !topCropElem->GetText())
        {
            return false;
        }
        sImageCamera.fCropTop = static_cast<float>(atof(topCropElem->GetText()));

        tinyxml2::XMLElement* rightCropElem = cameraElem->FirstChildElement("Right_Crop");
        if (!rightCropElem || !rightCropElem->GetText())
        {
            return false;
        }
        sImageCamera.fCropRight = static_cast<float>(atof(rightCropElem->GetText()));

        tinyxml2::XMLElement* bottomCropElem = cameraElem->FirstChildElement("Bottom_Crop");
        if (!bottomCropElem || !bottomCropElem->GetText())
        {
            return false;
        }
        sImageCamera.fCropBottom = static_cast<float>(atof(bottomCropElem->GetText()));

        mvsImageSettings.push_back(sImageCamera);
        bDataAvailable = true;
    }

    return true;
}// ReadImageSettings

bool CRTProtocol::ReadSkeletonSettings(bool& dataAvailable, bool skeletonGlobalData)
{
    tinyxml2::XMLDocument doc;
    dataAvailable = false;

    mSkeletonSettings.clear();
    mSkeletonSettingsHierarchical.clear();

    // Load the XML document
    if (!ReadSettings(skeletonGlobalData ? "Skeleton:global" : "Skeleton", doc))
    {
        return false;
    }

    int segmentIndex;
    std::map<int, int> segmentIdIndexMap;

    // Access the root element
    tinyxml2::XMLElement* root = doc.RootElement();
    if (!root || strcmp(root->Name(), "QTM_Settings") != 0)
    {
        sprintf(maErrorStr, "Root element QTM_Settings not found");
        return false;
    }

    // Navigate to Skeletons
    tinyxml2::XMLElement* skeletonsElem = root->FirstChildElement("Skeletons");
    if (!skeletonsElem)
    {
        sprintf(maErrorStr, "Skeletons element not found");
        return false;
    }

    // Iterate through Skeleton elements
    for (tinyxml2::XMLElement* skeletonElem = skeletonsElem->FirstChildElement("Skeleton"); skeletonElem; skeletonElem = skeletonElem->NextSiblingElement("Skeleton"))
    {
        SSettingsSkeletonHierarchical skeletonHierarchical;
        SSettingsSkeleton skeleton;
        segmentIndex = 0;

        // Skeleton name
        const char* skeletonName = skeletonElem->Attribute("Name");
        skeletonHierarchical.name = skeletonName ? skeletonName : "";
        skeleton.name = skeletonHierarchical.name;

        // Scale
        tinyxml2::XMLElement* scaleElem = skeletonElem->FirstChildElement("Scale");
        if (scaleElem)
        {
            if (!ParseString(scaleElem->GetText(), skeletonHierarchical.scale))
            {
                sprintf(maErrorStr, "Scale element parse error");
                return false;
            }
        }

        // Segments
        tinyxml2::XMLElement* segmentsElem = skeletonElem->FirstChildElement("Segments");
        if (segmentsElem)
        {
            // Define the recursive function
            std::function<void(tinyxml2::XMLElement*, SSettingsSkeletonSegmentHierarchical&, std::vector<SSettingsSkeletonSegment>&, int)> recurseSegments =
                [&](tinyxml2::XMLElement* segmentElem, SSettingsSkeletonSegmentHierarchical& segmentHierarchical, std::vector<SSettingsSkeletonSegment>& segments, int parentId)
                {
                    // Segment attributes
                    const char* segmentName = segmentElem->Attribute("Name");
                    segmentHierarchical.name = segmentName ? segmentName : "";

                    // Handle missing ID by generating one dynamically
                    const char* segmentIdStr = segmentElem->Attribute("ID");
                    if (segmentIdStr)
                    {
                        ParseString(segmentIdStr, segmentHierarchical.id);
                    }
                    else
                    {
                        // Dynamically assign an ID if it's missing
                        segmentHierarchical.id = segmentIndex++;
                    }

                    segmentIdIndexMap[segmentHierarchical.id] = segmentIndex++;

                    // Solver
                    tinyxml2::XMLElement* solverElem = segmentElem->FirstChildElement("Solver");
                    if (solverElem)
                    {
                        segmentHierarchical.solver = solverElem->GetText() ? solverElem->GetText() : "";
                    }

                    // Transform
                    tinyxml2::XMLElement* transformElem = segmentElem->FirstChildElement("Transform");
                    if (transformElem)
                    {
                        segmentHierarchical.position = ReadXMLPosition(*transformElem, "Position");
                        segmentHierarchical.rotation = ReadXMLRotation(*transformElem, "Rotation");
                    }

                    // Default Transform
                    tinyxml2::XMLElement* defaultTransformElem = segmentElem->FirstChildElement("DefaultTransform");
                    if (defaultTransformElem)
                    {
                        segmentHierarchical.defaultPosition = ReadXMLPosition(*defaultTransformElem, "Position");
                        segmentHierarchical.defaultRotation = ReadXMLRotation(*defaultTransformElem, "Rotation");
                    }

                    // DegreesOfFreedom
                    tinyxml2::XMLElement* dofElem = segmentElem->FirstChildElement("DegreesOfFreedom");
                    if (dofElem)
                    {
                        ReadXMLDegreesOfFreedom(*dofElem, "RotationX", segmentHierarchical.degreesOfFreedom);
                        ReadXMLDegreesOfFreedom(*dofElem, "RotationY", segmentHierarchical.degreesOfFreedom);
                        ReadXMLDegreesOfFreedom(*dofElem, "RotationZ", segmentHierarchical.degreesOfFreedom);
                        ReadXMLDegreesOfFreedom(*dofElem, "TranslationX", segmentHierarchical.degreesOfFreedom);
                        ReadXMLDegreesOfFreedom(*dofElem, "TranslationY", segmentHierarchical.degreesOfFreedom);
                        ReadXMLDegreesOfFreedom(*dofElem, "TranslationZ", segmentHierarchical.degreesOfFreedom);
                    }

                    // Endpoint
                    segmentHierarchical.endpoint = ReadXMLPosition(*segmentElem, "Endpoint");

                    // Markers
                    tinyxml2::XMLElement* markersElem = segmentElem->FirstChildElement("Markers");
                    if (markersElem)
                    {
                        for (tinyxml2::XMLElement* markerElem = markersElem->FirstChildElement("Marker"); markerElem; markerElem = markerElem->NextSiblingElement("Marker"))
                        {
                            SMarker marker;
                            marker.name = markerElem->Attribute("Name");
                            marker.position = ReadXMLPosition(*markerElem, "Position");
                            ParseString(markerElem->FirstChildElement("Weight")->GetText(), marker.weight);
                            segmentHierarchical.markers.push_back(marker);
                        }
                    }

                    // RigidBodies
                    tinyxml2::XMLElement* rigidBodiesElem = segmentElem->FirstChildElement("RigidBodies");
                    if (rigidBodiesElem)
                    {
                        for (tinyxml2::XMLElement* bodyElem = rigidBodiesElem->FirstChildElement("RigidBody"); bodyElem; bodyElem = bodyElem->NextSiblingElement("RigidBody"))
                        {
                            SBody body;
                            body.name = bodyElem->Attribute("Name");
                            body.position = ReadXMLPosition(*bodyElem->FirstChildElement("Transform"), "Position");
                            body.rotation = ReadXMLRotation(*bodyElem->FirstChildElement("Transform"), "Rotation");
                            ParseString(bodyElem->FirstChildElement("Weight")->GetText(), body.weight);
                            segmentHierarchical.bodies.push_back(body);
                        }
                    }

                    // Push segment data
                    SSettingsSkeletonSegment segment;
                    segment.name = segmentHierarchical.name;
                    segment.id = segmentHierarchical.id;
                    segment.parentId = parentId;
                    segment.parentIndex = parentId != -1 ? segmentIdIndexMap[parentId] : -1;
                    segments.push_back(segment);

                    // Recurse into child segments
                    for (tinyxml2::XMLElement* childSegmentElem = segmentElem->FirstChildElement("Segment"); childSegmentElem; childSegmentElem = childSegmentElem->NextSiblingElement("Segment"))
                    {
                        SSettingsSkeletonSegmentHierarchical childSegment;
                        recurseSegments(childSegmentElem, childSegment, segments, segmentHierarchical.id);
                        segmentHierarchical.segments.push_back(childSegment);
                    }
                };

            // Start recursion from top-level segments
            for (tinyxml2::XMLElement* segmentElem = segmentsElem->FirstChildElement("Segment"); segmentElem; segmentElem = segmentElem->NextSiblingElement("Segment"))
            {
                recurseSegments(segmentElem, skeletonHierarchical.rootSegment, skeleton.segments, -1);
            }
        }

        mSkeletonSettingsHierarchical.push_back(skeletonHierarchical);
        mSkeletonSettings.push_back(skeleton);
    }

    dataAvailable = true;
    return true;
}


CRTProtocol::SPosition CRTProtocol::ReadXMLPosition(tinyxml2::XMLElement& element, const std::string& subElement)
{
    SPosition position;
    tinyxml2::XMLElement* subElem = element.FirstChildElement(subElement.c_str());
    if (subElem)
    {
        ParseString(subElem->Attribute("X"), position.x);
        ParseString(subElem->Attribute("Y"), position.y);
        ParseString(subElem->Attribute("Z"), position.z);
    }
    return position;
}

CRTProtocol::SRotation CRTProtocol::ReadXMLRotation(tinyxml2::XMLElement& element, const std::string& subElement)
{
    SRotation rotation;
    tinyxml2::XMLElement* subElem = element.FirstChildElement(subElement.c_str());
    if (subElem)
    {
        ParseString(subElem->Attribute("X"), rotation.x);
        ParseString(subElem->Attribute("Y"), rotation.y);
        ParseString(subElem->Attribute("Z"), rotation.z);
        ParseString(subElem->Attribute("W"), rotation.w);
    }
    return rotation;
}

bool CRTProtocol::ReadXMLDegreesOfFreedom(tinyxml2::XMLElement& element, const std::string& subElement, std::vector<SDegreeOfFreedom>& degreesOfFreedom)
{
    // Locate the sub-element
    tinyxml2::XMLElement* subElem = element.FirstChildElement(subElement.c_str());
    if (!subElem)
    {
        return false; // Sub-element is missing, gracefully exit
    }

    SDegreeOfFreedom degreeOfFreedom;
    degreeOfFreedom.type = CRTProtocol::SkeletonStringToDof(subElement);

    // Parse LowerBound and UpperBound (if attributes are missing, leave them unmodified)
    const char* lowerBoundAttr = subElem->Attribute("LowerBound");
    if (lowerBoundAttr)
    {
        ParseString(lowerBoundAttr, degreeOfFreedom.lowerBound);
    }

    const char* upperBoundAttr = subElem->Attribute("UpperBound");
    if (upperBoundAttr)
    {
        ParseString(upperBoundAttr, degreeOfFreedom.upperBound);
    }

    // Parse "Constraint" sub-element
    tinyxml2::XMLElement* constraintElem = subElem->FirstChildElement("Constraint");
    if (constraintElem)
    {
        const char* constraintLowerBound = constraintElem->Attribute("LowerBound");
        if (constraintLowerBound)
        {
            ParseString(constraintLowerBound, degreeOfFreedom.lowerBound);
        }

        const char* constraintUpperBound = constraintElem->Attribute("UpperBound");
        if (constraintUpperBound)
        {
            ParseString(constraintUpperBound, degreeOfFreedom.upperBound);
        }
    }

    // Parse "Couplings" sub-element
    tinyxml2::XMLElement* couplingsElem = subElem->FirstChildElement("Couplings");
    if (couplingsElem)
    {
        for (tinyxml2::XMLElement* couplingElem = couplingsElem->FirstChildElement("Coupling");
            couplingElem != nullptr;
            couplingElem = couplingElem->NextSiblingElement("Coupling"))
        {
            CRTProtocol::SCoupling coupling;
            const char* segmentAttr = couplingElem->Attribute("Segment");
            coupling.segment = segmentAttr ? segmentAttr : "";

            const char* dofAttr = couplingElem->Attribute("DegreeOfFreedom");
            if (dofAttr)
            {
                coupling.degreeOfFreedom = CRTProtocol::SkeletonStringToDof(dofAttr);
            }

            const char* coefficientAttr = couplingElem->Attribute("Coefficient");
            if (coefficientAttr)
            {
                ParseString(coefficientAttr, coupling.coefficient);
            }

            degreeOfFreedom.couplings.push_back(coupling);
        }
    }

    // Parse "Goal" sub-element
    tinyxml2::XMLElement* goalElem = subElem->FirstChildElement("Goal");
    if (goalElem)
    {
        const char* goalValueAttr = goalElem->Attribute("Value");
        if (goalValueAttr)
        {
            ParseString(goalValueAttr, degreeOfFreedom.goalValue);
        }

        const char* goalWeightAttr = goalElem->Attribute("Weight");
        if (goalWeightAttr)
        {
            ParseString(goalWeightAttr, degreeOfFreedom.goalWeight);
        }
    }

    // Add the degree of freedom to the vector
    degreesOfFreedom.push_back(degreeOfFreedom);
    return true;
}

void CRTProtocol::Get3DSettings(EAxis& axisUpwards, std::string& calibrationTime, std::vector<SSettings3DLabel>& labels3D, std::vector<SSettingsBone>& bones)
{
    axisUpwards = ms3DSettings.eAxisUpwards;
    calibrationTime = static_cast<std::string>(ms3DSettings.pCalibrationTime);
    labels3D = ms3DSettings.s3DLabels;
    bones = ms3DSettings.sBones;
}

void CRTProtocol::GetGazeVectorSettings(std::vector<SGazeVector>& gazeVectorSettings)
{
    gazeVectorSettings.clear();
    gazeVectorSettings.resize(mvsGazeVectorSettings.size());

    for (int i = 0; i < mvsGazeVectorSettings.size(); i++)
    {
        gazeVectorSettings.at(i) = mvsGazeVectorSettings.at(i);
    }
}

void CRTProtocol::GetEyeTrackerSettings(std::vector<SEyeTracker>& eyeTrackerSettings)
{
    eyeTrackerSettings.clear();
    eyeTrackerSettings.resize(mvsEyeTrackerSettings.size());

    for (int i = 0; i < mvsEyeTrackerSettings.size(); i++)
    {
        eyeTrackerSettings.at(i) = mvsEyeTrackerSettings.at(i);
    }
}

void CRTProtocol::GetAnalogSettings(std::vector<SAnalogDevice>& analogSettings)
{
    analogSettings.clear();
    analogSettings.resize(mvsAnalogDeviceSettings.size());

    for (int i = 0; i < mvsAnalogDeviceSettings.size(); i++)
    {
        analogSettings.at(i) = mvsAnalogDeviceSettings.at(i);
    }
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
            case VideoResolution1440p:
                oXML.AddElem("Video_Resolution", "1440p");
                break;
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
            case ModeSystemLiveTime:
                oXML.AddElem("Mode", "System live time");
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
        oXML.AddElem("Enabled", body.enabled ? "true" : "false");
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
        for (std::uint32_t i = 0; i < 9; i++)
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

bool CRTProtocol::ParseString(const std::string& str, std::uint32_t& value)
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

bool CRTProtocol::ParseString(const std::string& str, std::int32_t& value)
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
