#include "Input.h"
#include <conio.h>
#include <iostream>


bool CInput::CheckKeyPressed()
{
    return (GetKeyState(VK_ESCAPE) & 0x8000);
}


char CInput::WaitForKey()
{
    char c;

    do
    {
        c = _getch();
    } while (c == 0);

    return _getch();
}

bool CInput::ReadVersion(int &nMajorVersion, int &nMinorVersion)
{
    int nMaxMajorVersion = nMajorVersion;
    int nMaxMinorVersion = nMinorVersion;
    char pVerStr[255];

    printf("\nSelect protocol version 1.0 - %d.%d (Default %d.%d): ",
        nMaxMajorVersion, nMaxMinorVersion, nMaxMajorVersion, nMaxMinorVersion);
    gets_s(pVerStr, 254);
    printf("\n");
    if (sscanf_s(pVerStr, "%d.%d", &mnMajorVersion, &mnMinorVersion) != 2)
    {
        mnMajorVersion = nMaxMajorVersion;
        mnMinorVersion = nMaxMinorVersion;
    }

    nMajorVersion = mnMajorVersion;
    nMinorVersion = mnMinorVersion;

    return true;
} // ReadVersion


bool CInput::ReadByteOrder(bool &bBigEndian)
{
    char c;

    bBigEndian = false;

    printf("Select byte order:\n\n");
    printf("1: Little Endian (Default)\n");
    printf("2: Big Endian\n");
    printf("Q: Quit\n\n");

    c = ReadChar('1');

    if (c == '2')
    {
        bBigEndian = true;
    }
    else if ((c == 'q') || (c == 'Q'))
    {
        return false;
    }
    return true;
} // ReadByteOrder


void CInput::ReadClientControlPassword(char* pPassword, int nLen)
{
    printf("Enter Client Control Password : ");
    gets_s(pPassword, nLen);
}


bool CInput::ReadDiscoverRTServers(bool &bDiscover, char* tServerAddress)
{
    bDiscover = false;

    printf("\n\nSelect Operation:\n\n");
    printf("1 : Connect to QTM RT server '%s' (default)\n", tServerAddress);
    printf("2 : Discover QTM RT servers\n");
    printf("Q : Quit\n\n");

    int nSelection = (int)(ReadChar('1') - '0');

    if (nSelection >= 1 && nSelection <= 2)
    {
        bDiscover = (nSelection == 2);
        return true;
    }
    return false;
}


bool CInput::ReadOperation(EOperation &eOperation)
{
    int nSelection;

    printf("\n\nSelect Operation:\n\n");
    printf("1 : Data Transfer (default)\n");
    printf("2 : Statistics\n");
    printf("3 : 2D Noise\n");
    if (mnMajorVersion > 1 || mnMinorVersion > 4)
    {
        printf("4 : Monitor Events\n");
    }
    if (mnMajorVersion > 1 || mnMinorVersion > 7)
    {
        printf("5 : Discover QTM RT Servers\n");
    }
    if (mnMajorVersion > 1 || mnMinorVersion > 4)
    {
        printf("6 : Control QTM\n");
    }
    if (mnMajorVersion > 1 || mnMinorVersion > 6)
    {
        printf("7 : View Settings (General, Calibration, 3D, 6DOF, GazeVector, EyeTracker, Analog, Force, Image, Skeleton)\n");
    }
    if (mnMajorVersion > 1 || mnMinorVersion > 6)
    {
        printf("8 : Change Settings (General, 6d, Force, Image)\n");
    }
    printf("Q : Quit\n\n");

    nSelection = (int)(ReadChar('1') - '0');

    bool returnValue = false;

    switch (nSelection)
    {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            eOperation = (EOperation)nSelection;
            returnValue = true;
            break;
        case 8:
            printf("0 : Change General Settings (default)\n");
            printf("1 : Change External Time Base Settings\n");
            printf("2 : Change Timestamp Settings\n");
            printf("3 : Change Processing Actions Settings\n");
            printf("4 : Change Camera Settings\n");
            printf("5 : Change Camera Sync Out Settings\n");
            printf("6 : Change 6D Settings\n");
            printf("7 : Change Force Settings\n");
            printf("8 : Change Image Settings\n");
            printf("9 : Change Skeleton Settings\n");
            printf("Q : Quit\n\n");

            nSelection = (int)(ReadChar('0') - '0');

            if (nSelection >= 0 && nSelection <= 9)
            {
                eOperation = (EOperation)(nSelection + ChangeGeneralSystemSettings);
                returnValue = true;
            }
            break;
        default:
        break;
    }

    return returnValue;
}


