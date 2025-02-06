#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX



#include <cctype>
#include <thread>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <locale>
#include <vector>
#include <iterator>
#include <functional>

#include "Network.h"
#include "Tinyxml2Deserializer.h"
#include "Tinyxml2Serializer.h"
#include "RTProtocol.h"


#ifdef _WIN32
#include <iphlpapi.h>
// import the internet protocol helper library.
#pragma comment(lib, "IPHLPAPI.lib")
#else
#include <arpa/inet.h>

#endif

using namespace qualisys_cpp_sdk;

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
    return msGeneralSettings.captureFrequency;
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

bool CRTProtocol::Connect(const char* serverAddr, unsigned short port, unsigned short* udpServerPort,
                          int majorVersion, int minorVersion, bool bigEndian, bool negotiateVersion)
{
    CRTPacket::EPacketType type;
    std::string            tempStr;
    std::string            responseStr;

    mbBigEndian = bigEndian;
    mbIsMaster = false;
    mnMajorVersion = 1;
    if ((majorVersion == 1) && (minorVersion == 0))
    {
        mnMinorVersion = 0;
    }
    else
    {
        mnMinorVersion = 1;
        if (mbBigEndian)
        {
            port += 2;
        }
        else
        {
            port += 1;
        }
    }

    if (mpoRTPacket)
    {
        delete mpoRTPacket;
    }

    mpoRTPacket = new CRTPacket(majorVersion, minorVersion, bigEndian);

    if (mpoRTPacket == nullptr)
    {
        strcpy(maErrorStr, "Could not allocate data packet.");
        return false;
    }

    if (mpoNetwork->Connect(serverAddr, port))
    {
        if (udpServerPort != nullptr)
        {
            if (mpoNetwork->CreateUDPSocket(*udpServerPort) == false)
            {
                sprintf(maErrorStr, "CreateUDPSocket failed. %s", mpoNetwork->GetErrorString());
                Disconnect();
                return false;
            }
        }

        // Welcome message
        if (Receive(type, true) == CNetwork::ResponseType::success)
        {
            if (type == CRTPacket::PacketError)
            {
                strcpy(maErrorStr, mpoRTPacket->GetErrorString());
                Disconnect();
                return false;
            }
            else if (type == CRTPacket::PacketCommand)
            {
                const std::string welcomeMessage("QTM RT Interface connected");
                if (strncmp(welcomeMessage.c_str(), mpoRTPacket->GetCommandString(), welcomeMessage.size()) == 0)
                {
                    std::vector<RTVersion> versionList;
                    if (negotiateVersion) 
                    {
                        versionList = RTVersion::VersionList({majorVersion, minorVersion});
                    } 
                    else 
                    {
                        versionList = std::vector<RTVersion>(1, {majorVersion, minorVersion});
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


bool CRTProtocol::SetVersion(int majorVersion, int minorVersion)
{
    std::string responseStr;
    std::string tempStr = "Version " + std::to_string(majorVersion) + "." + std::to_string(minorVersion);

    if (SendCommand(tempStr, responseStr))
    {
        tempStr = "Version set to " + std::to_string(majorVersion) + "." + std::to_string(minorVersion);

        if (responseStr == tempStr)
        {
            mnMajorVersion = majorVersion;
            mnMinorVersion = minorVersion;
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


bool CRTProtocol::GetVersion(unsigned int &majorVersion, unsigned int &minorVersion)
{
    if (!Connected())
    {
        return false;
    }

    majorVersion = mnMajorVersion;
    minorVersion = mnMinorVersion;

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


bool CRTProtocol::GetByteOrder(bool &bigEndian)
{
    std::string responseStr;

    if (SendCommand("ByteOrder", responseStr))
    {
        bigEndian = (responseStr == "Byte order is big endian");
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


bool CRTProtocol::DiscoverRTServer(unsigned short serverPort, bool noLocalResponses, unsigned short discoverPort)
{
    char pData[10];
    SDiscoverResponse sResponse;        

    if (mnBroadcastPort == 0)
    {
        if (!mpoNetwork->CreateUDPSocket(serverPort, true))
        {
            strcpy(maErrorStr, mpoNetwork->GetErrorString());
            return false;
        }
        mnBroadcastPort = serverPort;
    }
    else
    {
        serverPort = mnBroadcastPort;
    }

    *((unsigned int*)pData)         = (unsigned int)10;
    *((unsigned int*)(pData + 4))   = (unsigned int)CRTPacket::PacketDiscover;
    *((unsigned short*)(pData + 8)) = htons(serverPort);

    if (mpoNetwork->SendUDPBroadcast(pData, 10, discoverPort))
    {
        mvsDiscoverResponseList.clear();

        CNetwork::Response response(CNetwork::ResponseType::error, 0);

        do 
        {
            unsigned int addr = 0;
            response = mpoNetwork->ReceiveUdpBroadcast(mDataBuff.data(), (int)mDataBuff.size(), 100000, &addr);

            if (response && response.received > qtmPacketHeaderSize)
            {
                if (CRTPacket::GetType(mDataBuff.data()) == CRTPacket::PacketCommand)
                {
                    char* discoverResponse  = CRTPacket::GetCommandString(mDataBuff.data());
                    sResponse.addr = addr;
                    sResponse.basePort = CRTPacket::GetDiscoverResponseBasePort(mDataBuff.data());

                    if (discoverResponse && (!noLocalResponses || !mpoNetwork->IsLocalAddress(addr)))
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


bool CRTProtocol::GetDiscoverResponse(unsigned int index, unsigned int &addr, unsigned short &basePort, std::string& message)
{
    if (index < mvsDiscoverResponseList.size())
    {
        addr     = mvsDiscoverResponseList[index].addr;
        basePort = mvsDiscoverResponseList[index].basePort;
        message   = mvsDiscoverResponseList[index].message;
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


bool CRTProtocol::GetCurrentFrame(unsigned int componentType, const SComponentOptions& componentOptions)
{
    std::string components;

    if (GetComponentString(components, componentType, componentOptions))
    {
        return GetCurrentFrame(components);
    }
    else
    {
        strcpy(maErrorStr, "DataComponent missing.");
    }
    return false;
}


bool CRTProtocol::StreamFrames(unsigned int componentType)
{
    return StreamFrames(EStreamRate::RateAllFrames, 0, 0, nullptr, componentType);
}

bool CRTProtocol::StreamFrames(EStreamRate rate, unsigned int rateArg, unsigned short udpPort, const char* udpAddr, const char* components)
{
    std::ostringstream commandString;

    if (rate == EStreamRate::RateFrequencyDivisor)
    {
        commandString << "StreamFrames FrequencyDivisor:" << rateArg << " ";
    }
    else if (rate == EStreamRate::RateFrequency)
    {
        commandString << "StreamFrames Frequency:" << rateArg << " ";
    }
    else if (rate == EStreamRate::RateAllFrames)
    {
        commandString << "StreamFrames AllFrames ";
    }
    else
    {
        commandString << "No valid rate.";
        return false;
    }

    if (udpPort > 0)
    {
        if (udpAddr != nullptr && strlen(udpAddr) > 64)
        {
            strcpy(maErrorStr, "UDP address string too long.");
            return false;
        }
        commandString << "UDP";
        if (udpAddr != nullptr)
        {
            commandString << ":" << udpAddr;
        }
        commandString << ":" << udpPort << " ";
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

bool CRTProtocol::StreamFrames(EStreamRate rate, unsigned int rateArg, unsigned short udpPort, const char* udpAddr,
                               unsigned int componentType, const SComponentOptions& componentOptions)
{
    std::string components;

    if (GetComponentString(components, componentType, componentOptions))
    {
        return StreamFrames(rate, rateArg, udpPort, udpAddr, components.c_str());
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


bool CRTProtocol::GetState(CRTPacket::EEvent &event, bool update, int timeout)
{
    CRTPacket::EPacketType type;

    if (update)
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
                response = Receive(type, false, timeout);
                if (response == CNetwork::ResponseType::success)
                {
                    if (mpoRTPacket->GetEvent(event))
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
        event = meState;
        return true;
    }
    return false;
}


bool CRTProtocol::GetCapture(const char* fileName, bool isC3D)
{
    CRTPacket::EPacketType type;
    std::string            responseStr;

    mpFileBuffer = fopen(fileName, "wb");
    if (mpFileBuffer != nullptr)
    {
        if (isC3D)
        {
            // C3D file
            if (SendCommand((mnMajorVersion > 1 || mnMinorVersion > 7) ? "GetCaptureC3D" : "GetCapture", responseStr))
            {
                if (responseStr == "Sending capture")
                {
                    if (Receive(type, true, 5000000) == CNetwork::ResponseType::success) // Wait for C3D file in 5 seconds.
                    {
                        if (type == CRTPacket::PacketC3DFile)
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
                    if (Receive(type, true, 5000000) == CNetwork::ResponseType::success) // Wait for QTM file in 5 seconds.
                    {
                        if (type == CRTPacket::PacketQTMFile)
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


bool CRTProtocol::SaveCapture(const std::string& fileName, bool overwrite, std::string* newFileName, int sizeOfNewFileName)
{
    std::string responseStr;
    std::string tempNewFileNameStr;
    std::string tempStr = "Save ";

    tempStr += fileName;
    tempStr += (overwrite ? " Overwrite" : "");

    if (SendCommand(tempStr, responseStr))
    {
        if (responseStr == "Measurement saved")
        {
            if (newFileName && !newFileName->empty())
            {
                newFileName->clear();
            }
            return true;
        }
        tempNewFileNameStr.resize(responseStr.size());
        if (sscanf(responseStr.c_str(), "Measurement saved as '%[^']'", &tempNewFileNameStr[0]) == 1)
        {
            if (newFileName)
            {
                *newFileName = tempNewFileNameStr;
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


bool CRTProtocol::GetEventString(CRTPacket::EEvent event, char* str)
{
    switch (event)
    {
        case CRTPacket::EventConnected: strcpy(str, "Connected");
            break;
        case CRTPacket::EventConnectionClosed: strcpy(str, "Connection Closed");
            break;
        case CRTPacket::EventCaptureStarted: strcpy(str, "Capture Started");
            break;
        case CRTPacket::EventCaptureStopped: strcpy(str, "Capture Stopped");
            break;
        case CRTPacket::EventCaptureFetchingFinished: strcpy(str, "Fetching Finished");
            break;
        case CRTPacket::EventCalibrationStarted: strcpy(str, "Calibration Started");
            break;
        case CRTPacket::EventCalibrationStopped: strcpy(str, "Calibration Stopped");
            break;
        case CRTPacket::EventRTfromFileStarted: strcpy(str, "RT From File Started");
            break;
        case CRTPacket::EventRTfromFileStopped: strcpy(str, "RT From File Stopped");
            break;
        case CRTPacket::EventWaitingForTrigger: strcpy(str, "Waiting For Trigger");
            break;
        case CRTPacket::EventCameraSettingsChanged: strcpy(str, "Camera Settings Changed");
            break;
        case CRTPacket::EventQTMShuttingDown: strcpy(str, "QTM Shutting Down");
            break;
        case CRTPacket::EventCaptureSaved: strcpy(str, "Capture Saved");
            break;
        case CRTPacket::EventReprocessingStarted: strcpy(str, "Reprocessing Started");
            break;
        case CRTPacket::EventReprocessingStopped: strcpy(str, "Reprocessing Stopped");
            break;
        case CRTPacket::EventTrigger: strcpy(str, "Trigger");
            break;
        default:
            return false;
    }
    return true;
}


bool CRTProtocol::ConvertRateString(const char* rateText, EStreamRate &rate, unsigned int &rateArg)
{
    std::string rateString;

    rateString.assign(rateText);
    rateString = ToLower(rateString);

    rate = EStreamRate::RateNone;

    if (rateString.compare(0, 9, "allframes", 9) == 0)
    {
        rate = EStreamRate::RateAllFrames;
    }
    else if (rateString.compare(0, 10, "frequency:") == 0)
    {
        rateArg = atoi(rateString.substr(10).c_str());
        if (rateArg > 0)
        {
            rate = EStreamRate::RateFrequency;
        }
    }
    else if (rateString.compare(0, 17, "frequencydivisor:") == 0)
    {
        rateArg = atoi(rateString.substr(17).c_str());
        if (rateArg > 0)
        {
            rate = EStreamRate::RateFrequencyDivisor;
        }
    }

    return rate != EStreamRate::RateNone;
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


bool CRTProtocol::GetComponentString(std::string& componentStr, unsigned int componentType, const SComponentOptions& options)
{
    std::string tempStr = "";

    if (componentType & cComponent2d)
    {
        tempStr += "2D ";
    }
    if (componentType & cComponent2dLin)
    {
        tempStr += "2DLin ";
    }
    if (componentType & cComponent3d)
    {
        tempStr += "3D ";
    }
    if (componentType & cComponent3dRes)
    {
        tempStr += "3DRes ";
    }
    if (componentType & cComponent3dNoLabels)
    {
        tempStr += "3DNoLabels ";
    }
    if (componentType & cComponent3dNoLabelsRes)
    {
        tempStr += "3DNoLabelsRes ";
    }
    if (componentType & cComponent6d)
    {
        tempStr += "6D ";
    }
    if (componentType & cComponent6dRes)
    {
        tempStr += "6DRes ";
    }
    if (componentType & cComponent6dEuler)
    {
        tempStr += "6DEuler ";
    }
    if (componentType & cComponent6dEulerRes)
    {
        tempStr += "6DEulerRes ";
    }
    if (componentType & cComponentAnalog)
    {
        tempStr += "Analog";

        if (options.mAnalogChannels != nullptr)
        {
            tempStr += ":";
            tempStr += options.mAnalogChannels;
        }

        tempStr += " ";
    }
    if (componentType & cComponentAnalogSingle)
    {
        tempStr += "AnalogSingle";

        if (options.mAnalogChannels != nullptr)
        {
            tempStr += ":";
            tempStr += options.mAnalogChannels;
        }

        tempStr += " ";
    }
    if (componentType & cComponentForce)
    {
        tempStr += "Force ";
    }
    if (componentType & cComponentForceSingle)
    {
        tempStr += "ForceSingle ";
    }
    if (componentType & cComponentGazeVector)
    {
        tempStr += "GazeVector ";
    }
    if (componentType & cComponentEyeTracker)
    {
        tempStr += "EyeTracker ";
    }
    if (componentType & cComponentImage)
    {
        tempStr += "Image ";
    }
    if (componentType & cComponentTimecode)
    {
        tempStr += "Timecode ";
    }
    if (componentType & cComponentSkeleton)
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


int CRTProtocol::ReceiveRTPacket(CRTPacket::EPacketType &type, bool bSkipEvents, int timeout)
{
    int returnVal = -1;
    auto response = Receive(type, bSkipEvents, timeout);
    
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


CNetwork::ResponseType CRTProtocol::Receive(CRTPacket::EPacketType &type, bool bSkipEvents, int timeout)
{
    CNetwork::Response response(CNetwork::ResponseType::error, 0);
    unsigned int nRecvedTotal = 0;
    unsigned int nFrameSize;

    type = CRTPacket::PacketNone;

    do 
    {
        nRecvedTotal = 0;

        response = mpoNetwork->Receive(mDataBuff.data(), (int)mDataBuff.size(), true, timeout);

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

        bool bigEndian = (mbBigEndian || (mnMajorVersion == 1 && mnMinorVersion == 0));
        nFrameSize = mpoRTPacket->GetSize(mDataBuff.data(), bigEndian);
        type      = mpoRTPacket->GetType(mDataBuff.data(), bigEndian);
        
        unsigned int nReadSize;

        if (type == CRTPacket::PacketC3DFile || type == CRTPacket::PacketQTMFile)
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
    } while (bSkipEvents && type == CRTPacket::PacketEvent);
    
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
    CRTPacket::EPacketType type;

    mvsAnalogDeviceSettings.clear();
    auto sendStr = std::string("GetParameters ") + settingsType;
    if (!SendCommand(sendStr.c_str()))
    {
        sprintf(maErrorStr, "GetParameters %s failed", settingsType.c_str());
        return nullptr;
    }

retry:
    auto received = Receive(type, true);

    if (received == CNetwork::ResponseType::timeout)
    {
        strcat(maErrorStr, " Expected XML packet.");
        return nullptr;
    }
    if (received == CNetwork::ResponseType::error)
    {
        return nullptr;
    }

    if (type != CRTPacket::PacketXML)
    {
        if (type == CRTPacket::PacketError)
        {
            sprintf(maErrorStr, "%s.", mpoRTPacket->GetErrorString());
            return nullptr;
        }
        else
        {
            goto retry;
            //sprintf(maErrorStr, "GetParameters %s returned wrong packet type. Got type %d expected type 2.", settingsType.c_str(), type);
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

    msGeneralSettings.cameras.clear();

    const char* data = ReadSettings("General");

    if (!data)
    {
        return false;
    }

    CTinyxml2Deserializer serializer(data, mnMajorVersion, mnMinorVersion);

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

bool CRTProtocol::Read3DSettings(bool& dataAvailable)
{
    dataAvailable = false;
    ms3DSettings.labels3D.clear();
    ms3DSettings.calibrationTime[0] = 0;

    const char* data = ReadSettings("3D");
    if (!data)
    {
        return false;
    }

    CTinyxml2Deserializer deserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.Deserialize3DSettings(ms3DSettings, dataAvailable);
}

bool CRTProtocol::Read6DOFSettings(bool& dataAvailable)
{
    m6DOFSettings.clear();

    const auto* data = ReadSettings("6D");
    if(!data)
    {
        return false;
    }

    CTinyxml2Deserializer deserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.Deserialize6DOFSettings(m6DOFSettings, msGeneralSettings, dataAvailable);
}

bool CRTProtocol::ReadGazeVectorSettings(bool& dataAvailable)
{
    dataAvailable = false;

    mvsGazeVectorSettings.clear();

    const auto* data = ReadSettings("GazeVector");
    if(!data)
    {
        return false;
    }

    CTinyxml2Deserializer deserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.DeserializeGazeVectorSettings(mvsGazeVectorSettings, dataAvailable);
}

bool CRTProtocol::ReadEyeTrackerSettings(bool& dataAvailable)
{
    dataAvailable = false;

    mvsEyeTrackerSettings.clear();

    const auto* data = ReadSettings("EyeTracker");
    if(!data)
    {
        return false;
    }

    CTinyxml2Deserializer deserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.DeserializeEyeTrackerSettings(mvsEyeTrackerSettings, dataAvailable);
}

bool CRTProtocol::ReadAnalogSettings(bool& dataAvailable)
{
    dataAvailable = false;

    mvsAnalogDeviceSettings.clear();

    const auto* data = ReadSettings("Analog");
    if(!data)
    {
        return false;
    }
    CTinyxml2Deserializer deserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.DeserializeAnalogSettings(mvsAnalogDeviceSettings, dataAvailable);
}

bool CRTProtocol::ReadForceSettings(bool& dataAvailable)
{
    dataAvailable = false;

    msForceSettings.forcePlates.clear();

    const auto* data = ReadSettings("Force");
    if(!data)
    {
        return false;
    }

    CTinyxml2Deserializer deserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.DeserializeForceSettings(msForceSettings, dataAvailable);
}

bool CRTProtocol::ReadImageSettings(bool& dataAvailable)
{
    dataAvailable = false;

    mvsImageSettings.clear();

    const auto* data = ReadSettings("Image");
    if(!data)
    {
        return false;
    }

    CTinyxml2Deserializer deserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.DeserializeImageSettings(mvsImageSettings, dataAvailable);
}

bool CRTProtocol::ReadSkeletonSettings(bool& dataAvailable, bool skeletonGlobalData)
{
    dataAvailable = false;

    mSkeletonSettings.clear();
    mSkeletonSettingsHierarchical.clear();

    const auto* data = ReadSettings(skeletonGlobalData ? "Skeleton:global" : "Skeleton");
    if (!data)
    {
        return false;
    }

    CTinyxml2Deserializer deserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.DeserializeSkeletonSettings(skeletonGlobalData, mSkeletonSettingsHierarchical, mSkeletonSettings, dataAvailable);
}

bool CRTProtocol::ReceiveCalibrationSettings(int timeout)
{
    CRTPacket::EPacketType  type;
    std::string             tStr;
    SCalibration            settings;
    CNetwork::ResponseType  response;
    CRTPacket::EEvent       event = CRTPacket::EventNone;

    do 
    {
        response = Receive(type, false, timeout);

        if (response == CNetwork::ResponseType::timeout)
        {
            strcat(maErrorStr, " Expected XML packet.");
            return false;
        }
        if (response == CNetwork::ResponseType::error)
        {
            return false;
        }

        if (type == CRTPacket::PacketEvent)
        {
            mpoRTPacket->GetEvent(event);
        }
        else
        {
            event = CRTPacket::EventNone;
        }
    } while (event != CRTPacket::EventNone && event != CRTPacket::EventConnectionClosed);

    if (type != CRTPacket::PacketXML)
    {
        if (event != CRTPacket::EventNone)
        {
            sprintf(maErrorStr, "Calibration aborted.");
        }
        else if (type == CRTPacket::PacketError)
        {
            sprintf(maErrorStr, "%s.", mpoRTPacket->GetErrorString());
        }
        else
        {
            sprintf(maErrorStr, "GetParameters Calibration returned wrong packet type. Got type %d expected type 2.", type);
        }
        return false;
    }

    auto data = mpoRTPacket->GetXMLString();
    CTinyxml2Deserializer deserializer(data, mnMajorVersion, mnMinorVersion);
    return deserializer.DeserializeCalibrationSettings(mCalibrationSettings);
} // ReadCalibrationSettings

void CRTProtocol::Get3DSettings(EAxis& axisUpwards, std::string& calibrationTime, std::vector<SSettings3DLabel>& labels3D, std::vector<SSettingsBone>& bones)
{
    axisUpwards = ms3DSettings.axisUpwards;
    calibrationTime = static_cast<std::string>(ms3DSettings.calibrationTime);

    labels3D = ms3DSettings.labels3D;
    bones = ms3DSettings.bones;
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
    unsigned int       &captureFrequency, float &captureTime,
    bool& startOnExtTrig, bool& startOnTrigNO, bool& startOnTrigNC, bool& startOnTrigSoftware,
    EProcessingActions &processingActions, EProcessingActions &rtProcessingActions, EProcessingActions &reprocessingActions) const
{
    captureFrequency = msGeneralSettings.captureFrequency;
    captureTime = msGeneralSettings.captureTime;
    startOnExtTrig = msGeneralSettings.startOnExternalTrigger;
    startOnTrigNO = msGeneralSettings.startOnTrigNO;
    startOnTrigNC = msGeneralSettings.startOnTrigNC;
    startOnTrigSoftware = msGeneralSettings.startOnTrigSoftware;
    processingActions = msGeneralSettings.processingActions;
    rtProcessingActions = msGeneralSettings.rtProcessingActions;
    reprocessingActions = msGeneralSettings.reprocessingActions;
}


void CRTProtocol::GetSystemSettings(
    unsigned int       &captureFrequency, float &captureTime,
    bool& startOnExtTrig, bool& startOnTrigNO, bool& startOnTrigNC, bool& startOnTrigSoftware,
    EProcessingActions &processingActions, EProcessingActions &rtProcessingActions, EProcessingActions &reprocessingActions) const
{
    GetGeneralSettings(captureFrequency, captureTime, startOnExtTrig, startOnTrigNO, startOnTrigNC, startOnTrigSoftware, processingActions, rtProcessingActions, reprocessingActions);
}


void CRTProtocol::GetCalibrationSettings(SCalibration &calibrationSettings) const
{
    calibrationSettings = mCalibrationSettings;
}


// External time base settings only available in version 1.10 of the rt protocol and later
void CRTProtocol::GetExtTimeBaseSettings(
    bool &enabled,                    ESignalSource &signalSource,
    bool &signalModePeriodic,         unsigned int &freqMultiplier,
    unsigned int &freqDivisor,        unsigned int &freqTolerance,
    float &nominalFrequency,          bool &negativeEdge,
    unsigned int &signalShutterDelay, float &nonPeriodicTimeout) const
{
    enabled            = msGeneralSettings.externalTimebase.enabled;
    signalSource       = msGeneralSettings.externalTimebase.signalSource;
    signalModePeriodic = msGeneralSettings.externalTimebase.signalModePeriodic;
    freqMultiplier     = msGeneralSettings.externalTimebase.freqMultiplier;
    freqDivisor        = msGeneralSettings.externalTimebase.freqDivisor;
    freqTolerance      = msGeneralSettings.externalTimebase.freqTolerance;
    nominalFrequency   = msGeneralSettings.externalTimebase.nominalFrequency;
    negativeEdge       = msGeneralSettings.externalTimebase.negativeEdge;
    signalShutterDelay = msGeneralSettings.externalTimebase.signalShutterDelay;
    nonPeriodicTimeout = msGeneralSettings.externalTimebase.nonPeriodicTimeout;
}

void CRTProtocol::GetExtTimestampSettings(SSettingsGeneralExternalTimestamp& timestamp) const
{
    timestamp = msGeneralSettings.timestamp;
}

void CRTProtocol::GetEulerAngles(std::string& first, std::string& second, std::string& third) const
{
    first = msGeneralSettings.eulerRotations[0];
    second = msGeneralSettings.eulerRotations[1];
    third = msGeneralSettings.eulerRotations[2];
}

unsigned int CRTProtocol::GetCameraCount() const
{
    return (unsigned int)msGeneralSettings.cameras.size();
}

std::vector<CRTProtocol::SSettingsGeneralCamera> CRTProtocol::GetDevices() const
{
    return msGeneralSettings.cameras;
}


bool CRTProtocol::GetCameraSettings(
    unsigned int cameraIndex, unsigned int &id,     ECameraModel &model,
    bool         &underwater, bool &supportsHwSync, unsigned int &serial, ECameraMode  &mode) const
{
    if (cameraIndex < msGeneralSettings.cameras.size())
    {
        id             = msGeneralSettings.cameras[cameraIndex].id;
        model          = msGeneralSettings.cameras[cameraIndex].model;
        underwater     = msGeneralSettings.cameras[cameraIndex].underwater;
        supportsHwSync = msGeneralSettings.cameras[cameraIndex].supportsHwSync;
        serial         = msGeneralSettings.cameras[cameraIndex].serial;
        mode           = msGeneralSettings.cameras[cameraIndex].mode;
        return true;
    }
    return false;
}


bool CRTProtocol::GetCameraMarkerSettings(
    unsigned int cameraIndex,   unsigned int &currentExposure, unsigned int &minExposure,
    unsigned int &maxExposure,  unsigned int &currentThreshold,
    unsigned int &minThreshold, unsigned int &maxThreshold) const
{
    if (cameraIndex < msGeneralSettings.cameras.size())
    {
        currentExposure  = msGeneralSettings.cameras[cameraIndex].markerExposure;
        minExposure      = msGeneralSettings.cameras[cameraIndex].markerExposureMin;
        maxExposure      = msGeneralSettings.cameras[cameraIndex].markerExposureMax;
        currentThreshold = msGeneralSettings.cameras[cameraIndex].markerThreshold;
        minThreshold     = msGeneralSettings.cameras[cameraIndex].markerThresholdMin;
        maxThreshold     = msGeneralSettings.cameras[cameraIndex].markerThresholdMax;
        return true;
    }
    return false;
}


bool CRTProtocol::GetCameraVideoSettings(
    unsigned int cameraIndex,            EVideoResolution &videoResolution,
    EVideoAspectRatio &videoAspectRatio, unsigned int &videoFrequency,
    unsigned int &currentExposure,       unsigned int &minExposure,
    unsigned int &maxExposure,           unsigned int &currentFlashTime,
    unsigned int &minFlashTime,          unsigned int &maxFlashTime) const
{
    if (cameraIndex < msGeneralSettings.cameras.size())
    {
        videoResolution   = msGeneralSettings.cameras[cameraIndex].videoResolution;
        videoAspectRatio  = msGeneralSettings.cameras[cameraIndex].videoAspectRatio;
        videoFrequency   = msGeneralSettings.cameras[cameraIndex].videoFrequency;
        currentExposure  = msGeneralSettings.cameras[cameraIndex].videoExposure;
        minExposure      = msGeneralSettings.cameras[cameraIndex].videoExposureMin;
        maxExposure      = msGeneralSettings.cameras[cameraIndex].videoExposureMax;
        currentFlashTime = msGeneralSettings.cameras[cameraIndex].videoFlashTime;
        minFlashTime     = msGeneralSettings.cameras[cameraIndex].videoFlashTimeMin;
        maxFlashTime     = msGeneralSettings.cameras[cameraIndex].videoFlashTimeMax;
        return true;
    }
    return false;
}


bool CRTProtocol::GetCameraSyncOutSettings(
    unsigned int cameraIndex, unsigned int portNumber, ESyncOutFreqMode &syncOutMode,
    unsigned int &syncOutValue, float      &syncOutDutyCycle,
    bool         &syncOutNegativePolarity) const
{
    if (cameraIndex < msGeneralSettings.cameras.size())
    {
        if (portNumber == 1 || portNumber == 2)
        {
            syncOutMode = msGeneralSettings.cameras[cameraIndex].syncOutMode[portNumber - 1];
            syncOutValue = msGeneralSettings.cameras[cameraIndex].syncOutValue[portNumber - 1];
            syncOutDutyCycle = msGeneralSettings.cameras[cameraIndex].syncOutDutyCycle[portNumber - 1];
        }
        if (portNumber > 0 && portNumber < 4)
        {
            syncOutNegativePolarity = msGeneralSettings.cameras[cameraIndex].syncOutNegativePolarity[portNumber - 1];
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
    unsigned int cameraIndex, SPoint &point, float rotationMatrix[3][3]) const
{
    if (cameraIndex < msGeneralSettings.cameras.size())
    {
        point.x = msGeneralSettings.cameras[cameraIndex].positionX;
        point.y = msGeneralSettings.cameras[cameraIndex].positionY;
        point.z = msGeneralSettings.cameras[cameraIndex].positionZ;
        memcpy(rotationMatrix, msGeneralSettings.cameras[cameraIndex].positionRotMatrix, 9 * sizeof(float));
        return true;
    }
    return false;
}


bool CRTProtocol::GetCameraOrientation(
    unsigned int cameraIndex, int &orientation) const
{
    if (cameraIndex < msGeneralSettings.cameras.size())
    {
        orientation = msGeneralSettings.cameras[cameraIndex].orientation;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraResolution(
    unsigned int cameraIndex, unsigned int &markerWidth, unsigned int &markerHeight,
    unsigned int &videoWidth, unsigned int &videoHeight) const
{
    if (cameraIndex < msGeneralSettings.cameras.size())
    {
        markerWidth  = msGeneralSettings.cameras[cameraIndex].markerResolutionWidth;
        markerHeight = msGeneralSettings.cameras[cameraIndex].markerResolutionHeight;
        videoWidth   = msGeneralSettings.cameras[cameraIndex].videoResolutionWidth;
        videoHeight  = msGeneralSettings.cameras[cameraIndex].videoResolutionHeight;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraFOV(
    unsigned int cameraIndex,  unsigned int &markerLeft,  unsigned int &markerTop,
    unsigned int &markerRight, unsigned int &markerBottom,
    unsigned int &videoLeft,   unsigned int &videoTop,
    unsigned int &videoRight,  unsigned int &videoBottom) const
{
    if (cameraIndex < msGeneralSettings.cameras.size())
    {
        markerLeft   = msGeneralSettings.cameras[cameraIndex].markerFOVLeft;
        markerTop    = msGeneralSettings.cameras[cameraIndex].markerFOVTop;
        markerRight  = msGeneralSettings.cameras[cameraIndex].markerFOVRight;
        markerBottom = msGeneralSettings.cameras[cameraIndex].markerFOVBottom;
        videoLeft    = msGeneralSettings.cameras[cameraIndex].videoFOVLeft;
        videoTop     = msGeneralSettings.cameras[cameraIndex].videoFOVTop;
        videoRight   = msGeneralSettings.cameras[cameraIndex].videoFOVRight;
        videoBottom  = msGeneralSettings.cameras[cameraIndex].videoFOVBottom;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraLensControlSettings(const unsigned int cameraIndex, float* focus, float* aperture) const
{
    if (cameraIndex < msGeneralSettings.cameras.size())
    {
        *focus = msGeneralSettings.cameras[cameraIndex].focus;
        if (std::isnan(*focus))
            return false;
        *aperture = msGeneralSettings.cameras[cameraIndex].aperture;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraAutoExposureSettings(const unsigned int cameraIndex, bool* autoExposureEnabled, float* autoExposureCompensation) const
{
    if (cameraIndex < msGeneralSettings.cameras.size())
    {
        *autoExposureCompensation = msGeneralSettings.cameras[cameraIndex].autoExposureCompensation;
        if (std::isnan(*autoExposureCompensation))
            return false;
        *autoExposureEnabled = msGeneralSettings.cameras[cameraIndex].autoExposureEnabled;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraAutoWhiteBalance(const unsigned int cameraIndex, bool* autoWhiteBalanceEnabled) const
{
    if (cameraIndex < msGeneralSettings.cameras.size() && msGeneralSettings.cameras[cameraIndex].autoWhiteBalance >= 0)
    {
        *autoWhiteBalanceEnabled = msGeneralSettings.cameras[cameraIndex].autoWhiteBalance == 1;
        return true;
    }
    return false;
}

CRTProtocol::EAxis CRTProtocol::Get3DUpwardAxis() const
{
    return ms3DSettings.axisUpwards;
}

const char* CRTProtocol::Get3DCalibrated() const
{
    return ms3DSettings.calibrationTime;
}

unsigned int CRTProtocol::Get3DLabeledMarkerCount() const
{
    return (unsigned int)ms3DSettings.labels3D.size();
}

const char* CRTProtocol::Get3DLabelName(unsigned int markerIndex) const
{
    if (markerIndex < ms3DSettings.labels3D.size())
    {
        return ms3DSettings.labels3D[markerIndex].name.c_str();
    }
    return nullptr;
}

unsigned int CRTProtocol::Get3DLabelColor(unsigned int markerIndex) const
{
    if (markerIndex < ms3DSettings.labels3D.size())
    {
        return ms3DSettings.labels3D[markerIndex].rgbColor;
    }
    return 0;
}

const char* CRTProtocol::Get3DTrajectoryType(unsigned int markerIndex) const
{
    if (markerIndex < ms3DSettings.labels3D.size())
    {
        return ms3DSettings.labels3D[markerIndex].type.c_str();
    }
    return 0;
}


unsigned int CRTProtocol::Get3DBoneCount() const
{
    return (unsigned int)ms3DSettings.bones.size();
}

const char* CRTProtocol::Get3DBoneFromName(unsigned int boneIndex) const
{
    if (boneIndex < ms3DSettings.bones.size())
    {
        return ms3DSettings.bones[boneIndex].fromName.c_str();
    }
    return nullptr;
}

const char* CRTProtocol::Get3DBoneToName(unsigned int boneIndex) const
{
    if (boneIndex < ms3DSettings.bones.size())
    {
        return ms3DSettings.bones[boneIndex].toName.c_str();
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


const char* CRTProtocol::Get6DOFBodyName(unsigned int bodyIndex) const
{
    if (bodyIndex < m6DOFSettings.size())
    {
        return m6DOFSettings[bodyIndex].name.c_str();
    }
    return nullptr;
}


unsigned int CRTProtocol::Get6DOFBodyColor(unsigned int bodyIndex) const
{
    if (bodyIndex < m6DOFSettings.size())
    {
        return m6DOFSettings[bodyIndex].color;
    }
    return 0;
}


unsigned int CRTProtocol::Get6DOFBodyPointCount(unsigned int bodyIndex) const
{
    if (bodyIndex < m6DOFSettings.size())
    {
        return (unsigned int)m6DOFSettings.at(bodyIndex).points.size();
    }
    return 0;
}


bool CRTProtocol::Get6DOFBodyPoint(unsigned int bodyIndex, unsigned int markerIndex, SPoint &point) const
{
    if (bodyIndex < m6DOFSettings.size())
    {
        if (markerIndex < m6DOFSettings.at(bodyIndex).points.size())
        {
            point.x = m6DOFSettings.at(bodyIndex).points[markerIndex].x;
            point.y = m6DOFSettings.at(bodyIndex).points[markerIndex].y;
            point.z = m6DOFSettings.at(bodyIndex).points[markerIndex].z;
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

const char* CRTProtocol::GetGazeVectorName(unsigned int gazeVectorIndex) const
{
    if (gazeVectorIndex < mvsGazeVectorSettings.size())
    {
        return mvsGazeVectorSettings[gazeVectorIndex].name.c_str();
    }
    return nullptr;
}

float CRTProtocol::GetGazeVectorFrequency(unsigned int gazeVectorIndex) const
{
    if (gazeVectorIndex < mvsGazeVectorSettings.size())
    {
        return mvsGazeVectorSettings[gazeVectorIndex].frequency;
    }
    return 0;
}

bool CRTProtocol::GetGazeVectorHardwareSyncUsed(unsigned int gazeVectorIndex) const
{
    if (gazeVectorIndex < mvsGazeVectorSettings.size())
    {
        return mvsGazeVectorSettings[gazeVectorIndex].hwSync;
    }
    return false;
}

bool CRTProtocol::GetGazeVectorFilterUsed(unsigned int gazeVectorIndex) const
{
    if (gazeVectorIndex < mvsGazeVectorSettings.size())
    {
        return mvsGazeVectorSettings[gazeVectorIndex].filter;
    }
    return false;
}


unsigned int CRTProtocol::GetEyeTrackerCount() const
{
    return (unsigned int)mvsEyeTrackerSettings.size();
}

const char* CRTProtocol::GetEyeTrackerName(unsigned int eyeTrackerIndex) const
{
    if (eyeTrackerIndex < mvsEyeTrackerSettings.size())
    {
        return mvsEyeTrackerSettings[eyeTrackerIndex].name.c_str();
    }
    return nullptr;
}

float CRTProtocol::GetEyeTrackerFrequency(unsigned int eyeTrackerIndex) const
{
    if (eyeTrackerIndex < mvsEyeTrackerSettings.size())
    {
        return mvsEyeTrackerSettings[eyeTrackerIndex].frequency;
    }
    return 0;
}

bool CRTProtocol::GetEyeTrackerHardwareSyncUsed(unsigned int eyeTrackerIndex) const
{
    if (eyeTrackerIndex < mvsEyeTrackerSettings.size())
    {
        return mvsEyeTrackerSettings[eyeTrackerIndex].hwSync;
    }
    return false;
}


unsigned int CRTProtocol::GetAnalogDeviceCount() const
{
    return (unsigned int)mvsAnalogDeviceSettings.size();
}


bool CRTProtocol::GetAnalogDevice(unsigned int deviceIndex, unsigned int &deviceID, unsigned int &channels,
                                  char* &name, unsigned int &frequency, char* &unit,
                                  float &fMinRange, float &fMaxRange) const
{
    if (deviceIndex < mvsAnalogDeviceSettings.size())
    {
        deviceID  = mvsAnalogDeviceSettings.at(deviceIndex).deviceID;
        name      = (char*)mvsAnalogDeviceSettings.at(deviceIndex).name.c_str();
        channels  = mvsAnalogDeviceSettings.at(deviceIndex).channels;
        frequency = mvsAnalogDeviceSettings.at(deviceIndex).frequency;
        unit      = (char*)mvsAnalogDeviceSettings.at(deviceIndex).unit.c_str();
        fMinRange  = mvsAnalogDeviceSettings.at(deviceIndex).minRange;
        fMaxRange  = mvsAnalogDeviceSettings.at(deviceIndex).maxRange;

        return true;
    }
    return false;
}


const char* CRTProtocol::GetAnalogLabel(unsigned int deviceIndex, unsigned int nChannelIndex) const
{
    if (deviceIndex < mvsAnalogDeviceSettings.size())
    {
        if (nChannelIndex < mvsAnalogDeviceSettings.at(deviceIndex).labels.size())
        {
            return mvsAnalogDeviceSettings.at(deviceIndex).labels.at(nChannelIndex).c_str();
        }
    }
    return nullptr;
}


const char* CRTProtocol::GetAnalogUnit(unsigned int deviceIndex, unsigned int nChannelIndex) const
{
    if (deviceIndex < mvsAnalogDeviceSettings.size())
    {
        if (nChannelIndex < mvsAnalogDeviceSettings.at(deviceIndex).units.size())
        {
            return mvsAnalogDeviceSettings.at(deviceIndex).units.at(nChannelIndex).c_str();
        }
    }
    return nullptr;
}


void CRTProtocol::GetForceUnits(char* &pLength, char* &pForce) const
{
    pLength = (char*)msForceSettings.unitLength.c_str();
    pForce  = (char*)msForceSettings.unitForce.c_str();
}


unsigned int CRTProtocol::GetForcePlateCount() const
{
    return (unsigned int)msForceSettings.forcePlates.size();
}


bool CRTProtocol::GetForcePlate(unsigned int nPlateIndex, unsigned int &id, unsigned int &nAnalogDeviceID,
                                unsigned int &frequency, char* &pType, char* &name, float &fLength, float &fWidth) const
{
    if (nPlateIndex < msForceSettings.forcePlates.size())
    {
        id             = msForceSettings.forcePlates[nPlateIndex].id;
        nAnalogDeviceID = msForceSettings.forcePlates[nPlateIndex].analogDeviceID;
        frequency      = msForceSettings.forcePlates[nPlateIndex].frequency;
        pType           = (char*)msForceSettings.forcePlates[nPlateIndex].type.c_str();
        name           = (char*)msForceSettings.forcePlates[nPlateIndex].name.c_str();
        fLength         = msForceSettings.forcePlates[nPlateIndex].length;
        fWidth          = msForceSettings.forcePlates[nPlateIndex].width;
        return true;
    }
    return false;
}


bool CRTProtocol::GetForcePlateLocation(unsigned int nPlateIndex, SPoint sCorner[4]) const
{
    if (nPlateIndex < msForceSettings.forcePlates.size())
    {
        memcpy(sCorner, msForceSettings.forcePlates[nPlateIndex].corner, 3 * 4 * sizeof(float));
        return true;
    }
    return false;
}


bool CRTProtocol::GetForcePlateOrigin(unsigned int nPlateIndex, SPoint &sOrigin) const
{
    if (nPlateIndex < msForceSettings.forcePlates.size())
    {
        sOrigin = msForceSettings.forcePlates[nPlateIndex].origin;
        return true;
    }
    return false;
}


unsigned int CRTProtocol::GetForcePlateChannelCount(unsigned int nPlateIndex) const
{
    if (nPlateIndex < msForceSettings.forcePlates.size())
    {
        return (unsigned int)msForceSettings.forcePlates[nPlateIndex].channels.size();
    }
    return 0;
}


bool CRTProtocol::GetForcePlateChannel(unsigned int nPlateIndex, unsigned int nChannelIndex,
                                       unsigned int &nChannelNumber, float &fConversionFactor) const
{
    if (nPlateIndex < msForceSettings.forcePlates.size())
    {
        if (nChannelIndex < msForceSettings.forcePlates[nPlateIndex].channels.size())
        {
            nChannelNumber    = msForceSettings.forcePlates[nPlateIndex].channels[nChannelIndex].channelNumber;
            fConversionFactor = msForceSettings.forcePlates[nPlateIndex].channels[nChannelIndex].conversionFactor;
            return true;
        }
    }
    return false;
}


bool CRTProtocol::GetForcePlateCalibrationMatrix(unsigned int nPlateIndex, float fvCalMatrix[12][12], unsigned int* rows, unsigned int* columns) const
{
    if (nPlateIndex < msForceSettings.forcePlates.size())
    {
        if (msForceSettings.forcePlates[nPlateIndex].validCalibrationMatrix)
        {
            *rows = msForceSettings.forcePlates[nPlateIndex].calibrationMatrixRows;
            *columns = msForceSettings.forcePlates[nPlateIndex].calibrationMatrixColumns;
            memcpy(
                fvCalMatrix,
                msForceSettings.forcePlates[nPlateIndex].calibrationMatrix,
                msForceSettings.forcePlates[nPlateIndex].calibrationMatrixRows * msForceSettings.forcePlates[nPlateIndex].calibrationMatrixColumns * sizeof(float));
            return true;
        }
    }
    return false;
}


unsigned int CRTProtocol::GetImageCameraCount() const
{
    return (unsigned int)mvsImageSettings.size();
}


bool CRTProtocol::GetImageCamera(unsigned int cameraIndex, unsigned int &nCameraID, bool &enabled,
                                 CRTPacket::EImageFormat &eFormat, unsigned int &nWidth, unsigned int &nHeight,
                                 float &fCropLeft, float &fCropTop, float &fCropRight, float &fCropBottom) const
{
    if (cameraIndex < mvsImageSettings.size())
    {
        nCameraID   = mvsImageSettings[cameraIndex].id;
        enabled    = mvsImageSettings[cameraIndex].enabled;
        eFormat     = mvsImageSettings[cameraIndex].format;
        nWidth      = mvsImageSettings[cameraIndex].width;
        nHeight     = mvsImageSettings[cameraIndex].height;
        fCropLeft   = mvsImageSettings[cameraIndex].cropLeft;
        fCropTop    = mvsImageSettings[cameraIndex].cropTop;
        fCropRight  = mvsImageSettings[cameraIndex].cropRight;
        fCropBottom = mvsImageSettings[cameraIndex].cropBottom;
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
    CTinyxml2Serializer serializer(mnMajorVersion, mnMinorVersion);
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
    CTinyxml2Serializer serializer(mnMajorVersion, mnMinorVersion);
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
    CTinyxml2Serializer serializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetExtTimestampSettings(timestampSettings);

    return SendXML(message.data());
}


// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraSettings(
    const unsigned int nCameraID,        const ECameraMode* peMode,
    const float*       pfMarkerExposure, const float*       pfMarkerThreshold,
    const int*         pnOrientation)
{
    CTinyxml2Serializer serializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetCameraSettings(
        nCameraID, peMode,
        pfMarkerExposure, pfMarkerThreshold,
        pnOrientation);

    return SendXML(message.data());
} // SetGeneralCamera


// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraVideoSettings(
    const unsigned int nCameraID,                const EVideoResolution* videoResolution,
    const EVideoAspectRatio* videoAspectRatio, const unsigned int* pnVideoFrequency,
    const float* pfVideoExposure,                const float* pfVideoFlashTime)
{
    CTinyxml2Serializer serializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetCameraVideoSettings(
        nCameraID, videoResolution,
        videoAspectRatio, pnVideoFrequency,
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
    CTinyxml2Serializer serializer(mnMajorVersion, mnMinorVersion);
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
    CTinyxml2Serializer serializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetCameraLensControlSettings(nCameraID, focus, aperture);
    return SendXML(message.data());

}

// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraAutoExposureSettings(const unsigned int nCameraID, const bool autoExposure, const float compensation)
{
    CTinyxml2Serializer serializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetCameraAutoExposureSettings(nCameraID, autoExposure, compensation);
    return SendXML(message.data());
}

// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraAutoWhiteBalance(const unsigned int nCameraID, const bool enable)
{
    CTinyxml2Serializer serializer(mnMajorVersion, mnMinorVersion);
    auto message = serializer.SetCameraAutoWhiteBalance(nCameraID, enable);
    return SendXML(message.data());
}


bool CRTProtocol::SetImageSettings(
    const unsigned int  nCameraID, const bool*         pbEnable,    const CRTPacket::EImageFormat* peFormat,
    const unsigned int* pnWidth,   const unsigned int* pnHeight,    const float* pfLeftCrop,
    const float*        pfTopCrop, const float*        pfRightCrop, const float* pfBottomCrop)
{
    CTinyxml2Serializer serializer (mnMajorVersion, mnMinorVersion);
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
    return qualisys_cpp_sdk::SkeletonDofToStringSettings(dof);
}

EDegreeOfFreedom CRTProtocol::SkeletonStringToDof(const std::string& str)
{
    return qualisys_cpp_sdk::SkeletonStringToDofSettings(str);
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
        CRTPacket::EPacketType type;

        while (Receive(type, true, timeout) == CNetwork::ResponseType::success)
        {
            if (type == CRTPacket::PacketCommand)
            {
                const auto commandResponseArr = mpoRTPacket->GetCommandString();
                commandResponseStr = (commandResponseArr != nullptr ? std::string(commandResponseArr) : "");
                return true;
            }
            if (type == CRTPacket::PacketError)
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
    CRTPacket::EPacketType type;

    if (SendString(pCmdStr, CRTPacket::PacketXML))
    {
        if (Receive(type, true) == CNetwork::ResponseType::success)
        {
            if (type == CRTPacket::PacketCommand)
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
            else if (type == CRTPacket::PacketError)
            {
                strcpy(maErrorStr, mpoRTPacket->GetErrorString());
            }
            else
            {
                sprintf(maErrorStr, "Expected command response packet. Got packet type %d.", (int)type);
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


