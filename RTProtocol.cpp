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
    return mGeneralSettings.captureFrequency;
}

CRTProtocol::CRTProtocol()
{
    mNetwork      = new CNetwork();
    mRTPacket     = nullptr;
    mLastEvent     = CRTPacket::EventCaptureStopped;
    mState         = CRTPacket::EventCaptureStopped;
    mMajorVersion  = 1;
    mMinorVersion  = 0;
    mBigEndian     = false;
    mErrorStr[0]   = 0;
    mBroadcastPort = 0;
    mFileBuffer    = nullptr;
    mIsMaster = false;
    mDataBuff.resize(65535);
    mSendBuffer.resize(5000);
} // CRTProtocol


CRTProtocol::~CRTProtocol()
{
    if (mNetwork)
    {
        delete mNetwork;
        mNetwork = nullptr;
    }
    if (mRTPacket)
    {
        delete mRTPacket;
        mRTPacket = nullptr;
    }
} // ~CRTProtocol

bool CRTProtocol::Connect(const char* serverAddr, unsigned short port, unsigned short* udpServerPort,
                          int majorVersion, int minorVersion, bool bigEndian, bool negotiateVersion)
{
    CRTPacket::EPacketType type;
    std::string            tempStr;
    std::string            responseStr;

    mBigEndian = bigEndian;
    mIsMaster = false;
    mMajorVersion = 1;
    if ((majorVersion == 1) && (minorVersion == 0))
    {
        mMinorVersion = 0;
    }
    else
    {
        mMinorVersion = 1;
        if (mBigEndian)
        {
            port += 2;
        }
        else
        {
            port += 1;
        }
    }

    if (mRTPacket)
    {
        delete mRTPacket;
    }

    mRTPacket = new CRTPacket(majorVersion, minorVersion, bigEndian);

    if (mRTPacket == nullptr)
    {
        strcpy(mErrorStr, "Could not allocate data packet.");
        return false;
    }

    if (mNetwork->Connect(serverAddr, port))
    {
        if (udpServerPort != nullptr)
        {
            if (mNetwork->CreateUDPSocket(*udpServerPort) == false)
            {
                sprintf(mErrorStr, "CreateUDPSocket failed. %s", mNetwork->GetErrorString());
                Disconnect();
                return false;
            }
        }

        // Welcome message
        if (Receive(type, true) == CNetwork::ResponseType::success)
        {
            if (type == CRTPacket::PacketError)
            {
                strcpy(mErrorStr, mRTPacket->GetErrorString());
                Disconnect();
                return false;
            }
            else if (type == CRTPacket::PacketCommand)
            {
                const std::string welcomeMessage("QTM RT Interface connected");
                if (strncmp(welcomeMessage.c_str(), mRTPacket->GetCommandString(), welcomeMessage.size()) == 0)
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
                            if ((mMajorVersion == 1) && (mMinorVersion == 0))
                            {
                                if (mBigEndian)
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
                                    strcpy(mErrorStr, "Set byte order failed.");
                                }
                            }
                            else
                            {
                                GetState(mState, true);
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
        if (mNetwork->GetError() == 10061)
        {
            strcpy(mErrorStr, "Check if QTM is running on target machine.");
        }
        else
        {
            strcpy(mErrorStr, mNetwork->GetErrorString());
        }
    }
    Disconnect();
    return false;
} // Connect


unsigned short CRTProtocol::GetUdpServerPort()
{
    if (mNetwork)
    {
        return mNetwork->GetUdpServerPort();
    }
    return 0;
}


void CRTProtocol::Disconnect()
{
    mNetwork->Disconnect();
    mBroadcastPort = 0;
    if (mRTPacket)
    {
        delete mRTPacket;
        mRTPacket = nullptr;
    }
    mIsMaster = false;
} // Disconnect


bool CRTProtocol::Connected() const
{
    return mNetwork->Connected();
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
            mMajorVersion = majorVersion;
            mMinorVersion = minorVersion;
            mRTPacket->SetVersion(mMajorVersion, mMinorVersion);
            return true;
        }

        if (!responseStr.empty())
        {
            sprintf(mErrorStr, "%s.", responseStr.c_str());
        }
        else
        {
            strcpy(mErrorStr, "Set Version failed.");
        }
    }
    else
    {
        tempStr = std::string(mErrorStr);
        sprintf(mErrorStr, "Send Version failed. %s.", tempStr.c_str());
    }
    return false;
}


bool CRTProtocol::GetVersion(unsigned int &majorVersion, unsigned int &minorVersion)
{
    if (!Connected())
    {
        return false;
    }

    majorVersion = mMajorVersion;
    minorVersion = mMinorVersion;

    return true;
}


bool CRTProtocol::GetQTMVersion(std::string& verStr)
{
    if (SendCommand("QTMVersion", verStr))
    {
        return true;
    }
    strcpy(mErrorStr, "Get QTM Version failed.");
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
    strcpy(mErrorStr, "Get Byte order failed.");
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
        strcpy(mErrorStr, "Wrong license code.");
    }
    else
    {
        strcpy(mErrorStr, "CheckLicense failed.");
    }

    return false;
}


