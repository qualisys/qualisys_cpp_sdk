#ifndef RTPROTOCOL_H
#define RTPROTOCOL_H


#include "RTPacket.h"
#include "Network.h"
#include "Settings.h"

#include <vector>
#include <string>
#include <map>
#include <limits>
#include <cmath>
#include <cstdint>
#include <memory>

#ifdef _WIN32
#pragma warning (disable : 4251)
#endif


#ifdef EXPORT_DLL
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

class CMarkup;

class DLL_EXPORT CRTProtocol
{
public:
    static const unsigned int cDefaultBasePort          = 22222;
    static const unsigned int cDefaultTelnetPort        = cDefaultBasePort - 1;
    static const unsigned int cDefaultLegacyPort        = cDefaultBasePort;     // Only supports version 1.0 of the protocol.
    static const unsigned int cDefaultLittleEndianPort  = cDefaultBasePort + 1;
    static const unsigned int cDefaultBigEndianPort     = cDefaultBasePort + 2;
    static const unsigned int cDefaultOscPort           = cDefaultBasePort + 3;
    static const unsigned int cDefaultAutoDiscoverPort  = cDefaultBasePort + 4;

    static const unsigned int cWaitForDataTimeout        = 5000000;   // 5 s
    static const unsigned int cWaitForSaveTimeout        = 30000000;  // 30 s
    static const unsigned int cWaitForCalibrationTimeout = 600000000; // 10 min

    static const unsigned int cComponent3d            = 0x000001;
    static const unsigned int cComponent3dNoLabels    = 0x000002;
    static const unsigned int cComponentAnalog        = 0x000004;
    static const unsigned int cComponentForce         = 0x000008;
    static const unsigned int cComponent6d            = 0x000010;
    static const unsigned int cComponent6dEuler       = 0x000020;
    static const unsigned int cComponent2d            = 0x000040;
    static const unsigned int cComponent2dLin         = 0x000080;
    static const unsigned int cComponent3dRes         = 0x000100;
    static const unsigned int cComponent3dNoLabelsRes = 0x000200;
    static const unsigned int cComponent6dRes         = 0x000400;
    static const unsigned int cComponent6dEulerRes    = 0x000800;
    static const unsigned int cComponentAnalogSingle  = 0x001000;
    static const unsigned int cComponentImage         = 0x002000;
    static const unsigned int cComponentForceSingle   = 0x004000;
    static const unsigned int cComponentGazeVector    = 0x008000;
    static const unsigned int cComponentTimecode      = 0x010000;
    static const unsigned int cComponentSkeleton      = 0x020000;
    static const unsigned int cComponentEyeTracker    = 0x040000;

    using SComponentOptions = CRTProtocolNs::SComponentOptions;
    using SPoint = CRTProtocolNs::SPoint;
    using SBodyPoint = CRTProtocolNs::SBodyPoint;
    using SDiscoverResponse = CRTProtocolNs::SDiscoverResponse;
    using SSettingsGeneralCamera = CRTProtocolNs::SSettingsGeneralCamera;
    using SSettingsGeneralExternalTimebase = CRTProtocolNs::SSettingsGeneralExternalTimebase;
    using SSettingsGeneralExternalTimestamp = CRTProtocolNs::SSettingsGeneralExternalTimestamp;
    using SSettingsGeneral = CRTProtocolNs::SSettingsGeneral;
    using SSettings3DLabel = CRTProtocolNs::SSettings3DLabel;
    using SSettingsBone = CRTProtocolNs::SSettingsBone;
    using SSettings3D = CRTProtocolNs::SSettings3D;
    using SSettings6DMesh = CRTProtocolNs::SSettings6DMesh;
    using SOrigin = CRTProtocolNs::SOrigin;
    using SSettings6DOFBody = CRTProtocolNs::SSettings6DOFBody;
    using SGazeVector = CRTProtocolNs::SGazeVector;
    using SEyeTracker = CRTProtocolNs::SEyeTracker;
    using SAnalogDevice = CRTProtocolNs::SAnalogDevice;
    using SForceChannel = CRTProtocolNs::SForceChannel;
    using SForcePlate = CRTProtocolNs::SForcePlate;
    using SSettingsForce = CRTProtocolNs::SSettingsForce;
    using SImageCamera = CRTProtocolNs::SImageCamera;
    using SCalibrationFov = CRTProtocolNs::SCalibrationFov;
    using SCalibrationTransform = CRTProtocolNs::SCalibrationTransform;
    using SCalibrationIntrinsic = CRTProtocolNs::SCalibrationIntrinsic;
    using SCalibrationCamera = CRTProtocolNs::SCalibrationCamera;
    using SCalibration = CRTProtocolNs::SCalibration;
    using SPosition = CRTProtocolNs::SPosition;
    using SRotation = CRTProtocolNs::SRotation;
    using SCoupling = CRTProtocolNs::SCoupling;
    using SDegreeOfFreedom = CRTProtocolNs::SDegreeOfFreedom;
    using SMarker = CRTProtocolNs::SMarker;
    using SBody = CRTProtocolNs::SBody;
    using SSettingsSkeletonSegmentHierarchical = CRTProtocolNs::SSettingsSkeletonSegmentHierarchical;
    using SSettingsSkeletonSegment = CRTProtocolNs::SSettingsSkeletonSegment;
    using SSettingsSkeletonHierarchical = CRTProtocolNs::SSettingsSkeletonHierarchical;
    using SSettingsSkeleton = CRTProtocolNs::SSettingsSkeleton;

