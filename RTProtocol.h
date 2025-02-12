#ifndef RTPROTOCOL_H
#define RTPROTOCOL_H


#include "RTPacket.h"
#include "Network.h"
#include "Settings.h"

#include <vector>
#include <string>
#include <map>
#include <limits>
#include <cstdint>
#include <memory>

#ifdef _MSC_VER
#pragma warning (disable : 4251)
#endif

#ifdef EXPORT_DLL
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

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

    using SComponentOptions = qualisys_cpp_sdk::SComponentOptions;
    using SPoint = qualisys_cpp_sdk::SPoint;
    using SBodyPoint = qualisys_cpp_sdk::SBodyPoint;
    using SDiscoverResponse = qualisys_cpp_sdk::SDiscoverResponse;
    using SSettingsGeneralCamera = qualisys_cpp_sdk::SSettingsGeneralCamera;
    using SSettingsGeneralExternalTimebase = qualisys_cpp_sdk::SSettingsGeneralExternalTimebase;
    using SSettingsGeneralExternalTimestamp = qualisys_cpp_sdk::SSettingsGeneralExternalTimestamp;
    using SSettingsGeneral = qualisys_cpp_sdk::SSettingsGeneral;
    using SSettings3DLabel = qualisys_cpp_sdk::SSettings3DLabel;
    using SSettingsBone = qualisys_cpp_sdk::SSettingsBone;
    using SSettings3D = qualisys_cpp_sdk::SSettings3D;
    using SSettings6DMesh = qualisys_cpp_sdk::SSettings6DMesh;
    using SOrigin = qualisys_cpp_sdk::SOrigin;
    using SSettings6DOFBody = qualisys_cpp_sdk::SSettings6DOFBody;
    using SGazeVector = qualisys_cpp_sdk::SGazeVector;
    using SEyeTracker = qualisys_cpp_sdk::SEyeTracker;
    using SAnalogDevice = qualisys_cpp_sdk::SAnalogDevice;
    using SForceChannel = qualisys_cpp_sdk::SForceChannel;
    using SForcePlate = qualisys_cpp_sdk::SForcePlate;
    using SSettingsForce = qualisys_cpp_sdk::SSettingsForce;
    using SImageCamera = qualisys_cpp_sdk::SImageCamera;
    using SCalibrationFov = qualisys_cpp_sdk::SCalibrationFov;
    using SCalibrationTransform = qualisys_cpp_sdk::SCalibrationTransform;
    using SCalibrationIntrinsic = qualisys_cpp_sdk::SCalibrationIntrinsic;
    using SCalibrationCamera = qualisys_cpp_sdk::SCalibrationCamera;
    using SCalibration = qualisys_cpp_sdk::SCalibration;
    using SPosition = qualisys_cpp_sdk::SPosition;
    using SRotation = qualisys_cpp_sdk::SRotation;
    using SCoupling = qualisys_cpp_sdk::SCoupling;
    using SDegreeOfFreedom = qualisys_cpp_sdk::SDegreeOfFreedom;
    using SMarker = qualisys_cpp_sdk::SMarker;
    using SBody = qualisys_cpp_sdk::SBody;
    using SSettingsSkeletonSegmentHierarchical = qualisys_cpp_sdk::SSettingsSkeletonSegmentHierarchical;
    using SSettingsSkeletonSegment = qualisys_cpp_sdk::SSettingsSkeletonSegment;
    using SSettingsSkeletonHierarchical = qualisys_cpp_sdk::SSettingsSkeletonHierarchical;
    using SSettingsSkeleton = qualisys_cpp_sdk::SSettingsSkeleton;

    using EStreamRate = qualisys_cpp_sdk::EStreamRate;
    using ECameraModel = qualisys_cpp_sdk::ECameraModel;
    using ECameraMode = qualisys_cpp_sdk::ECameraMode;
    using EVideoResolution = qualisys_cpp_sdk::EVideoResolution;
    using EVideoAspectRatio = qualisys_cpp_sdk::EVideoAspectRatio;
    using ESyncOutFreqMode = qualisys_cpp_sdk::ESyncOutFreqMode;
    using ESignalSource = qualisys_cpp_sdk::ESignalSource;
    using EAxis = qualisys_cpp_sdk::EAxis;
    using EProcessingActions = qualisys_cpp_sdk::EProcessingActions;
    using ECalibrationType = qualisys_cpp_sdk::ECalibrationType;
    using EOriginType = qualisys_cpp_sdk::EOriginType;
    using EDegreeOfFreedom = qualisys_cpp_sdk::EDegreeOfFreedom;
    using ETimestampType = qualisys_cpp_sdk::ETimestampType;