bool CRTProtocol::DiscoverRTServer(unsigned short serverPort, bool noLocalResponses, unsigned short discoverPort)
{
    char pData[10];
    SDiscoverResponse sResponse;        

    if (mBroadcastPort == 0)
    {
        if (!mNetwork->CreateUDPSocket(serverPort, true))
        {
            strcpy(mErrorStr, mNetwork->GetErrorString());
            return false;
        }
        mBroadcastPort = serverPort;
    }
    else
    {
        serverPort = mBroadcastPort;
    }

    *((unsigned int*)pData)         = (unsigned int)10;
    *((unsigned int*)(pData + 4))   = (unsigned int)CRTPacket::PacketDiscover;
    *((unsigned short*)(pData + 8)) = htons(serverPort);

    if (mNetwork->SendUDPBroadcast(pData, 10, discoverPort))
    {
        mDiscoverResponseList.clear();

        CNetwork::Response response(CNetwork::ResponseType::error, 0);

        do 
        {
            unsigned int addr = 0;
            response = mNetwork->ReceiveUdpBroadcast(mDataBuff.data(), (int)mDataBuff.size(), 100000, &addr);

            if (response && response.received > qtmPacketHeaderSize)
            {
                if (CRTPacket::GetType(mDataBuff.data()) == CRTPacket::PacketCommand)
                {
                    char* discoverResponse  = CRTPacket::GetCommandString(mDataBuff.data());
                    sResponse.addr = addr;
                    sResponse.basePort = CRTPacket::GetDiscoverResponseBasePort(mDataBuff.data());

                    if (discoverResponse && (!noLocalResponses || !mNetwork->IsLocalAddress(addr)))
                    {
                        strcpy(sResponse.message, discoverResponse);
                        mDiscoverResponseList.push_back(sResponse);
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
    return (int)mDiscoverResponseList.size();
}


bool CRTProtocol::GetDiscoverResponse(unsigned int index, unsigned int &addr, unsigned short &basePort, std::string& message)
{
    if (index < mDiscoverResponseList.size())
    {
        addr     = mDiscoverResponseList[index].addr;
        basePort = mDiscoverResponseList[index].basePort;
        message   = mDiscoverResponseList[index].message;
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
    strcpy(mErrorStr, "GetCurrentFrame failed.");

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
        strcpy(mErrorStr, "DataComponent missing.");
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
            strcpy(mErrorStr, "UDP address string too long.");
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
    strcpy(mErrorStr, "StreamFrames failed.");

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
        strcpy(mErrorStr, "DataComponent missing.");
    }

    return false;
}


bool CRTProtocol::StreamFramesStop()
{
    if (SendCommand("StreamFrames Stop"))
    {
        return true;
    }
    strcpy(mErrorStr, "StreamFrames Stop failed.");
    return false;
}


bool CRTProtocol::GetState(CRTPacket::EEvent &event, bool update, int timeout)
{
    CRTPacket::EPacketType type;

    if (update)
    {
        bool result;
        if (mMajorVersion > 1 || mMinorVersion > 9)
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
                    if (mRTPacket->GetEvent(event))
                    {
                        return true;
                    }
                }
            } while (response == CNetwork::ResponseType::success);
        }
        strcpy(mErrorStr, "GetLastEvent failed.");
    }
    else
    {
        event = mState;
        return true;
    }
    return false;
}


bool CRTProtocol::GetCapture(const char* fileName, bool isC3D)
{
    CRTPacket::EPacketType type;
    std::string            responseStr;

    mFileBuffer = fopen(fileName, "wb");
    if (mFileBuffer != nullptr)
    {
        if (isC3D)
        {
            // C3D file
            if (SendCommand((mMajorVersion > 1 || mMinorVersion > 7) ? "GetCaptureC3D" : "GetCapture", responseStr))
            {
                if (responseStr == "Sending capture")
                {
                    if (Receive(type, true, 5000000) == CNetwork::ResponseType::success) // Wait for C3D file in 5 seconds.
                    {
                        if (type == CRTPacket::PacketC3DFile)
                        {
                            if (mFileBuffer != nullptr)
                            {
                                fclose(mFileBuffer);
                                return true;
                            }
                            strcpy(mErrorStr, "Writing C3D file failed.");
                        }
                        else
                        {
                            strcpy(mErrorStr, "Wrong packet type received.");
                        }
                    }
                    else
                    {
                        strcpy(mErrorStr, "No packet received.");
                    }
                }
                else
                {
                    sprintf(mErrorStr, "%s failed.", (mMajorVersion > 1 || mMinorVersion > 7) ? "GetCaptureC3D" : "GetCapture");
                }
            }
            else
            {
                sprintf(mErrorStr, "%s failed.", (mMajorVersion > 1 || mMinorVersion > 7) ? "GetCaptureC3D" : "GetCapture");
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
                            if (mFileBuffer != nullptr)
                            {
                                fclose(mFileBuffer);
                                return true;
                            }
                            strcpy(mErrorStr, "Writing QTM file failed.");
                        }
                        else
                        {
                            strcpy(mErrorStr, "Wrong packet type received.");
                        }
                    }
                    else
                    {
                        sprintf(mErrorStr, "No packet received. %s.", mErrorStr);
                    }
                }
                else
                {
                    strcpy(mErrorStr, "GetCaptureQTM failed.");
                }
            }
            else
            {
                strcpy(mErrorStr, "GetCaptureQTM failed.");
            }
        }
    }
    if (mFileBuffer)
    {
        fclose(mFileBuffer);
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
        sprintf(mErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(mErrorStr, "Trig failed.");
    }
    return false;
}


bool CRTProtocol::SetQTMEvent(const std::string& label)
{
    std::string responseStr;
    std::string tempStr = (mMajorVersion > 1 || mMinorVersion > 7) ? "SetQTMEvent " : "Event ";

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
        sprintf(mErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        sprintf(mErrorStr, "%s failed.", (mMajorVersion > 1 || mMinorVersion > 7) ? "SetQTMEvent" : "Event");
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
            mIsMaster = true;
            return true;
        }
    }
    if (!responseStr.empty())
    {
        sprintf(mErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(mErrorStr, "TakeControl failed.");
    }

    mIsMaster = false;
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
            mIsMaster = false;
            return true;
        }
    }
    if (!responseStr.empty())
    {
        sprintf(mErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(mErrorStr, "ReleaseControl failed.");
    }
    return false;
} // ReleaseControl