    using EStreamRate = CRTProtocolNs::EStreamRate;
    using ECameraModel = CRTProtocolNs::ECameraModel;
    using ECameraMode = CRTProtocolNs::ECameraMode;
    using EVideoResolution = CRTProtocolNs::EVideoResolution;
    using EVideoAspectRatio = CRTProtocolNs::EVideoAspectRatio;
    using ESyncOutFreqMode = CRTProtocolNs::ESyncOutFreqMode;
    using ESignalSource = CRTProtocolNs::ESignalSource;
    using EAxis = CRTProtocolNs::EAxis;
    using EProcessingActions = CRTProtocolNs::EProcessingActions;
    using ECalibrationType = CRTProtocolNs::ECalibrationType;
    using EOriginType = CRTProtocolNs::EOriginType;
    using EDegreeOfFreedom = CRTProtocolNs::EDegreeOfFreedom;
    using ETimestampType = CRTProtocolNs::ETimestampType;

public:
    unsigned int GetSystemFrequency() const;

    CRTProtocol();
    ~CRTProtocol();

    bool       Connect(const char* pServerAddr, unsigned short nPort = cDefaultBasePort, unsigned short* pnUDPServerPort = nullptr,
                       int nMajorVersion = MAJOR_VERSION, int nMinorVersion = MINOR_VERSION, bool bBigEndian = false, bool bNegotiateVersion = true);
    unsigned short GetUdpServerPort();
    void       Disconnect();
    bool       Connected() const;
    bool       SetVersion(int nMajorVersion, int nMinorVersion);
    bool       GetVersion(unsigned int &nMajorVersion, unsigned int &nMinorVersion);
    bool       GetQTMVersion(std::string& verStr);
    bool       GetByteOrder(bool &bBigEndian);
    bool       CheckLicense(const std::string& licenseCode);
    bool       DiscoverRTServer(unsigned short nServerPort, bool bNoLocalResponses, unsigned short nDiscoverPort = cDefaultAutoDiscoverPort);
    int        GetNumberOfDiscoverResponses();
    bool       GetDiscoverResponse(unsigned int nIndex, unsigned int &nAddr, unsigned short &nBasePort, std::string& message);

    bool       GetCurrentFrame(const std::string& components);
    bool       GetCurrentFrame(unsigned int nComponentType, const SComponentOptions& componentOptions = { });
    bool       StreamFrames(unsigned int nComponentType);
    bool       StreamFrames(EStreamRate eRate, unsigned int nRateArg, unsigned short nUDPPort, const char* pUDPAddr, const char* components);
    bool       StreamFrames(EStreamRate eRate, unsigned int nRateArg, unsigned short nUDPPort, const char* pUDPAddr,
                            unsigned int nComponentType, const SComponentOptions& componentOptions = { });
    bool       StreamFramesStop();
    bool       GetState(CRTPacket::EEvent &eEvent, bool bUpdate = true, int nTimeout = cWaitForDataTimeout);
    bool       GetCapture(const char* pFileName, bool bC3D);
    bool       SendTrig();
    bool       SetQTMEvent(const std::string& label);
    bool       TakeControl(const std::string& password = "");
    bool       ReleaseControl();
    bool       IsControlling();
    bool       NewMeasurement();
    bool       CloseMeasurement();
    bool       StartCapture();
    bool       StartRTOnFile();
    bool       StopCapture();
    bool       Calibrate(const bool refine, SCalibration &calibrationResult, int timeout = cWaitForCalibrationTimeout);
    bool       LoadCapture(const std::string& fileName);
    bool       SaveCapture(const std::string& fileName, bool bOverwrite, std::string* pNewFileName = nullptr, int nSizeOfNewFileName = 0);
    bool       LoadProject(const std::string& fileName);
    bool       Reprocess();
    void       OverrideNetwork(INetwork* network);