public:
    unsigned int GetSystemFrequency() const;

    CRTProtocol();
    ~CRTProtocol();

    bool       Connect(const char* serverAddr, unsigned short port = cDefaultBasePort, unsigned short* udpServerPort = nullptr,
                       int majorVersion = MAJOR_VERSION, int minorVersion = MINOR_VERSION, bool bigEndian = false, bool negotiateVersion = true);
    unsigned short GetUdpServerPort();
    void       Disconnect();
    bool       Connected() const;
    bool       SetVersion(int majorVersion, int minorVersion);
    bool       GetVersion(unsigned int &majorVersion, unsigned int &minorVersion);
    bool       GetQTMVersion(std::string& verStr);
    bool       GetByteOrder(bool &bigEndian);
    bool       CheckLicense(const std::string& licenseCode);
    bool       DiscoverRTServer(unsigned short serverPort, bool noLocalResponses, unsigned short discoverPort = cDefaultAutoDiscoverPort);
    int        GetNumberOfDiscoverResponses();
    bool       GetDiscoverResponse(unsigned int index, unsigned int &addr, unsigned short &basePort, std::string& message);

    bool       GetCurrentFrame(const std::string& components);
    bool       GetCurrentFrame(unsigned int componentType, const SComponentOptions& componentOptions = { });
    bool       StreamFrames(unsigned int componentType);
    bool       StreamFrames(EStreamRate rate, unsigned int rateArg, unsigned short udpPort, const char* udpAddr, const char* components);
    bool       StreamFrames(EStreamRate rate, unsigned int rateArg, unsigned short udpPort, const char* udpAddr,
                            unsigned int componentType, const SComponentOptions& componentOptions = { });
    bool       StreamFramesStop();
    bool       GetState(CRTPacket::EEvent &event, bool update = true, int timeout = cWaitForDataTimeout);
    bool       GetCapture(const char* fileName, bool isC3D);
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
    bool       SaveCapture(const std::string& fileName, bool overwrite, std::string* newFileName = nullptr, int sizeOfNewFileName = 0);
    bool       LoadProject(const std::string& fileName);
    bool       Reprocess();
    void       OverrideNetwork(INetwork* network);

    static double SMPTENormalizedSubFrame(unsigned int captureFrequency, unsigned int timestampFrequency, unsigned int subFrame);
    static bool GetEventString(CRTPacket::EEvent event, char* str);
    static bool ConvertRateString(const char* rateText, EStreamRate &rate, unsigned int &rateArg);
    static unsigned int ConvertComponentString(const std::string& componentsString);
    static bool GetComponentString(std::string& componentStr, unsigned int componentType, const SComponentOptions& options = SComponentOptions());
    static std::vector<std::pair<unsigned int, std::string>> GetComponents(const std::string& componentsString);
    static bool GetComponent(std::string& componentStr, unsigned int& component, std::string& option);

    [[deprecated("Replaced by Receive.")]]
    int         ReceiveRTPacket(CRTPacket::EPacketType &type, bool skipEvents = true, int timeout = cWaitForDataTimeout); // timeout < 0 : Blocking receive
    CNetwork::ResponseType Receive(CRTPacket::EPacketType &type, bool skipEvents = true, int timeout = cWaitForDataTimeout); // timeout < 0 : Blocking receive
    

    CRTPacket* GetRTPacket();

    bool ReadGeneralSettings();
    [[deprecated("Replaced by ReadGeneralSettings.")]]
    bool ReadCameraSystemSettings(); // Same as ReadGeneralSettings
    bool ReadCalibrationSettings();
    bool Read3DSettings(bool &dataAvailable);
    bool Read6DOFSettings(bool &dataAvailable);
    bool ReadGazeVectorSettings(bool &dataAvailable);
    bool ReadEyeTrackerSettings(bool &dataAvailable);
    bool ReadAnalogSettings(bool &dataAvailable);
    bool ReadForceSettings(bool &dataAvailable);
    bool ReadImageSettings(bool &dataAvailable);
    bool ReadSkeletonSettings(bool &dataAvailable, bool skeletonGlobalData = false);


    void Get3DSettings(EAxis& axisUpwards, std::string& calibrationTime, std::vector<SSettings3DLabel>& labels3D, std::vector<SSettingsBone>& bones);
    void GetGazeVectorSettings(std::vector<SGazeVector>& gazeVectorSettings);
    void GetEyeTrackerSettings(std::vector<SEyeTracker>& eyeTrackerSettings);
    void GetAnalogSettings(std::vector<SAnalogDevice>& analogSettings);
    void GetForceSettings(SSettingsForce& forceSettings);
    void GetGeneralSettings(
        unsigned int &captureFrequency, float &captureTime,
        bool& startOnExtTrig, bool& trigNO, bool& trigNC, bool& trigSoftware,
        EProcessingActions &processingActions, EProcessingActions &rtProcessingActions, EProcessingActions &reprocessingActions) const;
    [[deprecated("Replaced by GetGeneralSettings.")]]
    void GetSystemSettings(
        unsigned int &captureFrequency, float &captureTime,
        bool& startOnExtTrig, bool& trigNO, bool& trigNC, bool& trigSoftware,
        EProcessingActions &processingActions, EProcessingActions &rtProcessingActions, EProcessingActions &reprocessingActions) const;

    void GetCalibrationSettings(SCalibration &calibrationSettings) const;

    void GetExtTimeBaseSettings(
        bool         &enabled,            ESignalSource &signalSource,
        bool         &signalModePeriodic, unsigned int  &freqMultiplier,
        unsigned int &freqDivisor,        unsigned int  &freqTolerance,
        float        &nominalFrequency,   bool          &negativeEdge,
        unsigned int &signalShutterDelay, float         &nonPeriodicTimeout) const;
    void GetExtTimestampSettings(SSettingsGeneralExternalTimestamp& timestamp) const;

    void GetEulerAngles(std::string& first, std::string& second, std::string& third) const;
    
    unsigned int GetCameraCount() const;
    std::vector<SSettingsGeneralCamera> GetDevices() const;

    bool GetCameraSettings(
        unsigned int cameraIndex, unsigned int &id,     ECameraModel &model, 
        bool         &underwater, bool &supportsHwSync, unsigned int &serial, ECameraMode  &mode) const;

    bool GetCameraMarkerSettings(
        unsigned int cameraIndex,       unsigned int &currentExposure,
        unsigned int &minExposure,      unsigned int &maxExposure,
        unsigned int &currentThreshold, unsigned int &minThreshold,
        unsigned int &maxThreshold) const;

    bool GetCameraVideoSettings(
        unsigned int cameraIndex,            EVideoResolution &videoResolution,
        EVideoAspectRatio &videoAspectRatio, unsigned int &videoFrequency,
        unsigned int &currentExposure,       unsigned int &minExposure,
        unsigned int &maxExposure,           unsigned int &currentFlashTime,
        unsigned int &minFlashTime,          unsigned int &maxFlashTime) const;

    bool GetCameraSyncOutSettings(
        unsigned int cameraIndex,   unsigned int portNumber, ESyncOutFreqMode &syncOutMode,
        unsigned int &syncOutValue, float        &syncOutDutyCycle,
        bool         &syncOutNegativePolarity) const;

    bool GetCameraPosition(
        unsigned int cameraIndex, SPoint &point, float rotationMatrix[3][3]) const;

    bool GetCameraOrientation(
        unsigned int cameraIndex, int &orientation) const;

    bool GetCameraResolution(
        unsigned int cameraIndex,   unsigned int &markerWidth,
        unsigned int &markerHeight, unsigned int &videoWidth,
        unsigned int &videoHeight) const;

    bool GetCameraFOV(
        unsigned int cameraIndex,  unsigned int &markerLeft,  unsigned int &markerTop,
        unsigned int &markerRight, unsigned int &markerBottom,
        unsigned int &videoLeft,   unsigned int &videoTop,
        unsigned int &videoRight,  unsigned int &videoBottom) const;

    bool GetCameraLensControlSettings(const unsigned int cameraIndex, float* focus, float* aperture) const;
    bool GetCameraAutoExposureSettings(const unsigned int cameraIndex, bool* autoExposureEnabled, float* autoExposureCompensation) const;
    bool GetCameraAutoWhiteBalance(const unsigned int cameraIndex, bool* autoWhiteBalanceEnabled) const;

    EAxis        Get3DUpwardAxis() const;
    const char*  Get3DCalibrated() const;
    unsigned int Get3DLabeledMarkerCount() const;
    const char*  Get3DLabelName(unsigned int markerIndex) const;
    unsigned int Get3DLabelColor(unsigned int markerIndex) const;

    const char*  Get3DTrajectoryType(unsigned int markerIndex) const;

    unsigned int Get3DBoneCount() const;
    const char*  Get3DBoneFromName(unsigned int boneIndex) const;
    const char*  Get3DBoneToName(unsigned int boneIndex) const;

    [[deprecated("EulerNames has been moved to general settings in protocol v1.21. New accessor is called GetEulerAngles.")]]
    void         Get6DOFEulerNames(std::string &first, std::string &second, std::string &third) const;
    unsigned int Get6DOFBodyCount() const;
    const char*  Get6DOFBodyName(unsigned int bodyIndex) const;
    unsigned int Get6DOFBodyColor(unsigned int bodyIndex) const;
    unsigned int Get6DOFBodyPointCount(unsigned int bodyIndex) const;
    bool         Get6DOFBodyPoint(unsigned int bodyIndex, unsigned int markerIndex, SPoint &point) const;
    bool         Get6DOFBodySettings(std::vector<SSettings6DOFBody>& settings);

    unsigned int GetGazeVectorCount() const;
    const char*  GetGazeVectorName(unsigned int gazeVectorIndex) const;
    float        GetGazeVectorFrequency(unsigned int gazeVectorIndex) const;
    bool         GetGazeVectorHardwareSyncUsed(unsigned int gazeVectorIndex) const;
    bool         GetGazeVectorFilterUsed(unsigned int gazeVectorIndex) const;

    unsigned int GetEyeTrackerCount() const;
    const char*  GetEyeTrackerName(unsigned int eyeTrackerIndex) const;
    float        GetEyeTrackerFrequency(unsigned int eyeTrackerIndex) const;
    bool         GetEyeTrackerHardwareSyncUsed(unsigned int eyeTrackerIndex) const;

    unsigned int GetAnalogDeviceCount() const;
    bool         GetAnalogDevice(unsigned int deviceIndex, unsigned int &deviceID, unsigned int &channels,
                                 char* &name, unsigned int &frequency, char* &unit,
                                 float &minRange, float &maxRange) const;
    const char*  GetAnalogLabel(unsigned int deviceIndex, unsigned int channelIndex) const;
    const char*  GetAnalogUnit(unsigned int deviceIndex, unsigned int channelIndex) const;

    void         GetForceUnits(char* &length, char* &force) const;
    unsigned int GetForcePlateCount() const;
    bool         GetForcePlate(unsigned int plateIndex, unsigned int &id, unsigned int &analogDeviceID,
                               unsigned int &frequency, char* &type, char* &name, float &length, float &width) const;
    bool         GetForcePlateLocation(unsigned int plateIndex, SPoint corner[4]) const;
    bool         GetForcePlateOrigin(unsigned int plateIndex, SPoint &origin) const;
    unsigned int GetForcePlateChannelCount(unsigned int plateIndex) const;
    bool         GetForcePlateChannel(unsigned int plateIndex, unsigned int channelIndex,
                                      unsigned int &channelNumber, float &conversionFactor) const;
    bool         GetForcePlateCalibrationMatrix(unsigned int plateIndex, float calMatrix[12][12], unsigned int* rows, unsigned int* columns) const;

    unsigned int GetImageCameraCount() const;
    bool         GetImageCamera(unsigned int cameraIndex, unsigned int &cameraID, bool &enabled,
                                CRTPacket::EImageFormat &format, unsigned int &width, unsigned int &height,
                                float &cropLeft, float &cropTop, float &cropRight, float &cropBottom) const;

    unsigned int GetSkeletonCount() const;
    const char*  GetSkeletonName(unsigned int skeletonIndex);
    unsigned int GetSkeletonSegmentCount(unsigned int skeletonIndex);
    bool         GetSkeleton(unsigned int skeletonIndex, SSettingsSkeleton* skeleton);
    bool         GetSkeletonSegment(unsigned int skeletonIndex, unsigned int segmentIndex, SSettingsSkeletonSegment* segment);
    bool         GetSkeleton(unsigned int skeletonIndex, SSettingsSkeletonHierarchical& skeleton);
    void         GetSkeletons(std::vector<SSettingsSkeletonHierarchical>& skeletons);

    bool SetGeneralSettings(
        const unsigned int* captureFrequency, const float* captureTime,
        const bool* startOnExtTrig, const bool* trigNO, const bool* trigNC, const bool* trigSoftware,
        const EProcessingActions* processingActions, const EProcessingActions* rtProcessingActions, const EProcessingActions* reprocessingActions);
    [[deprecated("Replaced by SetGeneralSettings.")]]
    bool SetSystemSettings(
        const unsigned int* captureFrequency, const float* captureTime,
        const bool* startOnExtTrig, const bool* trigNO, const bool* trigNC, const bool* trigSoftware,
        const EProcessingActions* processingActions, const EProcessingActions* rtProcessingActions, const EProcessingActions* reprocessingActions);

    bool SetExtTimeBaseSettings(
        const bool*         enabled,            const ESignalSource* signalSource,
        const bool*         signalModePeriodic, const unsigned int*  freqMultiplier,
        const unsigned int* freqDivisor,        const unsigned int*  freqTolerance,
        const float*        nominalFrequency,   const bool*          negativeEdge,
        const unsigned int* signalShutterDelay, const float*         nonPeriodicTimeout);

    bool SetExtTimestampSettings(const CRTProtocol::SSettingsGeneralExternalTimestamp& timestampSettings);

    bool SetCameraSettings(
        const unsigned int cameraID,        const ECameraMode* mode,
        const float*       markerExposure,  const float*       markerThreshold,
        const int*         orientation);

    bool SetCameraVideoSettings(
        const unsigned int       cameraID,         const EVideoResolution* videoResolution,
        const EVideoAspectRatio* videoAspectRatio, const unsigned int*     videoFrequency,
        const float*             videoExposure,    const float*            videoFlashTime);

    bool SetCameraSyncOutSettings(
        const unsigned int  cameraID,     const unsigned int portNumber, const ESyncOutFreqMode* syncOutMode,
        const unsigned int* syncOutValue, const float*       syncOutDutyCycle,
        const bool*         syncOutNegativePolarity);

    bool SetCameraLensControlSettings(const unsigned int cameraID, const float focus, const float aperture);
    bool SetCameraAutoExposureSettings(const unsigned int cameraID, const bool autoExposure, const float compensation);
    bool SetCameraAutoWhiteBalance(const unsigned int cameraID, const bool enable);

    bool SetImageSettings(
        const unsigned int  cameraID, const bool*         enable,    const CRTPacket::EImageFormat* format,
        const unsigned int* width,    const unsigned int* height,    const float* leftCrop,
        const float*        topCrop,  const float*        rightCrop, const float* bottomCrop);

    bool SetForceSettings(
        const unsigned int plateID,  const SPoint* corner1, const SPoint* corner2,
        const SPoint*      corner3,  const SPoint* corner4);

    bool Set6DOFBodySettings(std::vector<SSettings6DOFBody> settings);

    bool SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& skeletons);
    
    static const char* SkeletonDofToString(EDegreeOfFreedom dof);
    static EDegreeOfFreedom SkeletonStringToDof(const std::string& str);

    char* GetErrorString();