bool CInput::ReadStreamRate(CRTProtocol::EStreamRate &eRate, int &nArg)
{
    int nSelection;

    printf("\nSelect Transfer Rate:\n\n");
    printf("1 : All Frames (Default)\n");
    printf("2 : Frequency\n");
    printf("3 : Frequency Divisor\n");
    printf("Q : Quit\n\n");

    nSelection = (int)(ReadChar('1') - '0');

    if (nSelection >= 1 && nSelection <= 3)
    {
        eRate = (CRTProtocol::EStreamRate)nSelection;
        if (nSelection == 2)
        {
            nArg = ReadInt("Enter Frequency: ", 1);
            printf("\n\n");
            return true;
        }
        if (nSelection == 3)
        {
            nArg = ReadInt("Enter Frequency Divisor: ", 1);
            printf("\n\n");
            return true;
        }
        return true;
    }
    return false;
}


bool CInput::ReadDataComponents(unsigned int &nComponentType, char* selectedAnalogChannels, int selectedAnalogChannelsLen, bool &skeletonGlobalReferenceFrame)
{
    bool bNoComponentSelected = true;
    
    while (bNoComponentSelected)
    {
        nComponentType = ReadDataComponent(true, skeletonGlobalReferenceFrame);

        // Check if user wants multiple types.
        if (nComponentType == 0xffffffff)
        {
            unsigned int nAddType = 0;
            nComponentType = 0;
            printf("Add Component to multiple selection. Enter to stop selection.");
            do
            {
                nAddType = ReadDataComponent(false, skeletonGlobalReferenceFrame);
                nComponentType |= nAddType;
                char tmpStr[128];
                CRTProtocol::GetComponentString(tmpStr, nAddType);
                printf("%s\n", tmpStr);
            } while (nAddType != 0);

            if (nComponentType == 0)
            {
                printf("No components selected.\n\n\n");
            }
            else
            {
                bNoComponentSelected = false;
            }
        }
        else
        {
            if (nComponentType == 0)
            {
                nComponentType = CRTProtocol::cComponent2d;
            }
            bNoComponentSelected = false;
        }
    }

    if ((nComponentType & CRTProtocol::cComponentAnalog) || (nComponentType & CRTProtocol::cComponentAnalogSingle))
    {
        selectedAnalogChannels[0] = 0;

        if (!ReadYesNo("Read all channels (y/n)\n", true))
        {
            printf("Select analog channels: (Ex, 1,3,4-7,8)\n");
            fgets(selectedAnalogChannels, selectedAnalogChannelsLen, stdin);
        }
    }

    return nComponentType != 0;
} // ReadDataComponent


unsigned int CInput::ReadDataComponent(bool printInstr, bool &skeletonGlobalReferenceFrame)
{
    if (printInstr)
    {
        printf("Select Component:\n\n");
        printf("1: 2D (Default)\n");
        printf("2: 2D Linearized\n");
        printf("3: 3D\n");
        printf("4: 3D with residuals\n");
        printf("5: 3D no labels\n");
        printf("6: 3D no labels with residuals\n");
        printf("7: 6D\n");
        printf("8: 6D with residuals\n");
        printf("9: 6D Euler\n");
        printf("a: 6D Euler with residuals\n");
        if (mnMajorVersion > 1 || mnMinorVersion > 11)
        {
            printf("b: Gaze vector\n");
        }
        if (mnMajorVersion > 1 || mnMinorVersion > 18)
        {
            printf("c: Eye tracker\n");
        }
        printf("d: Analog\n");
        if (mnMajorVersion > 1 || mnMinorVersion > 0)
        {
            printf("e: Analog Single\n");
        }
        printf("f: Force\n");
        if (mnMajorVersion > 1 || mnMinorVersion > 7)
        {
            printf("g: Force Single\n");
        }
        if (mnMajorVersion > 1 || mnMinorVersion > 7)
        {
            printf("h: Image\n");
        }
        if (mnMajorVersion > 1 || mnMinorVersion > 16)
        {
            printf("i: Timecode\n");
        }
        if (mnMajorVersion > 1 || mnMinorVersion > 18)
        {
            printf("j: Skeleton Local\n");
            printf("k: Skeleton Global\n");
        }
        printf("l: Multiple Components\n");
    }

    int c = ReadChar('0');

    unsigned int nComponentType;

    switch (c)
    {
        case '1' :
            nComponentType = CRTProtocol::cComponent2d;
            break;
        case '2' :
            nComponentType = CRTProtocol::cComponent2dLin;
            break;
        case '3' :
            nComponentType = CRTProtocol::cComponent3d;
            break;
        case '4' :
            nComponentType = CRTProtocol::cComponent3dRes;
            break;
        case '5' :
            nComponentType = CRTProtocol::cComponent3dNoLabels;
            break;
        case '6' :
            nComponentType = CRTProtocol::cComponent3dNoLabelsRes;
            break;
        case '7' :
            nComponentType = CRTProtocol::cComponent6d;
            break;
        case '8' :
            nComponentType = CRTProtocol::cComponent6dRes;
            break;
        case '9' :
            nComponentType = CRTProtocol::cComponent6dEuler;
            break;
        case 'a' :
            nComponentType = CRTProtocol::cComponent6dEulerRes;
            break;
        case 'b' :
            nComponentType = CRTProtocol::cComponentGazeVector;
            break;
        case 'c':
            nComponentType = CRTProtocol::cComponentEyeTracker;
            break;
        case 'd' :
            nComponentType = CRTProtocol::cComponentAnalog;
            break;
        case 'e' :
            nComponentType = CRTProtocol::cComponentAnalogSingle;
            break;
        case 'f' :
            nComponentType = CRTProtocol::cComponentForce;
            break;
        case 'g' :
            nComponentType = CRTProtocol::cComponentForceSingle;
            break;
        case 'h' :
            nComponentType = CRTProtocol::cComponentImage;
            break;
        case 'i':
            nComponentType = CRTProtocol::cComponentTimecode;
            break;
        case 'j':
            nComponentType = CRTProtocol::cComponentSkeleton;
            skeletonGlobalReferenceFrame = false;
            break;
        case 'k':
            nComponentType = CRTProtocol::cComponentSkeleton;
            skeletonGlobalReferenceFrame = true;
            break;
        case 'l' :
            nComponentType = 0xffffffff;
            break;
        default:    
            nComponentType = 0;
            break;
    }
    return nComponentType;
} // ReadDataComponent


