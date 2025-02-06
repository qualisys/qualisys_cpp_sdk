#pragma once

#include "RTPacket.h"

#include <string>
#include <limits>
#include <cmath>
#include <stdexcept>

#ifdef EXPORT_DLL
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

namespace qualisys_cpp_sdk
{
    struct DLL_EXPORT SComponentOptions
    {
        SComponentOptions() :
            mAnalogChannels(nullptr),
            mSkeletonGlobalData(false)
        {
        }
        char* mAnalogChannels;
        bool mSkeletonGlobalData;
    };

    enum EStreamRate
    {
        RateNone = 0,
        RateAllFrames = 1,
        RateFrequency = 2,
        RateFrequencyDivisor = 3
    };

    enum ECameraModel
    {
        ModelMacReflex = 0,
        ModelProReflex120 = 1,
        ModelProReflex240 = 2,
        ModelProReflex500 = 3,
        ModelProReflex1000 = 4,
        ModelOqus100 = 5,
        ModelOqus300 = 6,
        ModelOqus300Plus = 7,
        ModelOqus400 = 8,
        ModelOqus500 = 9,
        ModelOqus200C = 10,
        ModelOqus500Plus = 11,
        ModelOqus700 = 12,
        ModelOqus700Plus = 13,
        ModelOqus600Plus = 14,
        ModelMiqusM1 = 15,
        ModelMiqusM3 = 16,
        ModelMiqusM5 = 17,
        ModelMiqusSyncUnit = 18,
        ModelMiqusVideo = 19,
        ModelMiqusVideoColor = 20,
        ModelMiqusHybrid = 21,
        ModelArqusA5 = 22,
        ModelArqusA12 = 23,
        ModelArqusA9 = 24,
        ModelArqusA26 = 25,
        ModelMiqusVideoColorPlus = 26,
        ModelUnknown = 27
    };

    enum ECameraMode
    {
        ModeMarker = 0,
        ModeMarkerIntensity = 1,
        ModeVideo = 2
    };

    enum EVideoResolution
    {
        VideoResolution1440p = 0,
        VideoResolution1080p = 1,
        VideoResolution720p = 2,
        VideoResolution540p = 3,
        VideoResolution480p = 4,
        VideoResolutionNone = 5
    };

    enum EVideoAspectRatio
    {
        VideoAspectRatio16x9 = 0,
        VideoAspectRatio4x3 = 1,
        VideoAspectRatio1x1 = 2,
        VideoAspectRatioNone = 3
    };

    enum ESyncOutFreqMode
    {
        ModeShutterOut = 1, // A pulse per frame is sent
        ModeMultiplier,
        ModeDivisor,
        ModeIndependentFreq,
        ModeMeasurementTime,
        ModeFixed100Hz,
        ModeSystemLiveTime
    };

    enum ESignalSource
    {
        SourceControlPort = 0,
        SourceIRReceiver = 1,
        SourceSMPTE = 2,
        SourceVideoSync = 3,
        SourceIRIG = 4
    };

    enum EAxis
    {
        XPos = 0,
        XNeg = 1,
        YPos = 2,
        YNeg = 3,
        ZPos = 4,
        ZNeg = 5
    };

    enum EProcessingActions
    {
        ProcessingNone = 0x0000,
        ProcessingTracking2D = 0x0001,
        ProcessingTracking3D = 0x0002,
        ProcessingTwinSystemMerge = 0x0004,
        ProcessingSplineFill = 0x0008,
        ProcessingAIM = 0x0010,
        Processing6DOFTracking = 0x0020,
        ProcessingForceData = 0x0040,
        ProcessingGazeVector = 0x0080,
        ProcessingExportTSV = 0x0100,
        ProcessingExportC3D = 0x0200,
        ProcessingExportMatlabFile = 0x0800,
        ProcessingExportAviFile = 0x1000,
        ProcessingPreProcess2D = 0x2000
    };

    struct DLL_EXPORT SPoint
    {
        float fX;
        float fY;
        float fZ;
    };