private:
    bool SendString(const char* cmdStr, int type);
    bool SendCommand(const char* cmdStr);
    bool SendCommand(const std::string& cmdStr, std::string& commandResponseStr, unsigned int timeout = cWaitForDataTimeout);
    bool SendXML(const char* cmdStr);
    const char * ReadSettings(const std::string& settingsType);
    bool ReceiveCalibrationSettings(int timeout = cWaitForDataTimeout);

private:
    INetwork*                      mNetwork;
    CRTPacket*                     mRTPacket;
    std::vector<char>              mDataBuff;
    std::vector<char>              mSendBuffer;
    CRTPacket::EEvent              mLastEvent;
    CRTPacket::EEvent              mState;  // Same as mLastEvent but without EventCameraSettingsChanged
    int                            mMinorVersion;
    int                            mMajorVersion;
    bool                           mBigEndian;
    bool                           mIsMaster;
    SSettingsGeneral               mGeneralSettings;
    SSettings3D                    m3DSettings;
    std::vector<SSettings6DOFBody> m6DOFSettings;
    std::vector<SGazeVector>       mGazeVectorSettings;
    std::vector<SEyeTracker>       mEyeTrackerSettings;
    std::vector<SAnalogDevice>     mAnalogDeviceSettings;
    SSettingsForce                 mForceSettings;
    std::vector<SImageCamera>      mImageSettings;
    std::vector<SSettingsSkeleton> mSkeletonSettings;
    std::vector<SSettingsSkeletonHierarchical> mSkeletonSettingsHierarchical;
    SCalibration                   mCalibrationSettings;
    char                           mErrorStr[1024];
    unsigned short                 mBroadcastPort;
    FILE*                          mFileBuffer;
    std::vector<SDiscoverResponse> mDiscoverResponseList;
};


#endif // RTPROTOCOL_H