bool CInput::Read2DNoiseTest()
{
    return ReadYesNo("\nPerform 2D Noise Test (y/n)?\n", false);
}


bool CInput::ReadDataTest(bool bLogSelection, bool &bStreamTCP, bool &bStreamUDP, bool &bLogToFile,
                          bool &bOnlyTimeAndFrameNumber, unsigned short &nUDPPort, char *tUDPAddress, int nAddressLen)
{
    int nSelection;

    bStreamTCP              = false;
    bStreamUDP              = false;
    bLogToFile              = false;
    bOnlyTimeAndFrameNumber = false;
    tUDPAddress[0]          = 0;

    printf("\nSelect Transfer Mode:\n\n"
        "1 : Stream Data TCP (Default).\n"
        "2 : Stream Data UDP\n"
        "3 : Read Data Frame by Frame.\n"
        "Q : Quit \n\n");

    nSelection = (int)(ReadChar('1') - '0');

    if (nSelection >= 1 && nSelection <= 3)
    {
        if (nSelection == 1)
        {
            bStreamTCP = true;
        }
        if (nSelection == 2)
        {
            bStreamUDP = true;
            printf("\n"
                "1 : Stream to client address at random port (Default)\n"
                "2 : Stream to different address and port\n"
                "Q : Quit \n\n");
            nSelection = (int)(ReadChar('1') - '0');

            if (nSelection >= 1 && nSelection <= 3)
            {
                switch (nSelection)
                {
                    case 1 :
                        nUDPPort = 0;
                        break;
                    case 2 :
                        nUDPPort = ReadPort(4545);
                        printf("Enter UDP receiver address : ");
                        gets_s(tUDPAddress, nAddressLen);
                        break;
                }
            }
            else
            {
                return false;
            }
        }

        if (nSelection == 1 && bLogSelection)
        {
            printf("\nSelect Logging Mode:\n\n"
                "1 : No Log (Default).\n"
                "2 : Log to file.\n"
                "3 : Log to file. Only time and frame number.\n"
                "Q : Quit \n\n");

            nSelection = (int)(ReadChar('1') - '0');

            if (nSelection >= 1 && nSelection <= 3)
            {
                if (nSelection == 2)
                {
                    bLogToFile = true;
                }
                if (nSelection == 3)
                {
                    bLogToFile              = true;
                    bOnlyTimeAndFrameNumber = true;
                }
                return true;
            }
        }
        else
        {
            return true;
        }
    }
    return false;
}