    struct DLL_EXPORT SBodyPoint
    {
        std::string name;
        float fX;
        float fY;
        float fZ;
        bool  virtual_;
        uint32_t physicalId;
    };


    struct DLL_EXPORT SDiscoverResponse
    {
        char           message[128];
        unsigned int   nAddr;
        unsigned short nBasePort;
    };

    struct DLL_EXPORT SSettingsGeneralCamera
    {
        unsigned int nID;
        ECameraModel eModel;
        bool         bUnderwater;
        bool         bSupportsHwSync;
        unsigned int nSerial;
        ECameraMode  eMode;
        EVideoResolution eVideoResolution;
        EVideoAspectRatio eVideoAspectRatio;
        unsigned int nVideoFrequency;
        unsigned int nVideoExposure;      // Micro seconds
        unsigned int nVideoExposureMin;   // Micro seconds
        unsigned int nVideoExposureMax;   // Micro seconds
        unsigned int nVideoFlashTime;     // Micro seconds
        unsigned int nVideoFlashTimeMin;  // Micro seconds
        unsigned int nVideoFlashTimeMax;  // Micro seconds
        unsigned int nMarkerExposure;     // Micro seconds
        unsigned int nMarkerExposureMin;  // Micro seconds
        unsigned int nMarkerExposureMax;  // Micro seconds
        unsigned int nMarkerThreshold;
        unsigned int nMarkerThresholdMin;
        unsigned int nMarkerThresholdMax;
        float        fPositionX;
        float        fPositionY;
        float        fPositionZ;
        float        fPositionRotMatrix[3][3];
        unsigned int nOrientation;             // Degrees
        unsigned int nMarkerResolutionWidth;   // Sub pixels
        unsigned int nMarkerResolutionHeight;  // Sub pixels
        unsigned int nVideoResolutionWidth;    // Pixels
        unsigned int nVideoResolutionHeight;   // Pixels
        unsigned int nMarkerFOVLeft;           // Pixels
        unsigned int nMarkerFOVTop;            // Pixels
        unsigned int nMarkerFOVRight;          // Pixels
        unsigned int nMarkerFOVBottom;         // Pixels
        unsigned int nVideoFOVLeft;            // Pixels
        unsigned int nVideoFOVTop;             // Pixels
        unsigned int nVideoFOVRight;           // Pixels
        unsigned int nVideoFOVBottom;          // Pixels
        ESyncOutFreqMode eSyncOutMode[2];
        unsigned int nSyncOutValue[2];
        float        fSyncOutDutyCycle[2];     // Percent
        bool         bSyncOutNegativePolarity[3];
        float        fFocus;
        float        fAperture;
        bool         autoExposureEnabled;
        float        autoExposureCompensation;
        int          autoWhiteBalance;
    };

    struct DLL_EXPORT SSettingsGeneralExternalTimebase
    {
        SSettingsGeneralExternalTimebase() :
            bEnabled(false),
            eSignalSource(SourceControlPort),
            bSignalModePeriodic(false),
            nFreqMultiplier(0),
            nFreqDivisor(0),
            nFreqTolerance(0),
            fNominalFrequency(0.0f),
            bNegativeEdge(false),
            nSignalShutterDelay(0),
            fNonPeriodicTimeout(0.0f)
        {
        }

        bool          bEnabled;
        ESignalSource eSignalSource;
        bool          bSignalModePeriodic;
        unsigned int  nFreqMultiplier;
        unsigned int  nFreqDivisor;
        unsigned int  nFreqTolerance;
        float         fNominalFrequency;
        bool          bNegativeEdge;
        unsigned int  nSignalShutterDelay;
        float         fNonPeriodicTimeout;
    };

    enum ETimestampType
    {
        Timestamp_SMPTE = 0,
        Timestamp_IRIG,
        Timestamp_CameraTime,
    };

    struct DLL_EXPORT SSettingsGeneralExternalTimestamp
    {
        SSettingsGeneralExternalTimestamp() :
            bEnabled(false),
            nFrequency(0),
            nType(Timestamp_SMPTE)
        {
        }
        bool bEnabled;
        uint32_t nFrequency;
        ETimestampType nType;
    };

