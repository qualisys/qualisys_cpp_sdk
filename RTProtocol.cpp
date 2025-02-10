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
#include <cstring>

#include "Network.h"
#include "SettingsDeserializer.h"
#include "SettingsSerializer.h"
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
    return mGeneralSettings.nCaptureFrequency;
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

bool CRTProtocol::Connect(const char* pServerAddr, unsigned short nPort, unsigned short* pnUDPServerPort,
                          int nMajorVersion, int nMinorVersion, bool bBigEndian, bool bNegotiateVersion)
{
    CRTPacket::EPacketType eType;
    std::string            tempStr;
    std::string            responseStr;

    mBigEndian = bBigEndian;
    mIsMaster = false;
    mMajorVersion = 1;
    if ((nMajorVersion == 1) && (nMinorVersion == 0))
    {
        mMinorVersion = 0;
    }
    else
    {
        mMinorVersion = 1;
        if (mBigEndian)
        {
            nPort += 2;
        }
        else
        {
            nPort += 1;
        }
    }

    if (mRTPacket)
    {
        delete mRTPacket;
    }

    mRTPacket = new CRTPacket(nMajorVersion, nMinorVersion, bBigEndian);

    if (mRTPacket == nullptr)
    {
        strcpy(mErrorStr, "Could not allocate data packet.");
        return false;
    }

    if (mNetwork->Connect(pServerAddr, nPort))
    {
        if (pnUDPServerPort != nullptr)
        {
            if (mNetwork->CreateUDPSocket(*pnUDPServerPort) == false)
            {
                sprintf(mErrorStr, "CreateUDPSocket failed. %s", mNetwork->GetErrorString());
                Disconnect();
                return false;
            }
        }

        // Welcome message
        if (Receive(eType, true) == CNetwork::ResponseType::success)
        {
            if (eType == CRTPacket::PacketError)
            {
                strcpy(mErrorStr, mRTPacket->GetErrorString());
                Disconnect();
                return false;
            }
            else if (eType == CRTPacket::PacketCommand)
            {
                const std::string welcomeMessage("QTM RT Interface connected");
                if (strncmp(welcomeMessage.c_str(), mRTPacket->GetCommandString(), welcomeMessage.size()) == 0)
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


bool CRTProtocol::SetVersion(int nMajorVersion, int nMinorVersion)
{
    std::string responseStr;
    std::string tempStr = "Version " + std::to_string(nMajorVersion) + "." + std::to_string(nMinorVersion);

    if (SendCommand(tempStr, responseStr))
    {
        tempStr = "Version set to " + std::to_string(nMajorVersion) + "." + std::to_string(nMinorVersion);

        if (responseStr == tempStr)
        {
            mMajorVersion = nMajorVersion;
            mMinorVersion = nMinorVersion;
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


bool CRTProtocol::GetVersion(unsigned int &nMajorVersion, unsigned int &nMinorVersion)
{
    if (!Connected())
    {
        return false;
    }

    nMajorVersion = mMajorVersion;
    nMinorVersion = mMinorVersion;

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


bool CRTProtocol::GetByteOrder(bool &bBigEndian)
{
    std::string responseStr;

    if (SendCommand("ByteOrder", responseStr))
    {
        bBigEndian = (responseStr == "Byte order is big endian");
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


bool CRTProtocol::DiscoverRTServer(unsigned short nServerPort, bool bNoLocalResponses, unsigned short nDiscoverPort)
{
    char data[10];
    SDiscoverResponse sResponse;        

    if (mBroadcastPort == 0)
    {
        if (!mNetwork->CreateUDPSocket(nServerPort, true))
        {
            strcpy(mErrorStr, mNetwork->GetErrorString());
            return false;
        }
        mBroadcastPort = nServerPort;
    }
    else
    {
        nServerPort = mBroadcastPort;
    }

    *((unsigned int*)data)         = (unsigned int)10;
    *((unsigned int*)(data + 4))   = (unsigned int)CRTPacket::PacketDiscover;
    *((unsigned short*)(data + 8)) = htons(nServerPort);

    if (mNetwork->SendUDPBroadcast(data, 10, nDiscoverPort))
    {
        mDiscoverResponseList.clear();

        CNetwork::Response response(CNetwork::ResponseType::error, 0);

        do 
        {
            unsigned int nAddr = 0;
            response = mNetwork->ReceiveUdpBroadcast(mDataBuff.data(), (int)mDataBuff.size(), 100000, &nAddr);

            if (response && response.received > qtmPacketHeaderSize)
            {
                if (CRTPacket::GetType(mDataBuff.data()) == CRTPacket::PacketCommand)
                {
                    char* discoverResponse  = CRTPacket::GetCommandString(mDataBuff.data());
                    sResponse.nAddr = nAddr;
                    sResponse.nBasePort = CRTPacket::GetDiscoverResponseBasePort(mDataBuff.data());

                    if (discoverResponse && (!bNoLocalResponses || !mNetwork->IsLocalAddress(nAddr)))
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


bool CRTProtocol::GetDiscoverResponse(unsigned int nIndex, unsigned int &nAddr, unsigned short &nBasePort, std::string& message)
{
    if (nIndex < mDiscoverResponseList.size())
    {
        nAddr     = mDiscoverResponseList[nIndex].nAddr;
        nBasePort = mDiscoverResponseList[nIndex].nBasePort;
        message   = mDiscoverResponseList[nIndex].message;
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


bool CRTProtocol::GetCurrentFrame(unsigned int nComponentType, const SComponentOptions& componentOptions)
{
    std::string components;

    if (GetComponentString(components, nComponentType, componentOptions))
    {
        return GetCurrentFrame(components);
    }
    else
    {
        strcpy(mErrorStr, "DataComponent missing.");
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
            strcpy(mErrorStr, "UDP address string too long.");
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


bool CRTProtocol::GetState(CRTPacket::EEvent &eEvent, bool bUpdate, int nTimeout)
{
    CRTPacket::EPacketType eType;

    if (bUpdate)
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
                response = Receive(eType, false, nTimeout);
                if (response == CNetwork::ResponseType::success)
                {
                    if (mRTPacket->GetEvent(eEvent))
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
        eEvent = mState;
        return true;
    }
    return false;
}


bool CRTProtocol::GetCapture(const char* pFileName, bool bC3D)
{
    CRTPacket::EPacketType eType;
    std::string            responseStr;

    mFileBuffer = fopen(pFileName, "wb");
    if (mFileBuffer != nullptr)
    {
        if (bC3D)
        {
            // C3D file
            if (SendCommand((mMajorVersion > 1 || mMinorVersion > 7) ? "GetCaptureC3D" : "GetCapture", responseStr))
            {
                if (responseStr == "Sending capture")
                {
                    if (Receive(eType, true, 5000000) == CNetwork::ResponseType::success) // Wait for C3D file in 5 seconds.
                    {
                        if (eType == CRTPacket::PacketC3DFile)
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
                    if (Receive(eType, true, 5000000) == CNetwork::ResponseType::success) // Wait for QTM file in 5 seconds.
                    {
                        if (eType == CRTPacket::PacketQTMFile)
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
                        char tmp[sizeof(mErrorStr)];
                        std::memcpy(tmp, mErrorStr, sizeof(mErrorStr));
                        (void)snprintf(mErrorStr, sizeof(mErrorStr), "No packet received. %s.", tmp);
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


CNetwork::ResponseType CRTProtocol::Receive(CRTPacket::EPacketType &eType, bool bSkipEvents, int nTimeout)
{
    CNetwork::Response response(CNetwork::ResponseType::error, 0);
    unsigned int nRecvedTotal = 0;
    unsigned int nFrameSize;

    eType = CRTPacket::PacketNone;

    do 
    {
        nRecvedTotal = 0;

        response = mNetwork->Receive(mDataBuff.data(), (int)mDataBuff.size(), true, nTimeout);

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

        bool bBigEndian = (mBigEndian || (mMajorVersion == 1 && mMinorVersion == 0));
        nFrameSize = mRTPacket->GetSize(mDataBuff.data(), bBigEndian);
        eType      = mRTPacket->GetType(mDataBuff.data(), bBigEndian);
        
        unsigned int nReadSize;

        if (eType == CRTPacket::PacketC3DFile || eType == CRTPacket::PacketQTMFile)
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
    } while (bSkipEvents && eType == CRTPacket::PacketEvent);
    
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
    CRTPacket::EPacketType eType;

    mAnalogDeviceSettings.clear();
    auto sendStr = std::string("GetParameters ") + settingsType;
    if (!SendCommand(sendStr.c_str()))
    {
        sprintf(mErrorStr, "GetParameters %s failed", settingsType.c_str());
        return nullptr;
    }

retry:
    auto received = Receive(eType, true);

    if (received == CNetwork::ResponseType::timeout)
    {
        strcat(mErrorStr, " Expected XML packet.");
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
            sprintf(mErrorStr, "%s.", mRTPacket->GetErrorString());
            return nullptr;
        }
        else
        {
            goto retry;
            //sprintf(mErrorStr, "GetParameters %s returned wrong packet type. Got type %d expected type 2.", settingsType.c_str(), eType);
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
    std::string             str;

    mGeneralSettings.vsCameras.clear();

    const char* data = ReadSettings("General");

    if (!data)
    {
        return false;
    }

    SettingsDeserializer serializer(data, mMajorVersion, mMinorVersion);

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

bool CRTProtocol::Read3DSettings(bool& bDataAvailable)
{
    bDataAvailable = false;
    m3DSettings.s3DLabels.clear();
    m3DSettings.pCalibrationTime[0] = 0;

    const char* data = ReadSettings("3D");
    if (!data)
    {
        return false;
    }

    SettingsDeserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.Deserialize3DSettings(m3DSettings, bDataAvailable);
}

bool CRTProtocol::Read6DOFSettings(bool& bDataAvailable)
{
    m6DOFSettings.clear();

    const auto* data = ReadSettings("6D");
    if(!data)
    {
        return false;
    }

    SettingsDeserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.Deserialize6DOFSettings(m6DOFSettings, mGeneralSettings, bDataAvailable);
}

bool CRTProtocol::ReadGazeVectorSettings(bool& bDataAvailable)
{
    bDataAvailable = false;

    mGazeVectorSettings.clear();

    const auto* data = ReadSettings("GazeVector");
    if(!data)
    {
        return false;
    }

    SettingsDeserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.DeserializeGazeVectorSettings(mGazeVectorSettings, bDataAvailable);
}

bool CRTProtocol::ReadEyeTrackerSettings(bool& bDataAvailable)
{
    bDataAvailable = false;

    mEyeTrackerSettings.clear();

    const auto* data = ReadSettings("EyeTracker");
    if(!data)
    {
        return false;
    }

    SettingsDeserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.DeserializeEyeTrackerSettings(mEyeTrackerSettings, bDataAvailable);
}

bool CRTProtocol::ReadAnalogSettings(bool& bDataAvailable)
{
    bDataAvailable = false;

    mAnalogDeviceSettings.clear();

    const auto* data = ReadSettings("Analog");
    if(!data)
    {
        return false;
    }
    SettingsDeserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.DeserializeAnalogSettings(mAnalogDeviceSettings, bDataAvailable);
}

bool CRTProtocol::ReadForceSettings(bool& bDataAvailable)
{
    bDataAvailable = false;

    mForceSettings.vsForcePlates.clear();

    const auto* data = ReadSettings("Force");
    if(!data)
    {
        return false;
    }

    SettingsDeserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.DeserializeForceSettings(mForceSettings, bDataAvailable);
}

bool CRTProtocol::ReadImageSettings(bool& bDataAvailable)
{
    bDataAvailable = false;

    mImageSettings.clear();

    const auto* data = ReadSettings("Image");
    if(!data)
    {
        return false;
    }

    SettingsDeserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.DeserializeImageSettings(mImageSettings, bDataAvailable);
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

    SettingsDeserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.DeserializeSkeletonSettings(skeletonGlobalData, mSkeletonSettingsHierarchical, mSkeletonSettings, bDataAvailable);
}

bool CRTProtocol::ReceiveCalibrationSettings(int timeout)
{
    CRTPacket::EPacketType  eType;
    std::string             str;
    SCalibration            settings;
    CNetwork::ResponseType  response;
    CRTPacket::EEvent       event = CRTPacket::EventNone;

    do 
    {
        response = Receive(eType, false, timeout);

        if (response == CNetwork::ResponseType::timeout)
        {
            strcat(mErrorStr, " Expected XML packet.");
            return false;
        }
        if (response == CNetwork::ResponseType::error)
        {
            return false;
        }

        if (eType == CRTPacket::PacketEvent)
        {
            mRTPacket->GetEvent(event);
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
            sprintf(mErrorStr, "Calibration aborted.");
        }
        else if (eType == CRTPacket::PacketError)
        {
            sprintf(mErrorStr, "%s.", mRTPacket->GetErrorString());
        }
        else
        {
            sprintf(mErrorStr, "GetParameters Calibration returned wrong packet type. Got type %d expected type 2.", eType);
        }
        return false;
    }

    auto data = mRTPacket->GetXMLString();
    SettingsDeserializer deserializer(data, mMajorVersion, mMinorVersion);
    return deserializer.DeserializeCalibrationSettings(mCalibrationSettings);
} // ReadCalibrationSettings

void CRTProtocol::Get3DSettings(EAxis& axisUpwards, std::string& calibrationTime, std::vector<SSettings3DLabel>& labels3D, std::vector<SSettingsBone>& bones)
{
    axisUpwards = m3DSettings.eAxisUpwards;
    calibrationTime = static_cast<std::string>(m3DSettings.pCalibrationTime);

    labels3D = m3DSettings.s3DLabels;
    bones = m3DSettings.sBones;
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
    unsigned int       &nCaptureFrequency, float &fCaptureTime,
    bool& bStartOnExtTrig, bool& startOnTrigNO, bool& startOnTrigNC, bool& startOnTrigSoftware,
    EProcessingActions &eProcessingActions, EProcessingActions &eRtProcessingActions, EProcessingActions &eReprocessingActions) const
{
    nCaptureFrequency = mGeneralSettings.nCaptureFrequency;
    fCaptureTime = mGeneralSettings.fCaptureTime;
    bStartOnExtTrig = mGeneralSettings.bStartOnExternalTrigger;
    startOnTrigNO = mGeneralSettings.bStartOnTrigNO;
    startOnTrigNC = mGeneralSettings.bStartOnTrigNC;
    startOnTrigSoftware = mGeneralSettings.bStartOnTrigSoftware;
    eProcessingActions = mGeneralSettings.eProcessingActions;
    eRtProcessingActions = mGeneralSettings.eRtProcessingActions;
    eReprocessingActions = mGeneralSettings.eReprocessingActions;
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
    bEnabled            = mGeneralSettings.sExternalTimebase.bEnabled;
    eSignalSource       = mGeneralSettings.sExternalTimebase.eSignalSource;
    bSignalModePeriodic = mGeneralSettings.sExternalTimebase.bSignalModePeriodic;
    nFreqMultiplier     = mGeneralSettings.sExternalTimebase.nFreqMultiplier;
    nFreqDivisor        = mGeneralSettings.sExternalTimebase.nFreqDivisor;
    nFreqTolerance      = mGeneralSettings.sExternalTimebase.nFreqTolerance;
    fNominalFrequency   = mGeneralSettings.sExternalTimebase.fNominalFrequency;
    bNegativeEdge       = mGeneralSettings.sExternalTimebase.bNegativeEdge;
    nSignalShutterDelay = mGeneralSettings.sExternalTimebase.nSignalShutterDelay;
    fNonPeriodicTimeout = mGeneralSettings.sExternalTimebase.fNonPeriodicTimeout;
}

void CRTProtocol::GetExtTimestampSettings(SSettingsGeneralExternalTimestamp& timestamp) const
{
    timestamp = mGeneralSettings.sTimestamp;
}

void CRTProtocol::GetEulerAngles(std::string& first, std::string& second, std::string& third) const
{
    first = mGeneralSettings.eulerRotations[0];
    second = mGeneralSettings.eulerRotations[1];
    third = mGeneralSettings.eulerRotations[2];
}

unsigned int CRTProtocol::GetCameraCount() const
{
    return (unsigned int)mGeneralSettings.vsCameras.size();
}

std::vector<CRTProtocol::SSettingsGeneralCamera> CRTProtocol::GetDevices() const
{
    return mGeneralSettings.vsCameras;
}


bool CRTProtocol::GetCameraSettings(
    unsigned int nCameraIndex, unsigned int &nID,     ECameraModel &eModel,
    bool         &bUnderwater, bool &bSupportsHwSync, unsigned int &nSerial, ECameraMode  &eMode) const
{
    if (nCameraIndex < mGeneralSettings.vsCameras.size())
    {
        nID             = mGeneralSettings.vsCameras[nCameraIndex].nID;
        eModel          = mGeneralSettings.vsCameras[nCameraIndex].eModel;
        bUnderwater     = mGeneralSettings.vsCameras[nCameraIndex].bUnderwater;
        bSupportsHwSync = mGeneralSettings.vsCameras[nCameraIndex].bSupportsHwSync;
        nSerial         = mGeneralSettings.vsCameras[nCameraIndex].nSerial;
        eMode           = mGeneralSettings.vsCameras[nCameraIndex].eMode;
        return true;
    }
    return false;
}


bool CRTProtocol::GetCameraMarkerSettings(
    unsigned int nCameraIndex,   unsigned int &nCurrentExposure, unsigned int &nMinExposure,
    unsigned int &nMaxExposure,  unsigned int &nCurrentThreshold,
    unsigned int &nMinThreshold, unsigned int &nMaxThreshold) const
{
    if (nCameraIndex < mGeneralSettings.vsCameras.size())
    {
        nCurrentExposure  = mGeneralSettings.vsCameras[nCameraIndex].nMarkerExposure;
        nMinExposure      = mGeneralSettings.vsCameras[nCameraIndex].nMarkerExposureMin;
        nMaxExposure      = mGeneralSettings.vsCameras[nCameraIndex].nMarkerExposureMax;
        nCurrentThreshold = mGeneralSettings.vsCameras[nCameraIndex].nMarkerThreshold;
        nMinThreshold     = mGeneralSettings.vsCameras[nCameraIndex].nMarkerThresholdMin;
        nMaxThreshold     = mGeneralSettings.vsCameras[nCameraIndex].nMarkerThresholdMax;
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
    if (nCameraIndex < mGeneralSettings.vsCameras.size())
    {
        eVideoResolution   = mGeneralSettings.vsCameras[nCameraIndex].eVideoResolution;
        eVideoAspectRatio  = mGeneralSettings.vsCameras[nCameraIndex].eVideoAspectRatio;
        nVideoFrequency   = mGeneralSettings.vsCameras[nCameraIndex].nVideoFrequency;
        nCurrentExposure  = mGeneralSettings.vsCameras[nCameraIndex].nVideoExposure;
        nMinExposure      = mGeneralSettings.vsCameras[nCameraIndex].nVideoExposureMin;
        nMaxExposure      = mGeneralSettings.vsCameras[nCameraIndex].nVideoExposureMax;
        nCurrentFlashTime = mGeneralSettings.vsCameras[nCameraIndex].nVideoFlashTime;
        nMinFlashTime     = mGeneralSettings.vsCameras[nCameraIndex].nVideoFlashTimeMin;
        nMaxFlashTime     = mGeneralSettings.vsCameras[nCameraIndex].nVideoFlashTimeMax;
        return true;
    }
    return false;
}


bool CRTProtocol::GetCameraSyncOutSettings(
    unsigned int nCameraIndex, unsigned int portNumber, ESyncOutFreqMode &eSyncOutMode,
    unsigned int &nSyncOutValue, float      &fSyncOutDutyCycle,
    bool         &bSyncOutNegativePolarity) const
{
    if (nCameraIndex < mGeneralSettings.vsCameras.size())
    {
        if (portNumber == 1 || portNumber == 2)
        {
            eSyncOutMode = mGeneralSettings.vsCameras[nCameraIndex].eSyncOutMode[portNumber - 1];
            nSyncOutValue = mGeneralSettings.vsCameras[nCameraIndex].nSyncOutValue[portNumber - 1];
            fSyncOutDutyCycle = mGeneralSettings.vsCameras[nCameraIndex].fSyncOutDutyCycle[portNumber - 1];
        }
        if (portNumber > 0 && portNumber < 4)
        {
            bSyncOutNegativePolarity = mGeneralSettings.vsCameras[nCameraIndex].bSyncOutNegativePolarity[portNumber - 1];
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
    unsigned int nCameraIndex, SPoint &point, float fvRotationMatrix[3][3]) const
{
    if (nCameraIndex < mGeneralSettings.vsCameras.size())
    {
        point.fX = mGeneralSettings.vsCameras[nCameraIndex].fPositionX;
        point.fY = mGeneralSettings.vsCameras[nCameraIndex].fPositionY;
        point.fZ = mGeneralSettings.vsCameras[nCameraIndex].fPositionZ;
        memcpy(fvRotationMatrix, mGeneralSettings.vsCameras[nCameraIndex].fPositionRotMatrix, 9 * sizeof(float));
        return true;
    }
    return false;
}


bool CRTProtocol::GetCameraOrientation(
    unsigned int nCameraIndex, int &nOrientation) const
{
    if (nCameraIndex < mGeneralSettings.vsCameras.size())
    {
        nOrientation = mGeneralSettings.vsCameras[nCameraIndex].nOrientation;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraResolution(
    unsigned int nCameraIndex, unsigned int &nMarkerWidth, unsigned int &nMarkerHeight,
    unsigned int &nVideoWidth, unsigned int &nVideoHeight) const
{
    if (nCameraIndex < mGeneralSettings.vsCameras.size())
    {
        nMarkerWidth  = mGeneralSettings.vsCameras[nCameraIndex].nMarkerResolutionWidth;
        nMarkerHeight = mGeneralSettings.vsCameras[nCameraIndex].nMarkerResolutionHeight;
        nVideoWidth   = mGeneralSettings.vsCameras[nCameraIndex].nVideoResolutionWidth;
        nVideoHeight  = mGeneralSettings.vsCameras[nCameraIndex].nVideoResolutionHeight;
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
    if (nCameraIndex < mGeneralSettings.vsCameras.size())
    {
        nMarkerLeft   = mGeneralSettings.vsCameras[nCameraIndex].nMarkerFOVLeft;
        nMarkerTop    = mGeneralSettings.vsCameras[nCameraIndex].nMarkerFOVTop;
        nMarkerRight  = mGeneralSettings.vsCameras[nCameraIndex].nMarkerFOVRight;
        nMarkerBottom = mGeneralSettings.vsCameras[nCameraIndex].nMarkerFOVBottom;
        nVideoLeft    = mGeneralSettings.vsCameras[nCameraIndex].nVideoFOVLeft;
        nVideoTop     = mGeneralSettings.vsCameras[nCameraIndex].nVideoFOVTop;
        nVideoRight   = mGeneralSettings.vsCameras[nCameraIndex].nVideoFOVRight;
        nVideoBottom  = mGeneralSettings.vsCameras[nCameraIndex].nVideoFOVBottom;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraLensControlSettings(const unsigned int nCameraIndex, float* focus, float* aperture) const
{
    if (nCameraIndex < mGeneralSettings.vsCameras.size())
    {
        *focus = mGeneralSettings.vsCameras[nCameraIndex].fFocus;
        if (std::isnan(*focus))
            return false;
        *aperture = mGeneralSettings.vsCameras[nCameraIndex].fAperture;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraAutoExposureSettings(const unsigned int nCameraIndex, bool* autoExposureEnabled, float* autoExposureCompensation) const
{
    if (nCameraIndex < mGeneralSettings.vsCameras.size())
    {
        *autoExposureCompensation = mGeneralSettings.vsCameras[nCameraIndex].autoExposureCompensation;
        if (std::isnan(*autoExposureCompensation))
            return false;
        *autoExposureEnabled = mGeneralSettings.vsCameras[nCameraIndex].autoExposureEnabled;
        return true;
    }
    return false;
}

bool CRTProtocol::GetCameraAutoWhiteBalance(const unsigned int nCameraIndex, bool* autoWhiteBalanceEnabled) const
{
    if (nCameraIndex < mGeneralSettings.vsCameras.size() && mGeneralSettings.vsCameras[nCameraIndex].autoWhiteBalance >= 0)
    {
        *autoWhiteBalanceEnabled = mGeneralSettings.vsCameras[nCameraIndex].autoWhiteBalance == 1;
        return true;
    }
    return false;
}

CRTProtocol::EAxis CRTProtocol::Get3DUpwardAxis() const
{
    return m3DSettings.eAxisUpwards;
}

const char* CRTProtocol::Get3DCalibrated() const
{
    return m3DSettings.pCalibrationTime;
}

unsigned int CRTProtocol::Get3DLabeledMarkerCount() const
{
    return (unsigned int)m3DSettings.s3DLabels.size();
}

const char* CRTProtocol::Get3DLabelName(unsigned int nMarkerIndex) const
{
    if (nMarkerIndex < m3DSettings.s3DLabels.size())
    {
        return m3DSettings.s3DLabels[nMarkerIndex].oName.c_str();
    }
    return nullptr;
}

unsigned int CRTProtocol::Get3DLabelColor(unsigned int nMarkerIndex) const
{
    if (nMarkerIndex < m3DSettings.s3DLabels.size())
    {
        return m3DSettings.s3DLabels[nMarkerIndex].nRGBColor;
    }
    return 0;
}

const char* CRTProtocol::Get3DTrajectoryType(unsigned int nMarkerIndex) const
{
    if (nMarkerIndex < m3DSettings.s3DLabels.size())
    {
        return m3DSettings.s3DLabels[nMarkerIndex].type.c_str();
    }
    return 0;
}


unsigned int CRTProtocol::Get3DBoneCount() const
{
    return (unsigned int)m3DSettings.sBones.size();
}

const char* CRTProtocol::Get3DBoneFromName(unsigned int boneIndex) const
{
    if (boneIndex < m3DSettings.sBones.size())
    {
        return m3DSettings.sBones[boneIndex].fromName.c_str();
    }
    return nullptr;
}

const char* CRTProtocol::Get3DBoneToName(unsigned int boneIndex) const
{
    if (boneIndex < m3DSettings.sBones.size())
    {
        return m3DSettings.sBones[boneIndex].toName.c_str();
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


bool CRTProtocol::Get6DOFBodyPoint(unsigned int nBodyIndex, unsigned int nMarkerIndex, SPoint &point) const
{
    if (nBodyIndex < m6DOFSettings.size())
    {
        if (nMarkerIndex < m6DOFSettings.at(nBodyIndex).points.size())
        {
            point.fX = m6DOFSettings.at(nBodyIndex).points[nMarkerIndex].fX;
            point.fY = m6DOFSettings.at(nBodyIndex).points[nMarkerIndex].fY;
            point.fZ = m6DOFSettings.at(nBodyIndex).points[nMarkerIndex].fZ;
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

const char* CRTProtocol::GetGazeVectorName(unsigned int nGazeVectorIndex) const
{
    if (nGazeVectorIndex < mGazeVectorSettings.size())
    {
        return mGazeVectorSettings[nGazeVectorIndex].name.c_str();
    }
    return nullptr;
}

float CRTProtocol::GetGazeVectorFrequency(unsigned int nGazeVectorIndex) const
{
    if (nGazeVectorIndex < mGazeVectorSettings.size())
    {
        return mGazeVectorSettings[nGazeVectorIndex].frequency;
    }
    return 0;
}

bool CRTProtocol::GetGazeVectorHardwareSyncUsed(unsigned int nGazeVectorIndex) const
{
    if (nGazeVectorIndex < mGazeVectorSettings.size())
    {
        return mGazeVectorSettings[nGazeVectorIndex].hwSync;
    }
    return false;
}

bool CRTProtocol::GetGazeVectorFilterUsed(unsigned int nGazeVectorIndex) const
{
    if (nGazeVectorIndex < mGazeVectorSettings.size())
    {
        return mGazeVectorSettings[nGazeVectorIndex].filter;
    }
    return false;
}


unsigned int CRTProtocol::GetEyeTrackerCount() const
{
    return (unsigned int)mEyeTrackerSettings.size();
}

const char* CRTProtocol::GetEyeTrackerName(unsigned int nEyeTrackerIndex) const
{
    if (nEyeTrackerIndex < mEyeTrackerSettings.size())
    {
        return mEyeTrackerSettings[nEyeTrackerIndex].name.c_str();
    }
    return nullptr;
}

float CRTProtocol::GetEyeTrackerFrequency(unsigned int nEyeTrackerIndex) const
{
    if (nEyeTrackerIndex < mEyeTrackerSettings.size())
    {
        return mEyeTrackerSettings[nEyeTrackerIndex].frequency;
    }
    return 0;
}

bool CRTProtocol::GetEyeTrackerHardwareSyncUsed(unsigned int nEyeTrackerIndex) const
{
    if (nEyeTrackerIndex < mEyeTrackerSettings.size())
    {
        return mEyeTrackerSettings[nEyeTrackerIndex].hwSync;
    }
    return false;
}


unsigned int CRTProtocol::GetAnalogDeviceCount() const
{
    return (unsigned int)mAnalogDeviceSettings.size();
}


bool CRTProtocol::GetAnalogDevice(unsigned int nDeviceIndex, unsigned int &nDeviceID, unsigned int &nChannels,
                                  char* &pName, unsigned int &nFrequency, char* &pUnit,
                                  float &fMinRange, float &fMaxRange) const
{
    if (nDeviceIndex < mAnalogDeviceSettings.size())
    {
        nDeviceID  = mAnalogDeviceSettings.at(nDeviceIndex).nDeviceID;
        pName      = (char*)mAnalogDeviceSettings.at(nDeviceIndex).oName.c_str();
        nChannels  = mAnalogDeviceSettings.at(nDeviceIndex).nChannels;
        nFrequency = mAnalogDeviceSettings.at(nDeviceIndex).nFrequency;
        pUnit      = (char*)mAnalogDeviceSettings.at(nDeviceIndex).oUnit.c_str();
        fMinRange  = mAnalogDeviceSettings.at(nDeviceIndex).fMinRange;
        fMaxRange  = mAnalogDeviceSettings.at(nDeviceIndex).fMaxRange;

        return true;
    }
    return false;
}


const char* CRTProtocol::GetAnalogLabel(unsigned int nDeviceIndex, unsigned int nChannelIndex) const
{
    if (nDeviceIndex < mAnalogDeviceSettings.size())
    {
        if (nChannelIndex < mAnalogDeviceSettings.at(nDeviceIndex).voLabels.size())
        {
            return mAnalogDeviceSettings.at(nDeviceIndex).voLabels.at(nChannelIndex).c_str();
        }
    }
    return nullptr;
}


const char* CRTProtocol::GetAnalogUnit(unsigned int nDeviceIndex, unsigned int nChannelIndex) const
{
    if (nDeviceIndex < mAnalogDeviceSettings.size())
    {
        if (nChannelIndex < mAnalogDeviceSettings.at(nDeviceIndex).voUnits.size())
        {
            return mAnalogDeviceSettings.at(nDeviceIndex).voUnits.at(nChannelIndex).c_str();
        }
    }
    return nullptr;
}


void CRTProtocol::GetForceUnits(char* &pLength, char* &pForce) const
{
    pLength = (char*)mForceSettings.oUnitLength.c_str();
    pForce  = (char*)mForceSettings.oUnitForce.c_str();
}


unsigned int CRTProtocol::GetForcePlateCount() const
{
    return (unsigned int)mForceSettings.vsForcePlates.size();
}


bool CRTProtocol::GetForcePlate(unsigned int nPlateIndex, unsigned int &nID, unsigned int &nAnalogDeviceID,
                                unsigned int &nFrequency, char* &pType, char* &pName, float &fLength, float &fWidth) const
{
    if (nPlateIndex < mForceSettings.vsForcePlates.size())
    {
        nID             = mForceSettings.vsForcePlates[nPlateIndex].nID;
        nAnalogDeviceID = mForceSettings.vsForcePlates[nPlateIndex].nAnalogDeviceID;
        nFrequency      = mForceSettings.vsForcePlates[nPlateIndex].nFrequency;
        pType           = (char*)mForceSettings.vsForcePlates[nPlateIndex].oType.c_str();
        pName           = (char*)mForceSettings.vsForcePlates[nPlateIndex].oName.c_str();
        fLength         = mForceSettings.vsForcePlates[nPlateIndex].fLength;
        fWidth          = mForceSettings.vsForcePlates[nPlateIndex].fWidth;
        return true;
    }
    return false;
}


bool CRTProtocol::GetForcePlateLocation(unsigned int nPlateIndex, SPoint sCorner[4]) const
{
    if (nPlateIndex < mForceSettings.vsForcePlates.size())
    {
        memcpy(sCorner, mForceSettings.vsForcePlates[nPlateIndex].asCorner, 3 * 4 * sizeof(float));
        return true;
    }
    return false;
}


bool CRTProtocol::GetForcePlateOrigin(unsigned int nPlateIndex, SPoint &sOrigin) const
{
    if (nPlateIndex < mForceSettings.vsForcePlates.size())
    {
        sOrigin = mForceSettings.vsForcePlates[nPlateIndex].sOrigin;
        return true;
    }
    return false;
}


unsigned int CRTProtocol::GetForcePlateChannelCount(unsigned int nPlateIndex) const
{
    if (nPlateIndex < mForceSettings.vsForcePlates.size())
    {
        return (unsigned int)mForceSettings.vsForcePlates[nPlateIndex].vChannels.size();
    }
    return 0;
}


bool CRTProtocol::GetForcePlateChannel(unsigned int nPlateIndex, unsigned int nChannelIndex,
                                       unsigned int &nChannelNumber, float &fConversionFactor) const
{
    if (nPlateIndex < mForceSettings.vsForcePlates.size())
    {
        if (nChannelIndex < mForceSettings.vsForcePlates[nPlateIndex].vChannels.size())
        {
            nChannelNumber    = mForceSettings.vsForcePlates[nPlateIndex].vChannels[nChannelIndex].nChannelNumber;
            fConversionFactor = mForceSettings.vsForcePlates[nPlateIndex].vChannels[nChannelIndex].fConversionFactor;
            return true;
        }
    }
    return false;
}


bool CRTProtocol::GetForcePlateCalibrationMatrix(unsigned int nPlateIndex, float fvCalMatrix[12][12], unsigned int* rows, unsigned int* columns) const
{
    if (nPlateIndex < mForceSettings.vsForcePlates.size())
    {
        if (mForceSettings.vsForcePlates[nPlateIndex].bValidCalibrationMatrix)
        {
            *rows = mForceSettings.vsForcePlates[nPlateIndex].nCalibrationMatrixRows;
            *columns = mForceSettings.vsForcePlates[nPlateIndex].nCalibrationMatrixColumns;
            memcpy(
                fvCalMatrix,
                mForceSettings.vsForcePlates[nPlateIndex].afCalibrationMatrix,
                mForceSettings.vsForcePlates[nPlateIndex].nCalibrationMatrixRows * mForceSettings.vsForcePlates[nPlateIndex].nCalibrationMatrixColumns * sizeof(float));
            return true;
        }
    }
    return false;
}


unsigned int CRTProtocol::GetImageCameraCount() const
{
    return (unsigned int)mImageSettings.size();
}


bool CRTProtocol::GetImageCamera(unsigned int nCameraIndex, unsigned int &nCameraID, bool &bEnabled,
                                 CRTPacket::EImageFormat &eFormat, unsigned int &nWidth, unsigned int &nHeight,
                                 float &fCropLeft, float &fCropTop, float &fCropRight, float &fCropBottom) const
{
    if (nCameraIndex < mImageSettings.size())
    {
        nCameraID   = mImageSettings[nCameraIndex].nID;
        bEnabled    = mImageSettings[nCameraIndex].bEnabled;
        eFormat     = mImageSettings[nCameraIndex].eFormat;
        nWidth      = mImageSettings[nCameraIndex].nWidth;
        nHeight     = mImageSettings[nCameraIndex].nHeight;
        fCropLeft   = mImageSettings[nCameraIndex].fCropLeft;
        fCropTop    = mImageSettings[nCameraIndex].fCropTop;
        fCropRight  = mImageSettings[nCameraIndex].fCropRight;
        fCropBottom = mImageSettings[nCameraIndex].fCropBottom;
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
    SettingsSerializer serializer(mMajorVersion, mMinorVersion);
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
    SettingsSerializer serializer(mMajorVersion, mMinorVersion);
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
    SettingsSerializer serializer(mMajorVersion, mMinorVersion);
    auto message = serializer.SetExtTimestampSettings(timestampSettings);

    return SendXML(message.data());
}


// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraSettings(
    const unsigned int nCameraID,        const ECameraMode* peMode,
    const float*       pfMarkerExposure, const float*       pfMarkerThreshold,
    const int*         pnOrientation)
{
    SettingsSerializer serializer(mMajorVersion, mMinorVersion);
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
    SettingsSerializer serializer(mMajorVersion, mMinorVersion);
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
    SettingsSerializer serializer(mMajorVersion, mMinorVersion);
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
    SettingsSerializer serializer(mMajorVersion, mMinorVersion);
    auto message = serializer.SetCameraLensControlSettings(nCameraID, focus, aperture);
    return SendXML(message.data());

}

// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraAutoExposureSettings(const unsigned int nCameraID, const bool autoExposure, const float compensation)
{
    SettingsSerializer serializer(mMajorVersion, mMinorVersion);
    auto message = serializer.SetCameraAutoExposureSettings(nCameraID, autoExposure, compensation);
    return SendXML(message.data());
}

// nCameraID starts on 1. If nCameraID < 0 then settings are applied to all cameras.
bool CRTProtocol::SetCameraAutoWhiteBalance(const unsigned int nCameraID, const bool enable)
{
    SettingsSerializer serializer(mMajorVersion, mMinorVersion);
    auto message = serializer.SetCameraAutoWhiteBalance(nCameraID, enable);
    return SendXML(message.data());
}


bool CRTProtocol::SetImageSettings(
    const unsigned int  nCameraID, const bool*         pbEnable,    const CRTPacket::EImageFormat* peFormat,
    const unsigned int* pnWidth,   const unsigned int* pnHeight,    const float* pfLeftCrop,
    const float*        pfTopCrop, const float*        pfRightCrop, const float* pfBottomCrop)
{
    SettingsSerializer serializer (mMajorVersion, mMinorVersion);
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
        SettingsSerializer serializer(mMajorVersion, mMinorVersion);
        auto message = serializer.SetForceSettings(nPlateID, psCorner1, psCorner2,
            psCorner3, psCorner4);
        return SendXML(message.data());
    }
    else
    {
        sprintf(mErrorStr, "Illegal force plate id: %d.", nPlateID);
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

    SettingsSerializer serializer(mMajorVersion, mMinorVersion);
    auto message = serializer.Set6DOFBodySettings(settings);

    return SendXML(message.data());
}

bool CRTProtocol::SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& skeletons)
{
    SettingsSerializer serializer(mMajorVersion, mMinorVersion);
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


bool CRTProtocol::SendString(const char* pCmdStr, int nType)
{
    std::uint32_t nCmdStrLen = (int)strlen(pCmdStr);
    std::uint32_t nSize = 8 + nCmdStrLen + 1; // Header size + length of the string + terminating null char

    if (nSize > mSendBuffer.size())
    {
        mSendBuffer.resize(nSize);
    }
    
    memcpy(mSendBuffer.data() + 8, pCmdStr, nCmdStrLen + 1);

    if ((mMajorVersion == 1 && mMinorVersion == 0) || mBigEndian)
    {
        *((unsigned int*)mSendBuffer.data())       = htonl(nSize);
        *((unsigned int*)(mSendBuffer.data() + 4)) = htonl(nType);
    }
    else
    {
        *((unsigned int*)mSendBuffer.data())       = nSize;
        *((unsigned int*)(mSendBuffer.data() + 4)) = nType;
    }

    if (mNetwork->Send(mSendBuffer.data(), nSize) == false)
    {
        strcpy(mErrorStr, mNetwork->GetErrorString());
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
                const auto commandResponseArr = mRTPacket->GetCommandString();
                commandResponseStr = (commandResponseArr != nullptr ? std::string(commandResponseArr) : "");
                return true;
            }
            if (eType == CRTPacket::PacketError)
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


bool CRTProtocol::SendXML(const char* pCmdStr)
{
    CRTPacket::EPacketType eType;

    if (SendString(pCmdStr, CRTPacket::PacketXML))
    {
        if (Receive(eType, true) == CNetwork::ResponseType::success)
        {
            if (eType == CRTPacket::PacketCommand)
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
            else if (eType == CRTPacket::PacketError)
            {
                strcpy(mErrorStr, mRTPacket->GetErrorString());
            }
            else
            {
                sprintf(mErrorStr, "Expected command response packet. Got packet type %d.", (int)eType);
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