void CInput::ReadGeneralSettings(unsigned int &nCaptureFrequency, float &fCaptureTime, bool &bExternalTrigger, bool& trigNO, bool& trigNC, bool& trigSoftware)
{
    nCaptureFrequency = ReadInt("Enter Capture Frequency (Hz) : ", 20);

    fCaptureTime = ReadFloat("Enter Capture Time (seconds) : ", 1.0);
    
    if (mnMajorVersion > 1 || mnMinorVersion > 14)
    {
        trigNO = ReadYesNo("Enter Start on Trig NO (y/n)?\n", false);
        trigNC = ReadYesNo("Enter Start Trig NC (y/n)?\n", false);
        trigSoftware = ReadYesNo("Enter Start Software trigger (y/n)?\n", false);
    }
    else
    {
        bExternalTrigger = ReadYesNo("Enter Start on External Trigger (y/n)?\n", false);
    }

}

void CInput::ReadProcessingActionsSettings(CRTProtocol::EProcessingActions &eProcessingActions,
                                           CRTProtocol::EProcessingActions &eRtProcessingActions,
                                           CRTProtocol::EProcessingActions &eReprocessingActions)
{
    const char* processings[3] = { "Processing Actions", "Real-time Processing Actions", "Reprocessing Actions" };
    CRTProtocol::EProcessingActions* processingActions[3] =
    {
        &eProcessingActions,
        &eRtProcessingActions,
        &eReprocessingActions
    };

    auto actionsCount = (mnMajorVersion > 1 || mnMinorVersion > 13) ? 3 : 1;

    for (auto i = 0; i < actionsCount; i++)
    {
        printf("\nChange %s settings (y/n)? ", processings[i]);
        if (!ReadYesNo("", true))
        {
            continue;
        }

        if (mnMajorVersion > 1 || mnMinorVersion > 13)
        {
            if (ReadYesNo("\n2D pre-processing (y/n)? ", true))
            {
                *processingActions[i] = CRTProtocol::ProcessingPreProcess2D;
            }
        }

        printf("Enter Tracking Mode :\n");
        printf("  1 : None\n");
        if (i != 1)
        {
            printf("  2 : 2D\n");
        }
        printf("  3 : 3D\n");
        printf("Select 1 - 3 : ");
        char nTracking = ReadChar('1', true);
        if (nTracking == '2' && i != 1) // i != 1 => Not RtProcessingSettings
        {
            *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingTracking2D);
        }
        else if (nTracking == '3')
        {
            *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingTracking3D);
        }
        else
        {
            *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingNone);
        }

        if (i != 1) //Not RtProcessingSettings
        {
            if (ReadYesNo("TwinSystem Merge (y/n)? ", true))
            {
                *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingTwinSystemMerge);
            }
            if (ReadYesNo("Spline Fill (y/n)? ", true))
            {
                *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingSplineFill);
            }
        }
        if (ReadYesNo("AIM (y/n)? ", true))
        {
            *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingAIM);
        }
        if (ReadYesNo("6DOF Tracking (y/n)? ", true))
        {
            *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::Processing6DOFTracking);
        }
        if (ReadYesNo("Force (y/n)? ", true))
        {
            *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingForceData);
        }
        if (ReadYesNo("Gaze Vector (y/n)? ", true))
        {
            *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingGazeVector);
        }

        if (i != 1) // Not RtProcessingSettings
        {
            if (ReadYesNo("Export TSV (y/n)? ", true))
            {
                *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingExportTSV);
            }
            if (ReadYesNo("Export C3D (y/n)? ", true))
            {
                *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingExportC3D);
            }
            if (ReadYesNo("Export MATLAB File (y/n)? ", true))
            {
                *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingExportMatlabFile);
            }
            if (ReadYesNo("Export AVI File (y/n)? ", true))
            {
                *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingExportAviFile);
            }
        }
        printf("\n");
    }
}