    struct DLL_EXPORT SSettingsGeneral
    {
        SSettingsGeneral() :
            nCaptureFrequency(0),
            fCaptureTime(0.0f),
            bStartOnExternalTrigger(false),
            bStartOnTrigNO(false),
            bStartOnTrigNC(false),
            bStartOnTrigSoftware(false),
            eProcessingActions(EProcessingActions::ProcessingNone),
            eRtProcessingActions(EProcessingActions::ProcessingNone),
            eReprocessingActions(EProcessingActions::ProcessingNone)
        {
            sExternalTimebase = { };
            sTimestamp = { };
            eulerRotations[0] = "";
            eulerRotations[1] = "";
            eulerRotations[2] = "";
            vsCameras.clear();
        }

        unsigned int nCaptureFrequency;
        float fCaptureTime;
        bool bStartOnExternalTrigger;
        bool bStartOnTrigNO;
        bool bStartOnTrigNC;
        bool bStartOnTrigSoftware;
        SSettingsGeneralExternalTimebase sExternalTimebase;
        SSettingsGeneralExternalTimestamp sTimestamp;
        EProcessingActions eProcessingActions;   // Binary flags.
        EProcessingActions eRtProcessingActions; // Binary flags.
        EProcessingActions eReprocessingActions; // Binary flags.
        std::string eulerRotations[3]; // R G B
        std::vector< SSettingsGeneralCamera > vsCameras;
    };

    struct DLL_EXPORT SSettings3DLabel
    {
        std::string  oName;
        unsigned int nRGBColor;
        std::string type;
    };

    struct DLL_EXPORT SSettingsBone
    {
        std::string fromName;
        std::string toName;
        unsigned int color;
    };

    struct DLL_EXPORT SSettings3D
    {
        EAxis                           eAxisUpwards;
        char                            pCalibrationTime[32];
        std::vector< SSettings3DLabel > s3DLabels;
        std::vector< SSettingsBone >    sBones;
    };

    struct DLL_EXPORT SSettings6DMesh
    {
        std::string name;
        SPoint      position;
        SPoint      rotation;
        float       scale;
        float       opacity;
    };

    enum EOriginType
    {
        GlobalOrigin,
        RelativeOrigin,
        FixedOrigin
    };

    struct DLL_EXPORT SOrigin
    {
        EOriginType type;
        uint32_t    relativeBody;
        SPoint      position;
        float       rotation[9];
    };

    struct DLL_EXPORT SSettings6DOFBody
    {
        std::string           name;
        bool                  enabled;
        uint32_t              color;
        std::string           filterPreset;
        float                 maxResidual;
        uint32_t              minMarkersInBody;
        float                 boneLengthTolerance;
        SSettings6DMesh       mesh;
        SOrigin               origin;
        std::vector<SBodyPoint> points;
    };

    struct DLL_EXPORT SGazeVector
    {
        std::string    name;
        float          frequency;
        bool           hwSync;
        bool           filter;
    };

    struct DLL_EXPORT SEyeTracker
    {
        std::string    name;
        float          frequency;
        bool           hwSync;
    };

    struct DLL_EXPORT SAnalogDevice
    {
        unsigned int               nDeviceID;
        unsigned int               nChannels;
        std::string                oName;
        std::vector< std::string > voLabels;
        unsigned int               nFrequency;
        std::string                oUnit;
        float                      fMinRange;
        float                      fMaxRange;
        std::vector< std::string > voUnits;
    };

    struct DLL_EXPORT SForceChannel
    {
        unsigned int nChannelNumber;
        float        fConversionFactor;
    };

    struct DLL_EXPORT SForcePlate
    {
        unsigned int                 nID;
        unsigned int                 nAnalogDeviceID;
        std::string                  oType;
        std::string                  oName;
        unsigned int                 nFrequency;
        float                        fLength;
        float                        fWidth;
        SPoint                       asCorner[4];
        SPoint                       sOrigin;
        std::vector< SForceChannel > vChannels;
        bool                         bValidCalibrationMatrix;
        float                        afCalibrationMatrix[12][12];
        unsigned int                 nCalibrationMatrixRows;
        unsigned int                 nCalibrationMatrixColumns;
    };