    static double SMPTENormalizedSubFrame(unsigned int captureFrequency, unsigned int timestampFrequency, unsigned int subFrame);
    static bool GetEventString(CRTPacket::EEvent eEvent, char* pStr);
    static bool ConvertRateString(const char* pRate, EStreamRate &eRate, unsigned int &nRateArg);
    static unsigned int ConvertComponentString(const std::string& componentsString);
    static bool GetComponentString(std::string& componentStr, unsigned int nComponentType, const SComponentOptions& options = SComponentOptions());
    static std::vector<std::pair<unsigned int, std::string>> GetComponents(const std::string& componentsString);
    static bool GetComponent(std::string& componentStr, unsigned int& component, std::string& option);

    [[deprecated("Replaced by Receive.")]]
    int         ReceiveRTPacket(CRTPacket::EPacketType &eType, bool bSkipEvents = true, int nTimeout = cWaitForDataTimeout); // nTimeout < 0 : Blocking receive
    CNetwork::ResponseType Receive(CRTPacket::EPacketType &eType, bool bSkipEvents = true, int nTimeout = cWaitForDataTimeout); // nTimeout < 0 : Blocking receive
    

    CRTPacket* GetRTPacket();

    bool ReadGeneralSettings();
    [[deprecated("Replaced by ReadGeneralSettings.")]]
    bool ReadCameraSystemSettings(); // Same as ReadGeneralSettings
    bool ReadCalibrationSettings();
    bool Read3DSettings(bool &bDataAvailable);
    bool Read6DOFSettings(bool &bDataAvailable);
    bool ReadGazeVectorSettings(bool &bDataAvailable);
    bool ReadEyeTrackerSettings(bool &bDataAvailable);
    bool ReadAnalogSettings(bool &bDataAvailable);
    bool ReadForceSettings(bool &bDataAvailable);
    bool ReadImageSettings(bool &bDataAvailable);
    bool ReadSkeletonSettings(bool &bDataAvailable, bool skeletonGlobalData = false);


    void Get3DSettings(EAxis& axisUpwards, std::string& calibrationTime, std::vector<SSettings3DLabel>& labels3D, std::vector<SSettingsBone>& bones);
    void GetGazeVectorSettings(std::vector<SGazeVector>& gazeVectorSettings);
    void GetEyeTrackerSettings(std::vector<SEyeTracker>& eyeTrackerSettings);
    void GetAnalogSettings(std::vector<SAnalogDevice>& analogSettings);
    void GetForceSettings(SSettingsForce& forceSettings);
    void GetGeneralSettings(
        unsigned int &nCaptureFrequency, float &fCaptureTime,
        bool& bStartOnExtTrig, bool& trigNO, bool& trigNC, bool& trigSoftware,
        EProcessingActions &eProcessingActions, EProcessingActions &eRtProcessingActions, EProcessingActions &eReprocessingActions) const;
    [[deprecated("Replaced by GetGeneralSettings.")]]
    void GetSystemSettings(
        unsigned int &nCaptureFrequency, float &fCaptureTime,
        bool& bStartOnExtTrig, bool& trigNO, bool& trigNC, bool& trigSoftware,
        EProcessingActions &eProcessingActions, EProcessingActions &eRtProcessingActions, EProcessingActions &eReprocessingActions) const;

    void GetCalibrationSettings(SCalibration &calibrationSettings) const;

    void GetExtTimeBaseSettings(
        bool         &bEnabled,            ESignalSource &eSignalSource,
        bool         &bSignalModePeriodic, unsigned int  &nFreqMultiplier,
        unsigned int &nFreqDivisor,        unsigned int  &nFreqTolerance,
        float        &fNominalFrequency,   bool          &bNegativeEdge,
        unsigned int &nSignalShutterDelay, float         &fNonPeriodicTimeout) const;
    void GetExtTimestampSettings(SSettingsGeneralExternalTimestamp& timestamp) const;