void CInput::ReadExtTimeBaseSettings(bool         &bEnabled,            int          &nSignalSource,
                                     bool         &bSignalModePeriodic, unsigned int &nMultiplier,
                                     unsigned int &nDivisor,            unsigned int &nFrequencyTolerance,
                                     float        &fNominalFrequency,   bool         &bNegativeEdge,
                                     unsigned int &nSignalShutterDelay, float        &fNonPeriodicTimeout)
{
    bEnabled = ReadYesNo("Enable External Time Base (y/n)? ", false);

    if (bEnabled)
    {
        printf("Enter Signal Source :\n");
        printf("  1 : Control Port\n");
        printf("  2 : IR Receiver\n");
        printf("  3 : SMPTE\n");
        printf("  4 : Video Sync\n");
        printf("Select 1 - 4 : ");
        nSignalSource = ReadChar('1', true) - '0' - 1;
        if (nSignalSource < 0 || nSignalSource > 3)
        {
            nSignalSource = 0;
        }

        if (nSignalSource == 0 || nSignalSource == 1 || nSignalSource == 3)
        {
            bSignalModePeriodic = ReadYesNo("Signal Mode Periodic (y/n)? ", true);
        }

        if ((nSignalSource == 0 || nSignalSource == 1 || nSignalSource == 2 || nSignalSource == 3) && bSignalModePeriodic)
        {
            nMultiplier = ReadInt("Enter Frequency Multiplier : ", 1);

            nDivisor = ReadInt("Enter Frequency Divisor : ", 1);

            if (nSignalSource == 0 || nSignalSource == 1 || nSignalSource == 3)
            {
                nFrequencyTolerance = ReadInt("Enter Frequency Tolerance (ppm): ", 1000);
            }

            fNominalFrequency = ReadFloat("Enter Nominal Frequency (Hz) : ", 0);
        }

        if (nSignalSource == 0 || nSignalSource == 3)
        {
            bNegativeEdge = ReadYesNo("Negative Edge (y/n)? ", true);
        }

        nFrequencyTolerance = ReadInt("Enter Signal Shutter Delay (us) : ", 0);

        if ((nSignalSource == 0 || nSignalSource == 1 || nSignalSource == 3) && !bSignalModePeriodic)
        {
            fNonPeriodicTimeout = ReadFloat("Non Periodic Timeout (s) : ", 1);
        }
    }
}

void CInput::ReadTimestampSettings(CRTProtocol::SSettingsGeneralExternalTimestamp& timestampSettings)
{
    timestampSettings.bEnabled = ReadYesNo("Enable Timestamp (y/n)? ", false);
    if (timestampSettings.bEnabled)
    {
        printf("Enter Type:\n");
        printf("  1 : SMPTE\n");
        printf("  2 : IRIG\n");
        printf("  3 : CameraTime\n");
        printf("Select 1 - 3 : ");
        auto type = ReadChar('1', true) - '0' - 1;
        if (type < 0 || type > 2)
        {
            type = CRTProtocol::Timestamp_SMPTE;
        }
        timestampSettings.nType = (CRTProtocol::ETimestampType)type;
        timestampSettings.nFrequency = ReadInt("Frequency: ", 30);
    }
}

void CInput::ReadCameraSettings(unsigned int &nCameraId,        int   &nMode,            CRTProtocol::EVideoResolution &videoResolution, CRTProtocol::EVideoAspectRatio &videoAspectRatio,
                                unsigned int &nVideoFrequency,  float &fVideoExposure,   float &fVideoFlashTime,
                                float        &fMarkerExposure,  float &fMarkerThreshold, int   &nRotation,
                                float        &fFocus,           float &fAperture,        bool  &autoExposure,
                                float        &exposureCompensation, bool &autoWhiteBalance)
{
    nCameraId = ReadInt("\nEnter Camera ID : ", 1);

    printf("Enter Camera Mode :\n");
    printf("  1 : Marker\n");
    printf("  2 : Marker Intensity\n");
    printf("  3 : Video\n");
    printf("Select 1 - 3 : ");
    nMode = ReadChar('1', true) - '0' - 1;
    if (nMode < 0 || nMode > 2)
    {
        nMode = 0;
    }

    if (nMode == 0 || nMode == 1)
    {
        fMarkerExposure = ReadFloat("Enter Marker Exposure (us) (Default 300 us): ", 300);
        fMarkerThreshold = ReadFloat("Enter Marker Threshold (50 - 900) (Default 150) : ", 150);
    }
    if (nMode == 2)
    {
        nVideoFrequency = ReadInt("Enter Video Frequency (Default 24 Hz) : ", 24);
        fVideoExposure = ReadFloat("Enter Video Exposure (us) (Default 300 us) : ", 300);
        fVideoFlashTime = ReadFloat("Enter Video Flash Time (us) (Default 300 us) : ", 300);
    }

    printf("Enter Video Resolution :\n");
    printf("  1 : 1080p\n");
    printf("  2 : 720p\n");
    printf("  3 : 540p\n");
    printf("  4 : 480p\n");
    printf("  5 : None (default)\n");
    printf("Select 1 - 5 : ");
    int tmpVideoRes = ReadChar('1', true) - '0' - 1;
    if (tmpVideoRes >= 0 && tmpVideoRes <= 3)
    {
        videoResolution = (CRTProtocol::EVideoResolution)tmpVideoRes;
    }
    else
    {
        videoResolution = CRTProtocol::VideoResolutionNone;
    }

    printf("Enter Video AspectRatio :\n");
    printf("  1 : 16:9\n");
    printf("  2 : 4:3\n");
    printf("  3 : 1:1\n");
    printf("  4 : None (default)\n");
    printf("Select 1 - 4 : ");
    int tmpVideoAsp = ReadChar('1', true) - '0' - 1;
    if (tmpVideoAsp >= 0 && tmpVideoAsp <= 2)
    {
        videoAspectRatio = (CRTProtocol::EVideoAspectRatio)tmpVideoAsp;;
    }
    else
    {
        videoAspectRatio = CRTProtocol::VideoAspectRatioNone;
    }

    nRotation = ReadInt("Enter Camera Rotation (degrees) (Default 0 degrees): ", 0);
    nRotation = nRotation - (nRotation % 90);
    if (nRotation < 0 || nRotation > 270)
    {
        nRotation = 0;
    }

    fFocus = ReadFloat("Enter Lens Focus: ", 5);
    fAperture = ReadFloat("Enter Lens Aperture: ", 5);

    autoExposure = ReadYesNo("Enable Auto Exposure? (y/n): ", false);
    if (autoExposure)
    {
        exposureCompensation = ReadFloat("Enter Exposure Compensation: ", 0);
    }
    autoWhiteBalance = ReadYesNo("Enable Auto White Balance? (y/n): ", true);
}