    struct DLL_EXPORT SSettingsForce
    {
        std::string                 oUnitLength;
        std::string                 oUnitForce;
        std::vector< SForcePlate >  vsForcePlates;
    };

    struct DLL_EXPORT SImageCamera
    {
        unsigned int            nID;
        bool                    bEnabled;
        CRTPacket::EImageFormat eFormat;
        unsigned int            nWidth;
        unsigned int            nHeight;
        float                   fCropLeft;
        float                   fCropTop;
        float                   fCropRight;
        float                   fCropBottom;
    };

    struct DLL_EXPORT SCalibrationFov
    {
        uint32_t left;
        uint32_t top;
        uint32_t right;
        uint32_t bottom;
    };

    struct DLL_EXPORT SCalibrationTransform
    {
        double x;
        double y;
        double z;
        double r11;
        double r12;
        double r13;
        double r21;
        double r22;
        double r23;
        double r31;
        double r32;
        double r33;
    };

    struct DLL_EXPORT SCalibrationIntrinsic
    {
        double focal_length;
        double sensor_min_u;
        double sensor_max_u;
        double sensor_min_v;
        double sensor_max_v;
        double focal_length_u;
        double focal_length_v;
        double center_point_u;
        double center_point_v;
        double skew;
        double radial_distortion_1;
        double radial_distortion_2;
        double radial_distortion_3;
        double tangental_distortion_1;
        double tangental_distortion_2;
    };

    struct DLL_EXPORT SCalibrationCamera
    {
        bool active;
        bool calibrated;
        std::string message;
        uint32_t point_count;
        double avg_residual;
        uint32_t serial;
        std::string model;
        uint32_t view_rotation;
        SCalibrationFov fov_marker;
        SCalibrationFov fov_marker_max;
        SCalibrationFov fov_video;
        SCalibrationFov fov_video_max;
        SCalibrationTransform transform;
        SCalibrationIntrinsic intrinsic;
    };

    enum ECalibrationType
    {
        regular,
        fixed,
        refine
    };

    struct DLL_EXPORT SCalibration
    {
        bool calibrated = false;
        std::string source;
        std::string created;
        std::string qtm_version;
        ECalibrationType type = regular;
        double refit_residual = std::numeric_limits<double>::quiet_NaN(); // Only for refine calibration.
        double wand_length = std::numeric_limits<double>::quiet_NaN(); // Not for fixed calibration.
        uint32_t max_frames = 0;                                        // Not for fixed calibration.
        double short_arm_end = std::numeric_limits<double>::quiet_NaN(); // Not for fixed calibration.
        double long_arm_end = std::numeric_limits<double>::quiet_NaN(); // Not for fixed calibration.
        double long_arm_middle = std::numeric_limits<double>::quiet_NaN(); // Not for fixed calibration.
        double result_std_dev = std::numeric_limits<double>::quiet_NaN(); // Not for fixed calibration.
        double result_min_max_diff = std::numeric_limits<double>::quiet_NaN(); // Not for fixed calibration.
        double result_refit_residual = std::numeric_limits<double>::quiet_NaN(); // Only for refine calibration.
        uint32_t result_consecutive = 0;    // Only for refine calibration.
        std::vector<SCalibrationCamera> cameras;
    };

    struct DLL_EXPORT SPosition
    {
        double x = std::numeric_limits<double>::quiet_NaN();
        double y = std::numeric_limits<double>::quiet_NaN();
        double z = std::numeric_limits<double>::quiet_NaN();

        bool IsValid() const
        {
            return !std::isnan(x) && !std::isnan(y) && !std::isnan(z);
        }
    };

    struct DLL_EXPORT SRotation
    {
        double x = std::numeric_limits<double>::quiet_NaN();
        double y = std::numeric_limits<double>::quiet_NaN();
        double z = std::numeric_limits<double>::quiet_NaN();
        double w = std::numeric_limits<double>::quiet_NaN();

        bool IsValid() const
        {
            return !std::isnan(x) && !std::isnan(y) && !std::isnan(z) && !std::isnan(w);
        }
    };