    void GetEulerAngles(std::string& first, std::string& second, std::string& third) const;
    
    unsigned int GetCameraCount() const;
    std::vector<SSettingsGeneralCamera> GetDevices() const;

    bool GetCameraSettings(
        unsigned int nCameraIndex, unsigned int &nID,     ECameraModel &eModel, 
        bool         &bUnderwater, bool &bSupportsHwSync, unsigned int &nSerial, ECameraMode  &eMode) const;

    bool GetCameraMarkerSettings(
        unsigned int nCameraIndex,       unsigned int &nCurrentExposure,
        unsigned int &nMinExposure,      unsigned int &nMaxExposure,
        unsigned int &nCurrentThreshold, unsigned int &nMinThreshold,
        unsigned int &nMaxThreshold) const;

    bool GetCameraVideoSettings(
        unsigned int nCameraIndex,            EVideoResolution &eVideoResolution,
        EVideoAspectRatio &eVideoAspectRatio, unsigned int &nVideoFrequency,
        unsigned int &nCurrentExposure,       unsigned int &nMinExposure,
        unsigned int &nMaxExposure,           unsigned int &nCurrentFlashTime,
        unsigned int &nMinFlashTime,          unsigned int &nMaxFlashTime) const;

    bool GetCameraSyncOutSettings(
        unsigned int nCameraIndex,   unsigned int portNumber, ESyncOutFreqMode &eSyncOutMode,
        unsigned int &nSyncOutValue, float        &fSyncOutDutyCycle,
        bool         &bSyncOutNegativePolarity) const;

    bool GetCameraPosition(
        unsigned int nCameraIndex, SPoint &sPoint, float fvRotationMatrix[3][3]) const;

    bool GetCameraOrientation(
        unsigned int nCameraIndex, int &nOrientation) const;

    bool GetCameraResolution(
        unsigned int nCameraIndex,   unsigned int &nMarkerWidth,
        unsigned int &nMarkerHeight, unsigned int &nVideoWidth,
        unsigned int &nVideoHeight) const;

    bool GetCameraFOV(
        unsigned int nCameraIndex,  unsigned int &nMarkerLeft,  unsigned int &nMarkerTop,
        unsigned int &nMarkerRight, unsigned int &nMarkerBottom,
        unsigned int &nVideoLeft,   unsigned int &nVideoTop,
        unsigned int &nVideoRight,  unsigned int &nVideoBottom) const;

    bool GetCameraLensControlSettings(const unsigned int nCameraIndex, float* focus, float* aperture) const;
    bool GetCameraAutoExposureSettings(const unsigned int nCameraIndex, bool* autoExposureEnabled, float* autoExposureCompensation) const;
    bool GetCameraAutoWhiteBalance(const unsigned int nCameraIndex, bool* autoWhiteBalanceEnabled) const;

    EAxis        Get3DUpwardAxis() const;
    const char*  Get3DCalibrated() const;
    unsigned int Get3DLabeledMarkerCount() const;
    const char*  Get3DLabelName(unsigned int nMarkerIndex) const;
    unsigned int Get3DLabelColor(unsigned int nMarkerIndex) const;

    const char*  Get3DTrajectoryType(unsigned int nMarkerIndex) const;

    unsigned int Get3DBoneCount() const;
    const char*  Get3DBoneFromName(unsigned int boneIndex) const;
    const char*  Get3DBoneToName(unsigned int boneIndex) const;

    [[deprecated("EulerNames has been moved to general settings in protocol v1.21. New accessor is called GetEulerAngles.")]]
    void         Get6DOFEulerNames(std::string &first, std::string &second, std::string &third) const;
    unsigned int Get6DOFBodyCount() const;
    const char*  Get6DOFBodyName(unsigned int nBodyIndex) const;
    unsigned int Get6DOFBodyColor(unsigned int nBodyIndex) const;
    unsigned int Get6DOFBodyPointCount(unsigned int nBodyIndex) const;
    bool         Get6DOFBodyPoint(unsigned int nBodyIndex, unsigned int nMarkerIndex, SPoint &sPoint) const;
    bool         Get6DOFBodySettings(std::vector<SSettings6DOFBody>& settings);