void CInput::ReadCameraSyncOutSettings(unsigned int &nCameraId, int &portNumber, int   &nSyncOutMode, unsigned int &nSyncOutValue,
                                        float &fSyncOutDutyCycle, bool  &bSyncOutNegativePolarity)
{
    nCameraId = ReadInt("\nEnter Camera ID : ", 1);
    portNumber = ReadInt("Enter Sync out port number (1-3) ", 1);

    if (portNumber > 0 && portNumber < 3)
    {
        printf("Enter Camera Mode :\n");
        printf("  1 : Shutter Out\n");
        printf("  2 : Multiplier\n");
        printf("  3 : Divisor\n");
        printf("  4 : Actual Frequency\n");
        printf("  5 : Measurement Time\n");
        printf("  6 : Fixed 100 Hz\n");
        printf("Select 1 - 6 : ");
        nSyncOutMode = ReadChar('1', true) - '0';
        if (nSyncOutMode < 1 || nSyncOutMode > 6)
        {
            nSyncOutMode = 1;
        }

        if (nSyncOutMode >= 2 && nSyncOutMode <= 4)
        {
            printf("Enter %s : ", nSyncOutMode == 2 ? "Multiplier" : (nSyncOutMode == 3 ? "Divisor" : "Camera Independent Frequency"));
            nSyncOutValue = ReadInt("", 1000);

            fSyncOutDutyCycle = ReadFloat("Enter Sync Out Duty Cycle (%%) : ", 0.5);
        }
    }

    if (nSyncOutMode < 6)
    {
        bSyncOutNegativePolarity = ReadYesNo("Negative Polarity? (y/n): ", true);
    }
}


void CInput::ReadImageSettings(unsigned int &nCameraId, bool &bEnable, int &nFormat, unsigned int &nWidth,
                               unsigned int &nHeight, float &fLeftCrop, float &fTopCrop, float &fRightCrop, float &fBottomCrop)
{
    int c;

    nCameraId = ReadInt("Enter Camera ID (Default 1): ", 1);

    bEnable = ReadYesNo("Enable Camera? (y/n): ", true);

    printf("Enter Image format :\n");
    printf("  1 : RAW Grayscale\n");
    printf("  2 : RAW BGR\n");
    printf("  3 : JPG (Default)\n");
    printf("  4 : PNG\n");
    printf("Select 1 - 4 : ");

    nFormat = ReadChar('3', true) - '0' - 1;
    if (nFormat < 1 || nFormat > 4)
    {
        nFormat = 3;
    }
    nWidth = ReadInt("Enter Image Width (Default 320): ", 320);
    nHeight = ReadInt("Enter Image Height (Default 200) : ", 200);
    
    c = ReadInt("Enter Image Left Crop (%% of Width, Default 0 %%) : ", 0);
    if (c < 0 || c > 100)
    {
        c = 0;
    }
    fLeftCrop = (float)c / (float)100.0;

    c = ReadInt("Enter Image Top Crop (%% of Height, Default 0 %%) : ", 0);
    if (c < 0 || c > 100)
    {
        c = 0;
    }
    fTopCrop = (float)c / (float)100.0;

    c = ReadInt("Enter Image Right Crop (%% of Width, Default 100 %%) : ", 100);
    if (c < 0 || c > 100)
    {
        c = 100;
    }
    fRightCrop = (float)c / (float)100.0;

    c = ReadInt("Enter Image Bottom Crop (%% of Height, Default 100 %%) : ", 100);
    if (c < 0 || c > 100)
    {
        c = 100;
    }
    fBottomCrop = (float)c / (float)100.0;
}