bool CRTProtocol::IsControlling()
{
    return mIsMaster;
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
        sprintf(mErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(mErrorStr, "New failed.");
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
        sprintf(mErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(mErrorStr, "Close failed.");
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
        sprintf(mErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(mErrorStr, "Start failed.");
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
        sprintf(mErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(mErrorStr, "Starting RT from file failed.");
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
        sprintf(mErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(mErrorStr, "Stop failed.");
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
        sprintf(mErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(mErrorStr, "Calibrate failed.");
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
        sprintf(mErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(mErrorStr, "Load failed.");
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
        sprintf(mErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(mErrorStr, "Save failed.");
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
        sprintf(mErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(mErrorStr, "Load project failed.");
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
        sprintf(mErrorStr, "%s.", responseStr.c_str());
    }
    else
    {
        strcpy(mErrorStr, "Reprocess failed.");
    }
    return false;
}

void CRTProtocol::OverrideNetwork(INetwork* network)
{
    delete mNetwork;
    mNetwork = network;
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


int CRTProtocol::ReceiveRTPacket(CRTPacket::EPacketType &type, bool skipEvents, int timeout)
{
    int returnVal = -1;
    auto response = Receive(type, skipEvents, timeout);
    
    switch (response)
    {
        case CNetwork::ResponseType::success:
            returnVal = mRTPacket->GetSize();
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


CNetwork::ResponseType CRTProtocol::Receive(CRTPacket::EPacketType &type, bool skipEvents, int timeout)
{
    CNetwork::Response response(CNetwork::ResponseType::error, 0);
    unsigned int nRecvedTotal = 0;
    unsigned int nFrameSize;

    type = CRTPacket::PacketNone;

    do 
    {
        nRecvedTotal = 0;

        response = mNetwork->Receive(mDataBuff.data(), (int)mDataBuff.size(), true, timeout);

        if (response.type == CNetwork::ResponseType::timeout)
        {
            // Receive timeout.
            strcpy(mErrorStr, "Data receive timeout.");
            return CNetwork::ResponseType::timeout;
        }
        if (response.type == CNetwork::ResponseType::error)
        {
            strcpy(mErrorStr, "Socket Error.");
            return CNetwork::ResponseType::error;
        }
        if (response.type == CNetwork::ResponseType::disconnect)
        {
            strcpy(mErrorStr, "Disconnected from server.");
            return CNetwork::ResponseType::disconnect;
        }        
        if (response.received < qtmPacketHeaderSize)
        {
            // QTM header not received.
            strcpy(mErrorStr, "Couldn't read header bytes.");
            return CNetwork::ResponseType::error;
        }
        nRecvedTotal += response.received;

        bool bigEndian = (mBigEndian || (mMajorVersion == 1 && mMinorVersion == 0));
        nFrameSize = mRTPacket->GetSize(mDataBuff.data(), bigEndian);
        type      = mRTPacket->GetType(mDataBuff.data(), bigEndian);
        
        unsigned int nReadSize;

        if (type == CRTPacket::PacketC3DFile || type == CRTPacket::PacketQTMFile)
        {
            if (mFileBuffer != nullptr)
            {
                rewind(mFileBuffer); // Start from the beginning
                if (fwrite(mDataBuff.data() + sizeof(int) * 2, 1, nRecvedTotal - sizeof(int) * 2, mFileBuffer) !=
                    nRecvedTotal - sizeof(int) * 2)
                {
                    strcpy(mErrorStr, "Failed to write file to disk.");
                    fclose(mFileBuffer);
                    mFileBuffer = nullptr;
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
                    response = mNetwork->Receive(&(mDataBuff.data()[sizeof(int) * 2]), nReadSize, false, cWaitForDataTimeout);
                    if (response.type == CNetwork::ResponseType::timeout)
                    {
                        strcpy(mErrorStr, "Packet truncated.");
                        return CNetwork::ResponseType::error;
                    }
                    if (response.type == CNetwork::ResponseType::error)
                    {
                        strcpy(mErrorStr, "Socket Error.");
                        fclose(mFileBuffer);
                        mFileBuffer = nullptr;
                        return CNetwork::ResponseType::error;
                    }
                    if (response.type == CNetwork::ResponseType::disconnect)
                    {
                        strcpy(mErrorStr, "Disconnected from server.");
                        return CNetwork::ResponseType::disconnect;
                    }

                    if (fwrite(mDataBuff.data() + sizeof(int) * 2, 1, response.received, mFileBuffer) != (size_t)(response.received))
                    {
                        strcpy(mErrorStr, "Failed to write file to disk.");
                        fclose(mFileBuffer);
                        mFileBuffer = nullptr;
                        return CNetwork::ResponseType::error;
                    }
                    nRecvedTotal += response.received;
                }
            }
            else
            {
                strcpy(mErrorStr, "Receive file buffer not opened.");
                if (mFileBuffer)
                {
                    fclose(mFileBuffer);
                }
                mFileBuffer = nullptr;
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
                response = mNetwork->Receive(&(mDataBuff.data()[nRecvedTotal]), nFrameSize - nRecvedTotal, false, -1);
                if (response.type == CNetwork::ResponseType::timeout)
                {
                    strcpy(mErrorStr, "Packet truncated.");
                    return CNetwork::ResponseType::error;
                }
                if (response.type == CNetwork::ResponseType::error)
                {
                    strcpy(mErrorStr, "Socket Error.");
                    return CNetwork::ResponseType::error;
                }
                if (response.type == CNetwork::ResponseType::disconnect)
                {
                    strcpy(mErrorStr, "Disconnected from server.");
                    return CNetwork::ResponseType::disconnect;
                }
                nRecvedTotal += response.received;
            }
        }

        mRTPacket->SetData(mDataBuff.data());

        if (mRTPacket->GetEvent(mLastEvent)) // Update last event if there is an event
        {
            if (mLastEvent != CRTPacket::EventCameraSettingsChanged)
            {
                mState = mLastEvent;
            }
        }
    } while (skipEvents && type == CRTPacket::PacketEvent);
    
    if (nRecvedTotal == nFrameSize)
    {
        return CNetwork::ResponseType::success;
    }
    strcpy(mErrorStr, "Packet truncated.");

    return CNetwork::ResponseType::error;
} // ReceiveRTPacket


CRTPacket* CRTProtocol::GetRTPacket()
{
    return mRTPacket;
}


const char * CRTProtocol::ReadSettings(const std::string& settingsType)
{
    CRTPacket::EPacketType type;

    mAnalogDeviceSettings.clear();
    auto sendStr = std::string("GetParameters ") + settingsType;
    if (!SendCommand(sendStr.c_str()))
    {
        sprintf(mErrorStr, "GetParameters %s failed", settingsType.c_str());
        return nullptr;
    }

retry:
    auto received = Receive(type, true);

    if (received == CNetwork::ResponseType::timeout)
    {
        strcat(mErrorStr, " Expected XML packet.");
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
            sprintf(mErrorStr, "%s.", mRTPacket->GetErrorString());
            return nullptr;
        }
        else
        {
            goto retry;
            //sprintf(mErrorStr, "GetParameters %s returned wrong packet type. Got type %d expected type 2.", settingsType.c_str(), type);
        }
    }

    return mRTPacket->GetXMLString();
}



bool CRTProtocol::ReadCameraSystemSettings()
{
    return ReadGeneralSettings();
}


bool CRTProtocol::ReadGeneralSettings()
{
    std::string             tStr;

    mGeneralSettings.cameras.clear();

    const char* data = ReadSettings("General");

    if (!data)
    {
        return false;
    }

    CTinyxml2Deserializer serializer(data, mMajorVersion, mMinorVersion);

    return serializer.DeserializeGeneralSettings(mGeneralSettings);

} // ReadGeneralSettings

bool CRTProtocol::ReadCalibrationSettings()
{
    if (!SendCommand("GetParameters Calibration"))
    {
        strcpy(mErrorStr, "GetParameters Calibration failed");
        return false;
    }

    return ReceiveCalibrationSettings();
}

bool CRTProtocol::Read3DSettings(bool& dataAvailable)
{
    dataAvailable = false;
    m3DSettings.labels3D.clear();
    m3DSettings.calibrationTime[0] = 0;

    const char* data = ReadSettings("3D");
    if (!data)
    {
        return false;
    }

    CTinyxml2Deserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.Deserialize3DSettings(m3DSettings, dataAvailable);
}

bool CRTProtocol::Read6DOFSettings(bool& dataAvailable)
{
    m6DOFSettings.clear();

    const auto* data = ReadSettings("6D");
    if(!data)
    {
        return false;
    }

    CTinyxml2Deserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.Deserialize6DOFSettings(m6DOFSettings, mGeneralSettings, dataAvailable);
}

bool CRTProtocol::ReadGazeVectorSettings(bool& dataAvailable)
{
    dataAvailable = false;

    mGazeVectorSettings.clear();

    const auto* data = ReadSettings("GazeVector");
    if(!data)
    {
        return false;
    }

    CTinyxml2Deserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.DeserializeGazeVectorSettings(mGazeVectorSettings, dataAvailable);
}

bool CRTProtocol::ReadEyeTrackerSettings(bool& dataAvailable)
{
    dataAvailable = false;

    mEyeTrackerSettings.clear();

    const auto* data = ReadSettings("EyeTracker");
    if(!data)
    {
        return false;
    }

    CTinyxml2Deserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.DeserializeEyeTrackerSettings(mEyeTrackerSettings, dataAvailable);
}

bool CRTProtocol::ReadAnalogSettings(bool& dataAvailable)
{
    dataAvailable = false;

    mAnalogDeviceSettings.clear();

    const auto* data = ReadSettings("Analog");
    if(!data)
    {
        return false;
    }
    CTinyxml2Deserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.DeserializeAnalogSettings(mAnalogDeviceSettings, dataAvailable);
}

bool CRTProtocol::ReadForceSettings(bool& dataAvailable)
{
    dataAvailable = false;

    mForceSettings.forcePlates.clear();

    const auto* data = ReadSettings("Force");
    if(!data)
    {
        return false;
    }

    CTinyxml2Deserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.DeserializeForceSettings(mForceSettings, dataAvailable);
}

bool CRTProtocol::ReadImageSettings(bool& dataAvailable)
{
    dataAvailable = false;

    mImageSettings.clear();

    const auto* data = ReadSettings("Image");
    if(!data)
    {
        return false;
    }

    CTinyxml2Deserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.DeserializeImageSettings(mImageSettings, dataAvailable);
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

    CTinyxml2Deserializer deserializer(data, mMajorVersion, mMinorVersion);
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
            strcat(mErrorStr, " Expected XML packet.");
            return false;
        }
        if (response == CNetwork::ResponseType::error)
        {
            return false;
        }

        if (type == CRTPacket::PacketEvent)
        {
            mRTPacket->GetEvent(event);
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
            sprintf(mErrorStr, "Calibration aborted.");
        }
        else if (type == CRTPacket::PacketError)
        {
            sprintf(mErrorStr, "%s.", mRTPacket->GetErrorString());
        }
        else
        {
            sprintf(mErrorStr, "GetParameters Calibration returned wrong packet type. Got type %d expected type 2.", type);
        }
        return false;
    }

    auto data = mRTPacket->GetXMLString();
    CTinyxml2Deserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.DeserializeCalibrationSettings(mCalibrationSettings);
} // ReadCalibrationSettings

void CRTProtocol::Get3DSettings(EAxis& axisUpwards, std::string& calibrationTime, std::vector<SSettings3DLabel>& labels3D, std::vector<SSettingsBone>& bones)
{
    axisUpwards = m3DSettings.axisUpwards;
    calibrationTime = static_cast<std::string>(m3DSettings.calibrationTime);

    labels3D = m3DSettings.labels3D;
    bones = m3DSettings.bones;
}

void CRTProtocol::GetGazeVectorSettings(std::vector<SGazeVector>& gazeVectorSettings)
{
    gazeVectorSettings = mGazeVectorSettings;
}

void CRTProtocol::GetEyeTrackerSettings(std::vector<SEyeTracker>& eyeTrackerSettings)
{
    eyeTrackerSettings = mEyeTrackerSettings;
}

void CRTProtocol::GetAnalogSettings(std::vector<SAnalogDevice>& analogSettings)
{
    analogSettings = mAnalogDeviceSettings;
}

void CRTProtocol::GetForceSettings(SSettingsForce& forceSettings)
{
    forceSettings = mForceSettings;
}

void CRTProtocol::GetGeneralSettings(
    unsigned int       &captureFrequency, float &captureTime,
    bool& startOnExtTrig, bool& startOnTrigNO, bool& startOnTrigNC, bool& startOnTrigSoftware,
    EProcessingActions &processingActions, EProcessingActions &rtProcessingActions, EProcessingActions &reprocessingActions) const
{
    captureFrequency = mGeneralSettings.captureFrequency;
    captureTime = mGeneralSettings.captureTime;
    startOnExtTrig = mGeneralSettings.startOnExternalTrigger;
    startOnTrigNO = mGeneralSettings.startOnTrigNO;
    startOnTrigNC = mGeneralSettings.startOnTrigNC;
    startOnTrigSoftware = mGeneralSettings.startOnTrigSoftware;
    processingActions = mGeneralSettings.processingActions;
    rtProcessingActions = mGeneralSettings.rtProcessingActions;
    reprocessingActions = mGeneralSettings.reprocessingActions;
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
    enabled            = mGeneralSettings.externalTimebase.enabled;
    signalSource       = mGeneralSettings.externalTimebase.signalSource;
    signalModePeriodic = mGeneralSettings.externalTimebase.signalModePeriodic;
    freqMultiplier     = mGeneralSettings.externalTimebase.freqMultiplier;
    freqDivisor        = mGeneralSettings.externalTimebase.freqDivisor;
    freqTolerance      = mGeneralSettings.externalTimebase.freqTolerance;
    nominalFrequency   = mGeneralSettings.externalTimebase.nominalFrequency;
    negativeEdge       = mGeneralSettings.externalTimebase.negativeEdge;
    signalShutterDelay = mGeneralSettings.externalTimebase.signalShutterDelay;
    nonPeriodicTimeout = mGeneralSettings.externalTimebase.nonPeriodicTimeout;
}

void CRTProtocol::GetExtTimestampSettings(SSettingsGeneralExternalTimestamp& timestamp) const
{
    timestamp = mGeneralSettings.timestamp;
}

void CRTProtocol::GetEulerAngles(std::string& first, std::string& second, std::string& third) const
{
    first = mGeneralSettings.eulerRotations[0];
    second = mGeneralSettings.eulerRotations[1];
    third = mGeneralSettings.eulerRotations[2];
}

unsigned int CRTProtocol::GetCameraCount() const
{
    return (unsigned int)mGeneralSettings.cameras.size();
}

std::vector<CRTProtocol::SSettingsGeneralCamera> CRTProtocol::GetDevices() const
{
    return mGeneralSettings.cameras;
}


bool CRTProtocol::GetCameraSettings(
    unsigned int cameraIndex, unsigned int &id,     ECameraModel &model,
    bool         &underwater, bool &supportsHwSync, unsigned int &serial, ECameraMode  &mode) const
{
    if (cameraIndex < mGeneralSettings.cameras.size())
    {
        id             = mGeneralSettings.cameras[cameraIndex].id;
        model          = mGeneralSettings.cameras[cameraIndex].model;
        underwater     = mGeneralSettings.cameras[cameraIndex].underwater;
        supportsHwSync = mGeneralSettings.cameras[cameraIndex].supportsHwSync;
        serial         = mGeneralSettings.cameras[cameraIndex].serial;
        mode           = mGeneralSettings.cameras[cameraIndex].mode;
        return true;
    }
    return false;
}


bool CRTProtocol::GetCameraMarkerSettings(
    unsigned int cameraIndex,   unsigned int &currentExposure, unsigned int &minExposure,
    unsigned int &maxExposure,  unsigned int &currentThreshold,
    unsigned int &minThreshold, unsigned int &maxThreshold) const
{
    if (cameraIndex < mGeneralSettings.cameras.size())
    {
        currentExposure  = mGeneralSettings.cameras[cameraIndex].markerExposure;
        minExposure      = mGeneralSettings.cameras[cameraIndex].markerExposureMin;
        maxExposure      = mGeneralSettings.cameras[cameraIndex].markerExposureMax;
        currentThreshold = mGeneralSettings.cameras[cameraIndex].markerThreshold;
        minThreshold     = mGeneralSettings.cameras[cameraIndex].markerThresholdMin;
        maxThreshold     = mGeneralSettings.cameras[cameraIndex].markerThresholdMax;
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
    if (cameraIndex < mGeneralSettings.cameras.size())
    {
        videoResolution   = mGeneralSettings.cameras[cameraIndex].videoResolution;
        videoAspectRatio  = mGeneralSettings.cameras[cameraIndex].videoAspectRatio;
        videoFrequency   = mGeneralSettings.cameras[cameraIndex].videoFrequency;
        currentExposure  = mGeneralSettings.cameras[cameraIndex].videoExposure;
        minExposure      = mGeneralSettings.cameras[cameraIndex].videoExposureMin;
        maxExposure      = mGeneralSettings.cameras[cameraIndex].videoExposureMax;
        currentFlashTime = mGeneralSettings.cameras[cameraIndex].videoFlashTime;
        minFlashTime     = mGeneralSettings.cameras[cameraIndex].videoFlashTimeMin;
        maxFlashTime     = mGeneralSettings.cameras[cameraIndex].videoFlashTimeMax;
        return true;
    }
    return false;
}


bool CRTProtocol::GetCameraSyncOutSettings(
    unsigned int cameraIndex, unsigned int portNumber, ESyncOutFreqMode &syncOutMode,
    unsigned int &syncOutValue, float      &syncOutDutyCycle,
    bool         &syncOutNegativePolarity) const
{
    if (cameraIndex < mGeneralSettings.cameras.size())
    {
        if (portNumber == 1 || portNumber == 2)
        {
            syncOutMode = mGeneralSettings.cameras[cameraIndex].syncOutMode[portNumber - 1];
            syncOutValue = mGeneralSettings.cameras[cameraIndex].syncOutValue[portNumber - 1];
            syncOutDutyCycle = mGeneralSettings.cameras[cameraIndex].syncOutDutyCycle[portNumber - 1];
        }
        if (portNumber > 0 && portNumber < 4)
        {
            syncOutNegativePolarity = mGeneralSettings.cameras[cameraIndex].syncOutNegativePolarity[portNumber - 1];
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
    if (cameraIndex < mGeneralSettings.cameras.size())
    {
        point.x = mGeneralSettings.cameras[cameraIndex].positionX;
        point.y = mGeneralSettings.cameras[cameraIndex].positionY;
        point.z = mGeneralSettings.cameras[cameraIndex].positionZ;
        memcpy(rotationMatrix, mGeneralSettings.cameras[cameraIndex].positionRotMatrix, 9 * sizeof(float));
        return true;
    }
    return false;
}


bool CRTProtocol::GetCameraOrientation(
    unsigned int cameraIndex, int &orientation) const
{
    if (cameraIndex < mGeneralSettings.cameras.size())
    {
        orientation = mGeneralSettings.cameras[cameraIndex].orientation;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraResolution(
    unsigned int cameraIndex, unsigned int &markerWidth, unsigned int &markerHeight,
    unsigned int &videoWidth, unsigned int &videoHeight) const
{
    if (cameraIndex < mGeneralSettings.cameras.size())
    {
        markerWidth  = mGeneralSettings.cameras[cameraIndex].markerResolutionWidth;
        markerHeight = mGeneralSettings.cameras[cameraIndex].markerResolutionHeight;
        videoWidth   = mGeneralSettings.cameras[cameraIndex].videoResolutionWidth;
        videoHeight  = mGeneralSettings.cameras[cameraIndex].videoResolutionHeight;
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
    if (cameraIndex < mGeneralSettings.cameras.size())
    {
        markerLeft   = mGeneralSettings.cameras[cameraIndex].markerFOVLeft;
        markerTop    = mGeneralSettings.cameras[cameraIndex].markerFOVTop;
        markerRight  = mGeneralSettings.cameras[cameraIndex].markerFOVRight;
        markerBottom = mGeneralSettings.cameras[cameraIndex].markerFOVBottom;
        videoLeft    = mGeneralSettings.cameras[cameraIndex].videoFOVLeft;
        videoTop     = mGeneralSettings.cameras[cameraIndex].videoFOVTop;
        videoRight   = mGeneralSettings.cameras[cameraIndex].videoFOVRight;
        videoBottom  = mGeneralSettings.cameras[cameraIndex].videoFOVBottom;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraLensControlSettings(const unsigned int cameraIndex, float* focus, float* aperture) const
{
    if (cameraIndex < mGeneralSettings.cameras.size())
    {
        *focus = mGeneralSettings.cameras[cameraIndex].focus;
        if (std::isnan(*focus))
            return false;
        *aperture = mGeneralSettings.cameras[cameraIndex].aperture;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraAutoExposureSettings(const unsigned int cameraIndex, bool* autoExposureEnabled, float* autoExposureCompensation) const
{
    if (cameraIndex < mGeneralSettings.cameras.size())
    {
        *autoExposureCompensation = mGeneralSettings.cameras[cameraIndex].autoExposureCompensation;
        if (std::isnan(*autoExposureCompensation))
            return false;
        *autoExposureEnabled = mGeneralSettings.cameras[cameraIndex].autoExposureEnabled;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraAutoWhiteBalance(const unsigned int cameraIndex, bool* autoWhiteBalanceEnabled) const
{
    if (cameraIndex < mGeneralSettings.cameras.size() && mGeneralSettings.cameras[cameraIndex].autoWhiteBalance >= 0)
    {
        *autoWhiteBalanceEnabled = mGeneralSettings.cameras[cameraIndex].autoWhiteBalance == 1;
        return true;
    }
    return false;
}

CRTProtocol::EAxis CRTProtocol::Get3DUpwardAxis() const
{
    return m3DSettings.axisUpwards;
}

const char* CRTProtocol::Get3DCalibrated() const
{
    return m3DSettings.calibrationTime;
}

unsigned int CRTProtocol::Get3DLabeledMarkerCount() const
{
    return (unsigned int)m3DSettings.labels3D.size();
}

const char* CRTProtocol::Get3DLabelName(unsigned int markerIndex) const
{
    if (markerIndex < m3DSettings.labels3D.size())
    {
        return m3DSettings.labels3D[markerIndex].name.c_str();
    }
    return nullptr;
}

unsigned int CRTProtocol::Get3DLabelColor(unsigned int markerIndex) const
{
    if (markerIndex < m3DSettings.labels3D.size())
    {
        return m3DSettings.labels3D[markerIndex].rgbColor;
    }
    return 0;
}

const char* CRTProtocol::Get3DTrajectoryType(unsigned int markerIndex) const
{
    if (markerIndex < m3DSettings.labels3D.size())
    {
        return m3DSettings.labels3D[markerIndex].type.c_str();
    }
    return 0;
}


unsigned int CRTProtocol::Get3DBoneCount() const
{
    return (unsigned int)m3DSettings.bones.size();
}

const char* CRTProtocol::Get3DBoneFromName(unsigned int boneIndex) const
{
    if (boneIndex < m3DSettings.bones.size())
    {
        return m3DSettings.bones[boneIndex].fromName.c_str();
    }
    return nullptr;
}

const char* CRTProtocol::Get3DBoneToName(unsigned int boneIndex) const
{
    if (boneIndex < m3DSettings.bones.size())
    {
        return m3DSettings.bones[boneIndex].toName.c_str();
    }
    return nullptr;
}

void CRTProtocol::Get6DOFEulerNames(std::string &first, std::string &second, std::string &third) const
{
    first = mGeneralSettings.eulerRotations[0];
    second = mGeneralSettings.eulerRotations[1];
    third = mGeneralSettings.eulerRotations[2];
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
    if (mMajorVersion == 1 && mMinorVersion < 21)
    {
        strcpy(mErrorStr, "Get6DOFBodySettings not available before protocol version 1.21");
        return false;
    }

    settings = m6DOFSettings;
    return true;
}

unsigned int CRTProtocol::GetGazeVectorCount() const
{
    return (unsigned int)mGazeVectorSettings.size();
}

const char* CRTProtocol::GetGazeVectorName(unsigned int gazeVectorIndex) const
{
    if (gazeVectorIndex < mGazeVectorSettings.size())
    {
        return mGazeVectorSettings[gazeVectorIndex].name.c_str();
    }
    return nullptr;
}

float CRTProtocol::GetGazeVectorFrequency(unsigned int gazeVectorIndex) const
{
    if (gazeVectorIndex < mGazeVectorSettings.size())
    {
        return mGazeVectorSettings[gazeVectorIndex].frequency;
    }
    return 0;
}

bool CRTProtocol::GetGazeVectorHardwareSyncUsed(unsigned int gazeVectorIndex) const
{
    if (gazeVectorIndex < mGazeVectorSettings.size())
    {
        return mGazeVectorSettings[gazeVectorIndex].hwSync;
    }
    return false;
}

bool CRTProtocol::GetGazeVectorFilterUsed(unsigned int gazeVectorIndex) const
{
    if (gazeVectorIndex < mGazeVectorSettings.size())
    {
        return mGazeVectorSettings[gazeVectorIndex].filter;
    }
    return false;
}


unsigned int CRTProtocol::GetEyeTrackerCount() const
{
    return (unsigned int)mEyeTrackerSettings.size();
}

const char* CRTProtocol::GetEyeTrackerName(unsigned int eyeTrackerIndex) const
{
    if (eyeTrackerIndex < mEyeTrackerSettings.size())
    {
        return mEyeTrackerSettings[eyeTrackerIndex].name.c_str();
    }
    return nullptr;
}

float CRTProtocol::GetEyeTrackerFrequency(unsigned int eyeTrackerIndex) const
{
    if (eyeTrackerIndex < mEyeTrackerSettings.size())
    {
        return mEyeTrackerSettings[eyeTrackerIndex].frequency;
    }
    return 0;
}

bool CRTProtocol::GetEyeTrackerHardwareSyncUsed(unsigned int eyeTrackerIndex) const
{
    if (eyeTrackerIndex < mEyeTrackerSettings.size())
    {
        return mEyeTrackerSettings[eyeTrackerIndex].hwSync;
    }
    return false;
}


unsigned int CRTProtocol::GetAnalogDeviceCount() const
{
    return (unsigned int)mAnalogDeviceSettings.size();
}


bool CRTProtocol::GetAnalogDevice(unsigned int deviceIndex, unsigned int &deviceID, unsigned int &channels,
                                  char* &name, unsigned int &frequency, char* &unit,
                                  float &minRange, float &maxRange) const
{
    if (deviceIndex < mAnalogDeviceSettings.size())
    {
        deviceID  = mAnalogDeviceSettings.at(deviceIndex).deviceID;
        name      = (char*)mAnalogDeviceSettings.at(deviceIndex).name.c_str();
        channels  = mAnalogDeviceSettings.at(deviceIndex).channels;
        frequency = mAnalogDeviceSettings.at(deviceIndex).frequency;
        unit      = (char*)mAnalogDeviceSettings.at(deviceIndex).unit.c_str();
        minRange  = mAnalogDeviceSettings.at(deviceIndex).minRange;
        maxRange  = mAnalogDeviceSettings.at(deviceIndex).maxRange;

        return true;
    }
    return false;
}


const char* CRTProtocol::GetAnalogLabel(unsigned int deviceIndex, unsigned int channelIndex) const
{
    if (deviceIndex < mAnalogDeviceSettings.size())
    {
        if (channelIndex < mAnalogDeviceSettings.at(deviceIndex).labels.size())
        {
            return mAnalogDeviceSettings.at(deviceIndex).labels.at(channelIndex).c_str();
        }
    }
    return nullptr;
}


const char* CRTProtocol::GetAnalogUnit(unsigned int deviceIndex, unsigned int channelIndex) const
{
    if (deviceIndex < mAnalogDeviceSettings.size())
    {
        if (channelIndex < mAnalogDeviceSettings.at(deviceIndex).units.size())
        {
            return mAnalogDeviceSettings.at(deviceIndex).units.at(channelIndex).c_str();
        }
    }
    return nullptr;
}


void CRTProtocol::GetForceUnits(char* &length, char* &force) const
{
    length = (char*)mForceSettings.unitLength.c_str();
    force  = (char*)mForceSettings.unitForce.c_str();
}


unsigned int CRTProtocol::GetForcePlateCount() const
{
    return (unsigned int)mForceSettings.forcePlates.size();
}


bool CRTProtocol::GetForcePlate(unsigned int plateIndex, unsigned int &id, unsigned int &analogDeviceID,
                                unsigned int &frequency, char* &type, char* &name, float &length, float &width) const
{
    if (plateIndex < mForceSettings.forcePlates.size())
    {
        id             = mForceSettings.forcePlates[plateIndex].id;
        analogDeviceID = mForceSettings.forcePlates[plateIndex].analogDeviceID;
        frequency      = mForceSettings.forcePlates[plateIndex].frequency;
        type           = (char*)mForceSettings.forcePlates[plateIndex].type.c_str();
        name           = (char*)mForceSettings.forcePlates[plateIndex].name.c_str();
        length         = mForceSettings.forcePlates[plateIndex].length;
        width          = mForceSettings.forcePlates[plateIndex].width;
        return true;
    }
    return false;
}


bool CRTProtocol::GetForcePlateLocation(unsigned int plateIndex, SPoint corner[4]) const
{
    if (plateIndex < mForceSettings.forcePlates.size())
    {
        memcpy(corner, mForceSettings.forcePlates[plateIndex].corner, 3 * 4 * sizeof(float));
        return true;
    }
    return false;
}


bool CRTProtocol::GetForcePlateOrigin(unsigned int plateIndex, SPoint &origin) const
{
    if (plateIndex < mForceSettings.forcePlates.size())
    {
        origin = mForceSettings.forcePlates[plateIndex].origin;
        return true;
    }
    return false;
}


unsigned int CRTProtocol::GetForcePlateChannelCount(unsigned int plateIndex) const
{
    if (plateIndex < mForceSettings.forcePlates.size())
    {
        return (unsigned int)mForceSettings.forcePlates[plateIndex].channels.size();
    }
    return 0;
}


bool CRTProtocol::GetForcePlateChannel(unsigned int plateIndex, unsigned int channelIndex,
                                       unsigned int &channelNumber, float &conversionFactor) const
{
    if (plateIndex < mForceSettings.forcePlates.size())
    {
        if (channelIndex < mForceSettings.forcePlates[plateIndex].channels.size())
        {
            channelNumber    = mForceSettings.forcePlates[plateIndex].channels[channelIndex].channelNumber;
            conversionFactor = mForceSettings.forcePlates[plateIndex].channels[channelIndex].conversionFactor;
            return true;
        }
    }
    return false;
}


bool CRTProtocol::GetForcePlateCalibrationMatrix(unsigned int plateIndex, float calMatrix[12][12], unsigned int* rows, unsigned int* columns) const
{
    if (plateIndex < mForceSettings.forcePlates.size())
    {
        if (mForceSettings.forcePlates[plateIndex].validCalibrationMatrix)
        {
            *rows = mForceSettings.forcePlates[plateIndex].calibrationMatrixRows;
            *columns = mForceSettings.forcePlates[plateIndex].calibrationMatrixColumns;
            memcpy(
                calMatrix,
                mForceSettings.forcePlates[plateIndex].calibrationMatrix,
                mForceSettings.forcePlates[plateIndex].calibrationMatrixRows * mForceSettings.forcePlates[plateIndex].calibrationMatrixColumns * sizeof(float));
            return true;
        }
    }
    return false;
}


unsigned int CRTProtocol::GetImageCameraCount() const
{
    return (unsigned int)mImageSettings.size();
}


bool CRTProtocol::GetImageCamera(unsigned int cameraIndex, unsigned int &cameraID, bool &enabled,
                                 CRTPacket::EImageFormat &format, unsigned int &width, unsigned int &height,
                                 float &cropLeft, float &cropTop, float &cropRight, float &cropBottom) const
{
    if (cameraIndex < mImageSettings.size())
    {
        cameraID   = mImageSettings[cameraIndex].id;
        enabled    = mImageSettings[cameraIndex].enabled;
        format     = mImageSettings[cameraIndex].format;
        width      = mImageSettings[cameraIndex].width;
        height     = mImageSettings[cameraIndex].height;
        cropLeft   = mImageSettings[cameraIndex].cropLeft;
        cropTop    = mImageSettings[cameraIndex].cropTop;
        cropRight  = mImageSettings[cameraIndex].cropRight;
        cropBottom = mImageSettings[cameraIndex].cropBottom;
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
    const unsigned int* captureFrequency, const float* captureTime,
    const bool* startOnExtTrig, const bool* startOnTrigNO, const bool* startOnTrigNC, const bool* startOnTrigSoftware,
    const EProcessingActions* processingActions, const EProcessingActions* rtProcessingActions, const EProcessingActions* reprocessingActions)
{
    return SetGeneralSettings(captureFrequency, captureTime, startOnExtTrig, startOnTrigNO, startOnTrigNC, startOnTrigSoftware, processingActions, rtProcessingActions, reprocessingActions);
}

bool CRTProtocol::SetGeneralSettings(
    const unsigned int* captureFrequency, const float* captureTime,
    const bool* startOnExtTrig, const bool* startOnTrigNO, const bool* startOnTrigNC, const bool* startOnTrigSoftware,
    const EProcessingActions* processingActions, const EProcessingActions* rtProcessingActions, const EProcessingActions* reprocessingActions)
{
    CTinyxml2Serializer serializer(mMajorVersion, mMinorVersion);
    auto message = serializer.SetGeneralSettings(captureFrequency, captureTime, startOnExtTrig,startOnTrigNO, startOnTrigNC, startOnTrigSoftware, processingActions, rtProcessingActions, reprocessingActions);
    
    if (SendXML(message.data()))
    {
        return true;
    }

    return false;
} // SetGeneral


bool CRTProtocol::SetExtTimeBaseSettings(
    const bool*         enabled,            const ESignalSource* signalSource,
    const bool*         signalModePeriodic, const unsigned int*  freqMultiplier,
    const unsigned int* freqDivisor,        const unsigned int*  freqTolerance,
    const float*        nominalFrequency,   const bool*          negativeEdge,
    const unsigned int* signalShutterDelay, const float*         nonPeriodicTimeout)
{
    CTinyxml2Serializer serializer(mMajorVersion, mMinorVersion);
    auto message = serializer.SetExtTimeBaseSettings(
        enabled, signalSource,
        signalModePeriodic, freqMultiplier,
        freqDivisor, freqTolerance,
        nominalFrequency, negativeEdge,
        signalShutterDelay, nonPeriodicTimeout
    );

    return SendXML(message.data());
} // SetGeneralExtTimeBase


bool CRTProtocol::SetExtTimestampSettings(const CRTProtocol::SSettingsGeneralExternalTimestamp& timestampSettings)
{
    CTinyxml2Serializer serializer(mMajorVersion, mMinorVersion);
    auto message = serializer.SetExtTimestampSettings(timestampSettings);

    return SendXML(message.data());
}


// cameraID starts on 1. If cameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraSettings(
    const unsigned int cameraID,        const ECameraMode* mode,
    const float*       markerExposure, const float*       markerThreshold,
    const int*         orientation)
{
    CTinyxml2Serializer serializer(mMajorVersion, mMinorVersion);
    auto message = serializer.SetCameraSettings(
        cameraID, mode,
        markerExposure, markerThreshold,
        orientation);

    return SendXML(message.data());
} // SetGeneralCamera


// cameraID starts on 1. If cameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraVideoSettings(
    const unsigned int cameraID,                const EVideoResolution* videoResolution,
    const EVideoAspectRatio* videoAspectRatio, const unsigned int* videoFrequency,
    const float* videoExposure,                const float* videoFlashTime)
{
    CTinyxml2Serializer serializer(mMajorVersion, mMinorVersion);
    auto message = serializer.SetCameraVideoSettings(
        cameraID, videoResolution,
        videoAspectRatio, videoFrequency,
        videoExposure, videoFlashTime
    );

    return SendXML(message.data());

} // SetGeneralCameraVideo


// cameraID starts on 1. If cameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraSyncOutSettings(
    const unsigned int  cameraID,         const unsigned int portNumber, const ESyncOutFreqMode* syncOutMode,
    const unsigned int* syncOutValue, const float*       syncOutDutyCycle,
    const bool*         syncOutNegativePolarity)
{
    CTinyxml2Serializer serializer(mMajorVersion, mMinorVersion);
    auto message = serializer.SetCameraSyncOutSettings(
        cameraID, portNumber, syncOutMode,
        syncOutValue, syncOutDutyCycle,
        syncOutNegativePolarity
    );

    return SendXML(message.data());
} // SetGeneralCameraSyncOut


  // cameraID starts on 1. If cameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraLensControlSettings(const unsigned int cameraID, const float focus, const float aperture)
{
    CTinyxml2Serializer serializer(mMajorVersion, mMinorVersion);
    auto message = serializer.SetCameraLensControlSettings(cameraID, focus, aperture);
    return SendXML(message.data());

}

// cameraID starts on 1. If cameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraAutoExposureSettings(const unsigned int cameraID, const bool autoExposure, const float compensation)
{
    CTinyxml2Serializer serializer(mMajorVersion, mMinorVersion);
    auto message = serializer.SetCameraAutoExposureSettings(cameraID, autoExposure, compensation);
    return SendXML(message.data());
}

// cameraID starts on 1. If cameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraAutoWhiteBalance(const unsigned int cameraID, const bool enable)
{
    CTinyxml2Serializer serializer(mMajorVersion, mMinorVersion);
    auto message = serializer.SetCameraAutoWhiteBalance(cameraID, enable);
    return SendXML(message.data());
}


bool CRTProtocol::SetImageSettings(
    const unsigned int  cameraID, const bool*         enable,    const CRTPacket::EImageFormat* format,
    const unsigned int* width,   const unsigned int* height,    const float* leftCrop,
    const float*        topCrop, const float*        rightCrop, const float* bottomCrop)
{
    CTinyxml2Serializer serializer (mMajorVersion, mMinorVersion);
    auto message = serializer.SetImageSettings(
        cameraID, enable, format,
        width, height, leftCrop,
        topCrop, rightCrop, bottomCrop
    );

    return SendXML(message.data());
} // SetImageSettings


bool CRTProtocol::SetForceSettings(
    const unsigned int plateID,  const SPoint* corner1, const SPoint* corner2,
    const SPoint*      corner3, const SPoint* corner4)
{
    if (plateID > 0)
    {
        CTinyxml2Serializer serializer(mMajorVersion, mMinorVersion);
        auto message = serializer.SetForceSettings(plateID, corner1, corner2,
            corner3, corner4);
        return SendXML(message.data());
    }
    else
    {
        sprintf(mErrorStr, "Illegal force plate id: %d.", plateID);
    }
    return false;
} // SetForceSettings

bool CRTProtocol::Set6DOFBodySettings(std::vector<SSettings6DOFBody> settings)
{
    if (mMajorVersion == 1 && mMinorVersion < 21)
    {
        strcpy(mErrorStr, "Set6DOFBodySettings only available for protocol version 1.21 and later.");

        return false;
    }

    CTinyxml2Serializer serializer(mMajorVersion, mMinorVersion);
    auto message = serializer.Set6DOFBodySettings(settings);

    return SendXML(message.data());
}

bool CRTProtocol::SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& skeletons)
{
    CTinyxml2Serializer serializer(mMajorVersion, mMinorVersion);
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
    return mErrorStr;
}


bool CRTProtocol::SendString(const char* cmdStr, int type)
{
    std::uint32_t nCmdStrLen = (int)strlen(cmdStr);
    std::uint32_t nSize = 8 + nCmdStrLen + 1; // Header size + length of the string + terminating null char

    if (nSize > mSendBuffer.size())
    {
        mSendBuffer.resize(nSize);
    }
    
    memcpy(mSendBuffer.data() + 8, cmdStr, nCmdStrLen + 1);

    if ((mMajorVersion == 1 && mMinorVersion == 0) || mBigEndian)
    {
        *((unsigned int*)mSendBuffer.data())       = htonl(nSize);
        *((unsigned int*)(mSendBuffer.data() + 4)) = htonl(type);
    }
    else
    {
        *((unsigned int*)mSendBuffer.data())       = nSize;
        *((unsigned int*)(mSendBuffer.data() + 4)) = type;
    }

    if (mNetwork->Send(mSendBuffer.data(), nSize) == false)
    {
        strcpy(mErrorStr, mNetwork->GetErrorString());
        return false;
    }

    return true;
} // SendString


bool CRTProtocol::SendCommand(const char* cmdStr)
{
    return SendString(cmdStr, CRTPacket::PacketCommand);
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
                const auto commandResponseArr = mRTPacket->GetCommandString();
                commandResponseStr = (commandResponseArr != nullptr ? std::string(commandResponseArr) : "");
                return true;
            }
            if (type == CRTPacket::PacketError)
            {
                const auto commandResponseArr = mRTPacket->GetErrorString();
                commandResponseStr = (commandResponseArr != nullptr ? std::string(commandResponseArr) : "");
                strcpy(mErrorStr, commandResponseStr.c_str());
                return false;
            }
        }
    }
    else
    {
        std::string tmpStr;
        tmpStr = std::string(mErrorStr);
        sprintf(mErrorStr, "\'%s\' command failed. %s", cmdStr.c_str(), tmpStr.c_str());
    }
    commandResponseStr.clear();
    return false;
} // SendCommand


bool CRTProtocol::SendXML(const char* cmdStr)
{
    CRTPacket::EPacketType type;

    if (SendString(cmdStr, CRTPacket::PacketXML))
    {
        if (Receive(type, true) == CNetwork::ResponseType::success)
        {
            if (type == CRTPacket::PacketCommand)
            {
                if (strcmp(mRTPacket->GetCommandString(), "Setting parameters succeeded") == 0)
                {
                    return true;
                }
                else
                {
                    sprintf(mErrorStr,
                        "Expected command response \"Setting parameters succeeded\". Got \"%s\".",
                        mRTPacket->GetCommandString());
                }
            }
            else if (type == CRTPacket::PacketError)
            {
                strcpy(mErrorStr, mRTPacket->GetErrorString());
            }
            else
            {
                sprintf(mErrorStr, "Expected command response packet. Got packet type %d.", (int)type);
            }
        }
        else
        {        
            strcpy(mErrorStr, "Missing command response packet.");
        }
    }
    else
    {
        char pTmpStr[256];
        strcpy(pTmpStr, mErrorStr);
        sprintf(mErrorStr, "Failed to send XML string. %s", pTmpStr);
    }
    return false;
} // SendXML