    unsigned int GetGazeVectorCount() const;
    const char*  GetGazeVectorName(unsigned int nGazeVectorIndex) const;
    float        GetGazeVectorFrequency(unsigned int nGazeVectorIndex) const;
    bool         GetGazeVectorHardwareSyncUsed(unsigned int nGazeVectorIndex) const;
    bool         GetGazeVectorFilterUsed(unsigned int nGazeVectorIndex) const;

    unsigned int GetEyeTrackerCount() const;
    const char*  GetEyeTrackerName(unsigned int nEyeTrackerIndex) const;
    float        GetEyeTrackerFrequency(unsigned int nEyeTrackerIndex) const;
    bool         GetEyeTrackerHardwareSyncUsed(unsigned int nEyeTrackerIndex) const;

    unsigned int GetAnalogDeviceCount() const;
    bool         GetAnalogDevice(unsigned int nDeviceIndex, unsigned int &nDeviceID, unsigned int &nChannels,
                                 char* &pName, unsigned int &nFrequency, char* &pUnit,
                                 float &fMinRange, float &fMaxRange) const;
    const char*  GetAnalogLabel(unsigned int nDeviceIndex, unsigned int nChannelIndex) const;
    const char*  GetAnalogUnit(unsigned int nDeviceIndex, unsigned int nChannelIndex) const;

    void         GetForceUnits(char* &pLength, char* &pForce) const;
    unsigned int GetForcePlateCount() const;
    bool         GetForcePlate(unsigned int nPlateIndex, unsigned int &nID, unsigned int &nAnalogDeviceID,
                               unsigned int &nFrequency, char* &pType, char* &pName, float &fLength, float &fWidth) const;
    bool         GetForcePlateLocation(unsigned int nPlateIndex, SPoint sCorner[4]) const;
    bool         GetForcePlateOrigin(unsigned int nPlateIndex, SPoint &sOrigin) const;
    unsigned int GetForcePlateChannelCount(unsigned int nPlateIndex) const;
    bool         GetForcePlateChannel(unsigned int nPlateIndex, unsigned int nChannelIndex,
                                      unsigned int &nChannelNumber, float &fConversionFactor) const;
    bool         GetForcePlateCalibrationMatrix(unsigned int nPlateIndex, float fvCalMatrix[12][12], unsigned int* rows, unsigned int* columns) const;

    unsigned int GetImageCameraCount() const;
    bool         GetImageCamera(unsigned int nCameraIndex, unsigned int &nCameraID, bool &bEnabled,
                                CRTPacket::EImageFormat &eFormat, unsigned int &nWidth, unsigned int &nHeight,
                                float &fCropLeft, float &fCropTop, float &fCropRight, float &fCropBottom) const;

    unsigned int GetSkeletonCount() const;
    const char*  GetSkeletonName(unsigned int skeletonIndex);
    unsigned int GetSkeletonSegmentCount(unsigned int skeletonIndex);
    bool         GetSkeleton(unsigned int skeletonIndex, SSettingsSkeleton* skeleton);
    bool         GetSkeletonSegment(unsigned int skeletonIndex, unsigned int segmentIndex, SSettingsSkeletonSegment* segment);
    bool         GetSkeleton(unsigned int skeletonIndex, SSettingsSkeletonHierarchical& skeleton);
    void         GetSkeletons(std::vector<SSettingsSkeletonHierarchical>& skeletons);

    bool SetGeneralSettings(
        const unsigned int* pnCaptureFrequency, const float* pfCaptureTime,
        const bool* pbStartOnExtTrig, const bool* trigNO, const bool* trigNC, const bool* trigSoftware,
        const EProcessingActions* peProcessingActions, const EProcessingActions* peRtProcessingActions, const EProcessingActions* peReprocessingActions);
    [[deprecated("Replaced by SetGeneralSettings.")]]
    bool SetSystemSettings(
        const unsigned int* pnCaptureFrequency, const float* pfCaptureTime,
        const bool* pbStartOnExtTrig, const bool* trigNO, const bool* trigNC, const bool* trigSoftware,
        const EProcessingActions* peProcessingActions, const EProcessingActions* peRtProcessingActions, const EProcessingActions* peReprocessingActions);