void CInput::ReadForceSettings(unsigned int &nForcePlateIndex, float afCorner[4][3])
{
    nForcePlateIndex = ReadInt("Enter Force Plate Index : ", 1);

    // Read Force Plate Parameters
    for (int nCorner = 0; nCorner < 4; nCorner++)
    {
        printf("Enter Force plate corner %d\nX : ", nCorner + 1);
        afCorner[nCorner][0] = ReadFloat("", 0.0);
        afCorner[nCorner][1] = ReadFloat("Y : ", 0.0);
        afCorner[nCorner][2] = ReadFloat("Z : ", 0.0);
    }
}

void CInput::Read6DSettings(unsigned int &color, float &maxResidual, unsigned int &minMarkersInBody, float &boneLengthTolerance, std::string &filterPreset)
{
    unsigned int colorTmp[3];

    colorTmp[0] = ReadInt("Enter color R (0-255): ", 0xff);
    colorTmp[1] = ReadInt("Enter color G (0-255): ", 0xff);
    colorTmp[2] = ReadInt("Enter color B (0-255): ", 0xff);
    color = colorTmp[0] | (colorTmp[1] << 8) | (colorTmp[2] << 16);
    maxResidual = ReadFloat("Enter max residual: ", 10);
    minMarkersInBody = ReadInt("Enter min markers in body: ", 3);
    boneLengthTolerance = ReadFloat("Enter bone length tolerance: ", 5);
    int preset = ReadInt("Enter filter preset(0-3):\n  0 - No filter\n  1 - Multi-purpose\n  2 - High stability\n  3 - Static pose\n", 0);
    switch (preset)
    {
    case 0: filterPreset = "No filter";
        break;
    case 1: filterPreset = "Multi-purpose";
        break;
    case 2: filterPreset = "High stability";
        break;
    case 3: filterPreset = "Static pose";
        break;
    default:
        filterPreset = "";
    }
}

void CInput::Read6DSettingsMesh(CRTProtocol::SSettings6DMesh &mesh)
{
    mesh.name = ReadString("Enter mesh name: ");
    mesh.position.fX = ReadFloat("Enter mesh position X: ", 0);
    mesh.position.fY = ReadFloat("Enter mesh position Y: ", 0);
    mesh.position.fZ = ReadFloat("Enter mesh position Z: ", 0);
    mesh.rotation.fX = ReadFloat("Enter mesh rotation X: ", 0);
    mesh.rotation.fY = ReadFloat("Enter mesh rotation Y: ", 0);
    mesh.rotation.fZ = ReadFloat("Enter mesh rotation Z: ", 0);
    mesh.scale = ReadFloat("Enter mesh scale: ", 1);
    mesh.opacity = ReadFloat("Enter mesh opacity: ", 1);
}

void CInput::Read6DSettingsOrigin(CRTProtocol::SOrigin &origin)
{
    origin.type = (CRTProtocol::EOriginType)ReadInt("Enter origin (0 = Global, 1 = Relative, 2 = Fixed)", 0);
    if (origin.type == CRTProtocol::RelativeOrigin)
    {
        origin.relativeBody = ReadInt("Enter relative body index: ", 0) + 1; // QTM body index starts at 1.
    }
    else if (origin.type == CRTProtocol::FixedOrigin)
    {
        origin.position.fX = ReadFloat("Enter origin position X: ", 0);
        origin.position.fY = ReadFloat("Enter origin position Y: ", 0);
        origin.position.fZ = ReadFloat("Enter origin position Z: ", 0);
        origin.rotation[0] = ReadFloat("Enter origin rotation matrix (1,1): ", 0);
        origin.rotation[1] = ReadFloat("Enter origin rotation matrix (1,2): ", 0);
        origin.rotation[2] = ReadFloat("Enter origin rotation matrix (1,3): ", 0);
        origin.rotation[3] = ReadFloat("Enter origin rotation matrix (2,1): ", 0);
        origin.rotation[4] = ReadFloat("Enter origin rotation matrix (2,2): ", 0);
        origin.rotation[5] = ReadFloat("Enter origin rotation matrix (2,3): ", 0);
        origin.rotation[6] = ReadFloat("Enter origin rotation matrix (3,1): ", 0);
        origin.rotation[7] = ReadFloat("Enter origin rotation matrix (3,2): ", 0);
        origin.rotation[8] = ReadFloat("Enter origin rotation matrix (3,3): ", 0);
    }
}