    enum EDegreeOfFreedom
    {
        RotationX = 0,
        RotationY = 1,
        RotationZ = 2,
        TranslationX = 3,
        TranslationY = 4,
        TranslationZ = 5
    };

    struct DLL_EXPORT SCoupling
    {
        std::string segment;
        EDegreeOfFreedom degreeOfFreedom;
        double coefficient = std::numeric_limits<double>::quiet_NaN();
    };

    struct DLL_EXPORT SDegreeOfFreedom
    {
        EDegreeOfFreedom type;
        double lowerBound = std::numeric_limits<double>::quiet_NaN();
        double upperBound = std::numeric_limits<double>::quiet_NaN();
        std::vector<SCoupling> couplings;
        double goalValue = std::numeric_limits<double>::quiet_NaN();
        double goalWeight = std::numeric_limits<double>::quiet_NaN();
    };

    struct DLL_EXPORT SMarker
    {
        std::string name;
        SPosition position;
        double weight;
    };

    struct DLL_EXPORT SBody
    {
        std::string name;
        SPosition position;
        SRotation rotation;
        double weight;
    };

    struct DLL_EXPORT SSettingsSkeletonSegmentHierarchical
    {
        std::string name;
        uint32_t id = 0;
        std::string solver;
        SPosition position;
        SRotation rotation;
        SPosition defaultPosition;
        SRotation defaultRotation;
        std::vector<SDegreeOfFreedom> degreesOfFreedom;
        SPosition endpoint;
        std::vector<SMarker> markers;
        std::vector<SBody> bodies;
        std::vector<SSettingsSkeletonSegmentHierarchical> segments;
    };

    struct DLL_EXPORT SSettingsSkeletonSegment : CRTPacket::SSkeletonSegment
    {
        std::string name;
        int parentId;
        int parentIndex;
    };

    struct DLL_EXPORT SSettingsSkeletonHierarchical
    {
        std::string name;
        double scale;
        SSettingsSkeletonSegmentHierarchical rootSegment;
    };

    struct DLL_EXPORT SSettingsSkeleton
    {
        std::string name;
        std::vector<SSettingsSkeletonSegment> segments;
    };

    constexpr auto DEGREES_OF_FREEDOM =
    {
        std::make_pair(qualisys_cpp_sdk::EDegreeOfFreedom::RotationX, "RotationX"),
        std::make_pair(qualisys_cpp_sdk::EDegreeOfFreedom::RotationY, "RotationY"),
        std::make_pair(qualisys_cpp_sdk::EDegreeOfFreedom::RotationZ, "RotationZ"),
        std::make_pair(qualisys_cpp_sdk::EDegreeOfFreedom::TranslationX, "TranslationX"),
        std::make_pair(qualisys_cpp_sdk::EDegreeOfFreedom::TranslationY, "TranslationY"),
        std::make_pair(qualisys_cpp_sdk::EDegreeOfFreedom::TranslationZ, "TranslationZ")
    };

    DLL_EXPORT const char* SkeletonDofToStringSettings(EDegreeOfFreedom dof);

    DLL_EXPORT EDegreeOfFreedom SkeletonStringToDofSettings(const std::string& str);

    struct DLL_EXPORT ISettingsDeserializer {
        virtual ~ISettingsDeserializer() = default;
        virtual bool DeserializeGeneralSettings(SSettingsGeneral& pGeneralSettings) = 0;
        virtual bool Deserialize3DSettings(SSettings3D& p3dSettings, bool& pDataAvailable) = 0;
        virtual bool DeserializeAnalogSettings(std::vector<SAnalogDevice>& pAnalogDeviceSettings, bool& pDataAvailable) = 0;
        virtual bool DeserializeForceSettings(SSettingsForce& pForceSettings, bool& pDataAvailable) = 0;
        virtual bool DeserializeImageSettings(std::vector<SImageCamera>& pImageSettings, bool& pDataAvailable) = 0;
        virtual bool Deserialize6DOFSettings(std::vector<SSettings6DOFBody>& p6DOFSettings, SSettingsGeneral& pGeneralSettings, bool& pDataAvailable) = 0;
        virtual bool DeserializeGazeVectorSettings(std::vector<SGazeVector>& pGazeVectorSettings, bool& pDataAvailable) = 0;
        virtual bool DeserializeEyeTrackerSettings(std::vector<SEyeTracker>& pEyeTrackerSettings, bool& pDataAvailable) = 0;
        virtual bool DeserializeSkeletonSettings(bool pSkeletonGlobalData, std::vector<SSettingsSkeletonHierarchical>&, std::vector<SSettingsSkeleton>&, bool& pDataAvailable) = 0;
        virtual bool DeserializeCalibrationSettings(SCalibration& pCalibrationSettings) = 0;
    };

