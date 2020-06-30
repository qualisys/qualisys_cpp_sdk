#ifndef INPUT_H
#define INPUT_H

#include "RTProtocol.h"

class CInput
{
public:
    enum EOperation
    {
        DataTransfer                    = 1,
        Statistics                      = 2,
        Noise2D                         = 3,
        MonitorEvents                   = 4,
        DiscoverRTServers               = 5,
        ControlQTM                      = 6,
        ViewSettings                    = 7,
        ChangeGeneralSystemSettings     = 8,
        ChangeExtTimebaseSettings       = 9,
        ChangeExtTimestampSettings      = 10,
        ChangeProcessingActionsSettings = 11,
        ChangeCameraSettings            = 12,
        ChangeCameraSyncOutSettings     = 13,
        Change6dSettings                = 14,
        ChangeForceSettings             = 15,
        ChangeImageSettings             = 16,
        ChangeSkeletonSettings          = 17
    };

    enum EQTMCommand
    {
        New             = 1,
        Close           = 2,
        Start           = 3,
        StartRTFromFile = 4,
        Stop            = 5,
        Load            = 6,
        Save            = 7,
        LoadProject     = 8,
        GetCaptureC3D   = 9,
        GetCaptureQTM   = 10,
        Trig            = 11,
        SetQTMEvent     = 12,
        Reprocess       = 13,
        Calibrate       = 14,
        CalibrateRefine = 15
    };

public:
    bool CheckKeyPressed();
    char WaitForKey();
    bool ReadVersion(int &nMajorVersion, int &nMinorVersion);
    bool ReadByteOrder(bool &bBigEndian);
    void ReadClientControlPassword(char* pPassword, int nLen);
    bool ReadDiscoverRTServers(bool &bDiscover, char* tServerAddress);
    bool ReadOperation(EOperation &eOperation);
    bool ReadStreamRate(CRTProtocol::EStreamRate &eRate, int &nArg);
    bool ReadDataComponents(unsigned int &nComponentType, char* selectedAnalogChannels, int selectedAnalogChannelsLen, bool &skeletonGlobalReferenceFrame);
    unsigned int ReadDataComponent(bool printInstr, bool &skeletonGlobalReferenceFrame);
    bool Read2DNoiseTest();
    bool ReadDataTest(bool bLogSelection, bool &bStreamTCP, bool &bStreamUDP, bool &bLogToFile, bool &bOnlyTimeAndFrameNumber, unsigned short &nUDPPort, char *tAddress, int nAddressLen);
    void ReadGeneralSettings(unsigned int &nCaptureFrequency, float &fCaptureTime, bool &bExternalTrigger, bool& trigNO, bool& trigNC, bool& trigSoftware);
    void ReadProcessingActionsSettings(CRTProtocol::EProcessingActions &eProcessingActions,
                                       CRTProtocol::EProcessingActions &eRtProcessingActions,
                                       CRTProtocol::EProcessingActions &eReprocessingActions);
    void ReadExtTimeBaseSettings(bool         &bEnabled,            int          &nSignalSource,
                                 bool         &bSignalModePeriodic, unsigned int &nMultiplier,
                                 unsigned int &nDivisor,            unsigned int &nFrequencyTolerance,
                                 float        &fNominalFrequency,   bool         &bNegativeEdge,
                                 unsigned int &nSignalShutterDelay, float        &fNonPeriodicTimeout);
    void ReadTimestampSettings(CRTProtocol::SSettingsGeneralExternalTimestamp& timestampSettings);
    void ReadCameraSettings(unsigned int &nCameraId,       int   &nMode,            CRTProtocol::EVideoResolution &videoResolution, CRTProtocol::EVideoAspectRatio &videoAspectRatio,
                            unsigned int &nVideoFrequency, float &fVideoExposure,   float &fVideoFlashTime,
                            float        &fMarkerExposure, float &fMarkerThreshold, int   &nRotation,
                            float        &fFocus,          float &fAperture,        bool  &autoExposure,
                            float        &exposureCompensation, bool &autoWhiteBalance);
    void ReadCameraSyncOutSettings(unsigned int &nCameraId,         int &portNumber, int  &nSyncOutMode, unsigned int &nSyncOutValue,
                                    float        &fSyncOutDutyCycle, bool &bSyncOutNegativePolarity);

    void ReadImageSettings(unsigned int &nCameraId, bool &bEnable, int &nFormat, unsigned int &nWidth,
                           unsigned int &nHeight, float &fLeftCrop, float &fTopCrop, float &fRightCrop, float &fBottomCrop);
    void ReadForceSettings(unsigned int &nForcePlateIndex, float afCorner[4][3]);
    void Read6DSettings(unsigned int &color, float &maxResidual, unsigned int &minMarkersInBody, float &boneLengthTolerance, std::string &filterPreset);
    void Read6DSettingsMesh(CRTProtocol::SSettings6DMesh &mesh);
    void Read6DSettingsOrigin(CRTProtocol::SOrigin &origin);
    void Read6DSettingsPoints(std::vector<CRTProtocol::SBodyPoint> &points);
        bool ReadQTMCommand(EQTMCommand &eCommand);

    std::string ReadString(const std::string& text);
    float       ReadFloat(const std::string& text, float fDefault);
    bool        ReadYesNo(const std::string& text, bool bDefault);
    int         ReadInt(const std::string& text, int nDefault);
    char        ReadChar(char cDefault, bool bShowInput = false);
    unsigned short ReadPort(int nDefault);
    bool        ReadUseScrolling(bool &bOutputModeScrolling);

private:
    unsigned int mnMajorVersion;
    unsigned int mnMinorVersion;
};


#endif // INPUT_H