void CInput::Read6DSettingsPoints(std::vector<CRTProtocol::SBodyPoint> &points)
{
    CRTProtocol::SBodyPoint point;

    while (ReadYesNo("Enter point (y/n): ", true))
    {
        point.name = ReadString("Enter point name: ");
        point.fX = ReadFloat("Enter point position X: ", 0);
        point.fY = ReadFloat("Enter point position Y: ", 0);
        point.fZ = ReadFloat("Enter point position Z: ", 0);
        point.virtual_ = ReadYesNo("Is point virtual? (y/n): ", false);
        point.physicalId = ReadInt("Enter physical id: ", 0);
        points.push_back(point);
    }
}

bool CInput::ReadQTMCommand(EQTMCommand &eCommand)
{
    printf("Enter Command :\n");
    printf("  1 : New\n");
    printf("  2 : Close\n");
    printf("  3 : Start\n");
    printf("  4 : Start RT from file\n");
    printf("  5 : Stop\n");
    printf("  6 : Load\n");
    printf("  7 : Save\n");
    printf("  8 : LoadProject\n");
    printf("  9 : GetCaptureC3D\n");
    printf("  a : GetCaptureQTM\n");
    printf("  b : Trig\n");
    printf("  c : SetQTMEvent\n");
    printf("  d : Reprocess\n");
    printf("  e : Calibrate\n");
    printf("  f : Refine Calibration\n");
    printf("  q : Quit\n\n");
    printf("Select 1 - e : ");

    eCommand = (EQTMCommand)(ReadChar('0') - '0');
    switch (eCommand)
    {
        case 49 : // a
            eCommand = GetCaptureQTM;
            break;
        case 50 : // b
            eCommand = Trig;
            break;
        case 51 : // c
            eCommand = SetQTMEvent;
            break;
        case 52 : // d
            eCommand = Reprocess;
            break;
        case 53:  // e
            eCommand = Calibrate;
            break;
        case 54:  // f
            eCommand = CalibrateRefine;
            break;
        default :
            break;
    }
    printf("\n");
    if (eCommand < 1 || eCommand > CalibrateRefine)
    {
        return false;
    }
    return true;
}

std::string CInput::ReadString(const std::string& text)
{
    std::string str;

    printf(text.c_str());
    std::getline(std::cin, str);
    
    return str;
}

float CInput::ReadFloat(const std::string& text, float fDefault)
{
    char  pStr[128];
    float fVal;

    printf(text.c_str());

    gets_s(pStr, 32);
    if (sscanf_s(pStr, "%f", &fVal) != 1)
    {
        printf("%f\n", fDefault);
        return fDefault;
    }
    return fVal;
}

bool CInput::ReadYesNo(const std::string& text, bool bDefault)
{
    char c;

    printf(text.c_str());

    do
    {
        c = _getch();
    } while (c == 0);

    if (c == 'y' || c == 'Y')
    {
        bDefault = true;
    }
    else if (c == 'n' || c == 'N')
    {
        bDefault = false;
    }

    printf("%s\n", bDefault ? "yes" : "no");
    return bDefault;
}

int CInput::ReadInt(const std::string& text, int nDefault)
{
    char pStr[128];
    int  nVal;

    printf(text.c_str());

    gets_s(pStr, 32);
    if (sscanf_s(pStr, "%d", &nVal) != 1)
    {
        printf("%d\n", nDefault);
        return nDefault;
    }
    return nVal;
}

char CInput::ReadChar(char cDefault, bool bShowInput)
{
    char c;

    do 
    {
        c = _getch();
    } while (c == 0);
    
    if (c == 13)
    {
        c = cDefault;
    }
    if (bShowInput)
    {
        printf("%c", c);
    }
    printf("\n");
    return c;
}

unsigned short CInput::ReadPort(int nDefault)
{
    char pStr[128];
    int  nVal;

    printf("Enter port number (1024 - 65535) (Default %d) : ", nDefault);

    do 
    {
        gets_s(pStr, 32);
        if (sscanf_s(pStr, "%d", &nVal) != 1)
        {
            printf("%d\n", nDefault);
            return nDefault;
        }
        if (nVal < 1024 || nVal > 65535)
        {
            printf("Enter port number must be greater than 1024 and less than 65535");
        }
    } while (nVal < 1024 || nVal > 65535);

    return nVal;
}

bool CInput::ReadUseScrolling(bool &bOutputModeScrolling)
{
    bOutputModeScrolling = false;

    printf("Select Output Mode :\n\n");
    printf("1 : Show current frame (Default)\n");
    printf("2 : Scroll frames\n");
    printf("Q : Quit\n\n");

    char ch = ReadChar('0');

    if (ch == 'q' || ch == 'Q')
    {
        return false;
    }

    ch -= '0';

    if (ch == 2)
    {
        bOutputModeScrolling = true;
    }
    return true;
}