    struct DLL_EXPORT ISettingsSerializer {
        virtual ~ISettingsSerializer() = default;

        virtual std::string SetGeneralSettings(const unsigned int* pnCaptureFrequency, const float* pfCaptureTime,
                                               const bool* pbStartOnExtTrig, const bool* pStartOnTrigNO,
                                               const bool* pStartOnTrigNC, const bool* pStartOnTrigSoftware,
                                               const EProcessingActions* peProcessingActions,
                                               const EProcessingActions* peRtProcessingActions,
                                               const EProcessingActions* peReprocessingActions) = 0;

        virtual std::string SetExtTimeBaseSettings(
            const bool* pbEnabled, const ESignalSource* peSignalSource,
            const bool* pbSignalModePeriodic, const unsigned int* pnFreqMultiplier,
            const unsigned int* pnFreqDivisor, const unsigned int* pnFreqTolerance,
            const float* pfNominalFrequency, const bool* pbNegativeEdge,
            const unsigned int* pnSignalShutterDelay, const float* pfNonPeriodicTimeout) = 0;

        virtual std::string SetExtTimestampSettings(const SSettingsGeneralExternalTimestamp& timestampSettings) = 0;

        virtual std::string SetCameraSettings(
            const unsigned int pCameraId, const ECameraMode* peMode,
            const float* pfMarkerExposure, const float* pfMarkerThreshold,
            const int* pnOrientation) = 0;

        virtual std::string SetCameraVideoSettings(
            const unsigned int pCameraId, const EVideoResolution* eVideoResolution,
            const EVideoAspectRatio* eVideoAspectRatio, const unsigned int* pnVideoFrequency,
            const float* pfVideoExposure, const float* pfVideoFlashTime) = 0;

        virtual std::string SetCameraSyncOutSettings(
            const unsigned int pCameraId, const unsigned int portNumber, const ESyncOutFreqMode* peSyncOutMode,
            const unsigned int* pnSyncOutValue, const float* pfSyncOutDutyCycle,
            const bool* pbSyncOutNegativePolarity) = 0;

        virtual std::string SetCameraLensControlSettings(const unsigned int pCameraId, const float pFocus,
                                                         const float pAperture) = 0;

        virtual std::string SetCameraAutoExposureSettings(const unsigned int pCameraId, const bool pAutoExposure,
                                                          const float pCompensation) = 0;

        virtual std::string SetCameraAutoWhiteBalance(const unsigned int pCameraId, const bool pEnable) = 0;

        virtual std::string SetImageSettings(
            const unsigned int pCameraId, const bool* pbEnable, const CRTPacket::EImageFormat* peFormat,
            const unsigned int* pnWidth, const unsigned int* pnHeight, const float* pfLeftCrop,
            const float* pfTopCrop, const float* pfRightCrop, const float* pfBottomCrop) = 0;

        virtual std::string SetForceSettings(
            const unsigned int pPlateId, const SPoint* pCorner1, const SPoint* pCorner2,
            const SPoint* pCorner3, const SPoint* pCorner4) = 0;

        virtual std::string Set6DOFBodySettings(const std::vector<SSettings6DOFBody>& pSettings6Dofs) = 0;

        virtual std::string SetSkeletonSettings(const std::vector<SSettingsSkeletonHierarchical>& pSettingsSkeletons) = 0;
    };
}