    bool SetExtTimeBaseSettings(
        const bool*         pbEnabled,            const ESignalSource* peSignalSource,
        const bool*         pbSignalModePeriodic, const unsigned int*  pnFreqMultiplier,
        const unsigned int* pnFreqDivisor,        const unsigned int*  pnFreqTolerance,
        const float*        pfNominalFrequency,   const bool*          pbNegativeEdge,
        const unsigned int* pnSignalShutterDelay, const float*         pfNonPeriodicTimeout);

    bool SetExtTimestampSettings(const CRTProtocol::SSettingsGeneralExternalTimestamp& timestampSettings);

    bool SetCameraSettings(
        const unsigned int nCameraID,        const ECameraMode* peMode,
        const float*       pfMarkerExposure, const float*       pfMarkerThreshold,
        const int*         pnOrientation);

    bool SetCameraVideoSettings(
        const unsigned int nCameraID,               const EVideoResolution* eVideoResolution,
        const EVideoAspectRatio* eVideoAspectRatio, const unsigned int* pnVideoFrequency,
        const float* pfVideoExposure,               const float* pfVideoFlashTime);

    bool SetCameraSyncOutSettings(
        const unsigned int  nCameraID,      const unsigned int portNumber, const ESyncOutFreqMode* peSyncOutMode,
        const unsigned int* pnSyncOutValue, const float*       pfSyncOutDutyCycle,
        const bool*         pbSyncOutNegativePolarity);

    bool SetCameraLensControlSettings(const unsigned int nCameraID, const float focus, const float aperture);
    bool SetCameraAutoExposureSettings(const unsigned int nCameraID, const bool autoExposure, const float compensation);
    bool SetCameraAutoWhiteBalance(const unsigned int nCameraID, const bool enable);

    bool SetImageSettings(
        const unsigned int  nCameraID, const bool*         pbEnable,    const CRTPacket::EImageFormat* peFormat,
        const unsigned int* pnWidth,   const unsigned int* pnHeight,    const float* pfLeftCrop,
        const float*        pfTopCrop, const float*        pfRightCrop, const float* pfBottomCrop);

    bool SetForceSettings(
        const unsigned int nPlateID,  const SPoint* psCorner1, const SPoint* psCorner2,
        const SPoint*      psCorner3, const SPoint* psCorner4);

    bool Set6DOFBodySettings(std::vector<SSettings6DOFBody>);


    bool SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& skeletons);
    
    static const char* SkeletonDofToString(EDegreeOfFreedom dof);
    static EDegreeOfFreedom SkeletonStringToDof(const std::string& str);

    char* GetErrorString();

private:
    bool SendString(const char* pCmdStr, int nType);
    bool SendCommand(const char* pCmdStr);
    bool SendCommand(const std::string& cmdStr, std::string& commandResponseStr, unsigned int timeout = cWaitForDataTimeout);
    bool SendXML(const char* pCmdStr);
    const char * ReadSettings(const std::string& settingsType);
    bool ReceiveCalibrationSettings(int timeout = cWaitForDataTimeout);

private:
    INetwork*                      mpoNetwork;
    CRTPacket*                     mpoRTPacket;
    std::vector<char>              mDataBuff;
    std::vector<char>              mSendBuffer;
    CRTPacket::EEvent              meLastEvent;
    CRTPacket::EEvent              meState;  // Same as meLastEvent but without EventCameraSettingsChanged
    int                            mnMinorVersion;
    int                            mnMajorVersion;
    bool                           mbBigEndian;
    bool                           mbIsMaster;
    SSettingsGeneral               msGeneralSettings;
    SSettings3D                    ms3DSettings;
    std::vector<SSettings6DOFBody> m6DOFSettings;
    std::vector<SGazeVector>       mvsGazeVectorSettings;
    std::vector<SEyeTracker>       mvsEyeTrackerSettings;
    std::vector<SAnalogDevice>     mvsAnalogDeviceSettings;
    SSettingsForce                 msForceSettings;
    std::vector<SImageCamera>      mvsImageSettings;
    std::vector<SSettingsSkeleton> mSkeletonSettings;
    std::vector<SSettingsSkeletonHierarchical> mSkeletonSettingsHierarchical;
    SCalibration                   mCalibrationSettings;
    char                           maErrorStr[1024];
    unsigned short                 mnBroadcastPort;
    FILE*                          mpFileBuffer;
    std::vector<SDiscoverResponse> mvsDiscoverResponseList;
};


#endif // RTPROTOCOL_H