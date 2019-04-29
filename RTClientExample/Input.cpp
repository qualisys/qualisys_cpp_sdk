#include "Input.h"
#include <conio.h>


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
        printf("7 : View Settings (General, 3D, 6DOF, GazeVector, Analog, Force, Image, Skeleton)\n");
    }
    if (mnMajorVersion > 1 || mnMinorVersion > 6)
    {
        printf("8 : Change Settings (General, Force, Image)\n");
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
            printf("1 : Change General Settings (default)\n");
            printf("2 : Change External Time Base Settings\n");
            printf("3 : Change Processing Actions Settings\n");
            printf("4 : Change Camera Settings\n");
            printf("5 : Change Camera Sync Out Settings\n");
            printf("6 : Change Image Settings\n");
            printf("7 : Change Force Settings\n");
            printf("Q : Quit\n\n");

            nSelection = (int)(ReadChar('1') - '0');

            if (nSelection >= 1 && nSelection <= 7)
            {
                eOperation = (EOperation)(nSelection + ChangeGeneralSystemSettings - 1);
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
            printf("Enter Frequency: ");
            nArg = ReadInt(1);
            printf("\n\n");
            return true;
        }
        if (nSelection == 3)
        {
            printf("Enter Frequency Divisor: ");
            nArg = ReadInt(1);
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

        printf("Read all channels (y/n)\n");
        if (!ReadYesNo(true))
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
        printf("c: Analog\n");
        if (mnMajorVersion > 1 || mnMinorVersion > 0)
        {
            printf("d: Analog Single\n");
        }
        printf("e: Force\n");
        if (mnMajorVersion > 1 || mnMinorVersion > 7)
        {
            printf("f: Force Single\n");
        }
        if (mnMajorVersion > 1 || mnMinorVersion > 7)
        {
            printf("g: Image\n");
        }
        if (mnMajorVersion > 1 || mnMinorVersion > 16)
        {
            printf("h: Timecode\n");
        }
        if (mnMajorVersion > 1 || mnMinorVersion > 18)
        {
            printf("i: Skeleton Local\n");
            printf("j: Skeleton Global\n");
        }
        printf("k: Multiple Components\n");
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
        case 'c' :
            nComponentType = CRTProtocol::cComponentAnalog;
            break;
        case 'd' :
            nComponentType = CRTProtocol::cComponentAnalogSingle;
            break;
        case 'e' :
            nComponentType = CRTProtocol::cComponentForce;
            break;
        case 'f' :
            nComponentType = CRTProtocol::cComponentForceSingle;
            break;
        case 'g' :
            nComponentType = CRTProtocol::cComponentImage;
            break;
        case 'h':
            nComponentType = CRTProtocol::cComponentTimecode;
            break;
        case 'i':
            nComponentType = CRTProtocol::cComponentSkeleton;
            skeletonGlobalReferenceFrame = false;
            break;
        case 'j':
            nComponentType = CRTProtocol::cComponentSkeleton;
            skeletonGlobalReferenceFrame = true;
            break;
        case 'k' :
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
    printf("\nPerform 2D Noise Test (y/n)?\n");
    return ReadYesNo(false);
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

void CInput::ReadSystemSettings(unsigned int &nCaptureFrequency, float &fCaptureTime, bool &bExternalTrigger, bool& trigNO, bool& trigNC, bool& trigSoftware)
{
    printf("Enter Capture Frequency (Hz) : ");
    nCaptureFrequency = ReadInt(20);

    printf("Enter Capture Time (seconds) : ");
    fCaptureTime = ReadFloat(1.0);
    
    if (mnMajorVersion > 1 || mnMinorVersion > 14)
    {
        printf("Enter Start on Trig NO (y/n)?\n");
        trigNO = ReadYesNo(false);

        printf("Enter Start Trig NC (y/n)?\n");
        trigNC = ReadYesNo(false);

        printf("Enter Start Software trigger (y/n)?\n");
        trigSoftware = ReadYesNo(false);
    }
    else
    {
        printf("Enter Start on External Trigger (y/n)?\n");
        bExternalTrigger = ReadYesNo(false);
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
        if (!ReadYesNo(true))
        {
            continue;
        }

        if (mnMajorVersion > 1 || mnMinorVersion > 13)
        {
            printf("\n2D pre-processing (y/n)? ");
            if (ReadYesNo(true))
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
            printf("TwinSystem Merge (y/n)? ");
            if (ReadYesNo(true))
            {
                *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingTwinSystemMerge);
            }

            printf("Spline Fill (y/n)? ");
            if (ReadYesNo(true))
            {
                *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingSplineFill);
            }
        }

        printf("AIM (y/n)? ");
        if (ReadYesNo(true))
        {
            *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingAIM);
        }

        printf("6DOF Tracking (y/n)? ");
        if (ReadYesNo(true))
        {
            *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::Processing6DOFTracking);
        }

        printf("Force (y/n)? ");
        if (ReadYesNo(true))
        {
            *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingForceData);
        }

        printf("Gaze Vector (y/n)? ");
        if (ReadYesNo(true))
        {
            *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingGazeVector);
        }

        if (i != 1) // Not RtProcessingSettings
        {
            printf("Export TSV (y/n)? ");
            if (ReadYesNo(true))
            {
                *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingExportTSV);
            }

            printf("Export C3D (y/n)? ");
            if (ReadYesNo(true))
            {
                *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingExportC3D);
            }

            printf("Export MATLAB File (y/n)? ");
            if (ReadYesNo(true))
            {
                *processingActions[i] = (CRTProtocol::EProcessingActions)(*processingActions[i] + CRTProtocol::ProcessingExportMatlabFile);
            }

            printf("Export AVI File (y/n)? ");
            if (ReadYesNo(true))
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
    printf("Enable External Time Base (y/n)? ");
    bEnabled = ReadYesNo(false);

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
            printf("Signal Mode Periodic (y/n)? ");
            bSignalModePeriodic = ReadYesNo(true);
        }

        if ((nSignalSource == 0 || nSignalSource == 1 || nSignalSource == 2 || nSignalSource == 3) && bSignalModePeriodic)
        {
            printf("Enter Frequency Multiplier : ");
            nMultiplier = ReadInt(1);

            printf("Enter Frequency Divisor : ");
            nDivisor = ReadInt(1);

            if (nSignalSource == 0 || nSignalSource == 1 || nSignalSource == 3)
            {
                printf("Enter Frequency Tolerance (ppm): ");
                nFrequencyTolerance = ReadInt(1000);
            }

            printf("Enter Nominal Frequency (Hz) : ");
            fNominalFrequency = ReadFloat(0);
        }

        if (nSignalSource == 0 || nSignalSource == 3)
        {
            printf("Negative Edge (y/n)? ");
            bNegativeEdge = ReadYesNo(true);
        }

        printf("Enter Signal Shutter Delay (us) : ");
        nFrequencyTolerance = ReadInt(0);

        if ((nSignalSource == 0 || nSignalSource == 1 || nSignalSource == 3) && !bSignalModePeriodic)
        {
            printf("Non Periodic Timeout (s) : ");
            fNonPeriodicTimeout = ReadFloat(1);
        }
    }
}

void CInput::ReadCameraSettings(unsigned int &nCameraId,        int   &nMode,            CRTProtocol::EVideoResolution &videoResolution, CRTProtocol::EVideoAspectRatio &videoAspectRatio,
                                unsigned int &nVideoFrequency,  float &fVideoExposure,   float &fVideoFlashTime,
                                float        &fMarkerExposure,  float &fMarkerThreshold, int   &nRotation,
                                float        &fFocus,           float &fAperture,        bool  &autoExposure,
                                float        &exposureCompensation, bool &autoWhiteBalance)
{
    printf("\nEnter Camera ID : ");
    nCameraId = ReadInt(1);

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
        printf("Enter Marker Exposure (us) (Default 300 us): ");
        fMarkerExposure = ReadFloat(300);

        printf("Enter Marker Threshold (50 - 900) (Default 150) : ");
        fMarkerThreshold = ReadFloat(150);
    }
    if (nMode == 2)
    {
        printf("Enter Video Frequency (Default 24 Hz) : ");
        nVideoFrequency = ReadInt(24);

        printf("Enter Video Exposure (us) (Default 300 us) : ");
        fVideoExposure = ReadFloat(300);

        printf("Enter Video Flash Time (us) (Default 300 us) : ");
        fVideoFlashTime = ReadFloat(300);
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

    printf("Enter Camera Rotation (degrees) (Default 0 degrees): ");
    nRotation = ReadInt(0);
    nRotation = nRotation - (nRotation % 90);
    if (nRotation < 0 || nRotation > 270)
    {
        nRotation = 0;
    }

    printf("Enter Lens Focus: ");
    fFocus = ReadFloat(5);
    printf("Enter Lens Aperture: ");
    fAperture = ReadFloat(5);

    printf("Enable Auto Exposure: ");
    autoExposure = ReadYesNo(false);
    if (autoExposure)
    {
        printf("Enter Exposure Compensation: ");
        exposureCompensation = ReadFloat(0);
    }

    printf("Enable Auto White Balance: ");
    autoWhiteBalance = ReadYesNo(true);
}


void CInput::ReadCameraSyncOutSettings(unsigned int &nCameraId, int &portNumber, int   &nSyncOutMode, unsigned int &nSyncOutValue,
                                        float &fSyncOutDutyCycle, bool  &bSyncOutNegativePolarity)
{
    printf("\nEnter Camera ID : ");
    nCameraId = ReadInt(1);

    printf("Enter Sync out port number (1-3) ");
    portNumber = ReadInt(1);

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
            nSyncOutValue = ReadInt(1000);

            printf("Enter Sync Out Duty Cycle (%%) : ");
            fSyncOutDutyCycle = ReadFloat(0.5);
        }
    }

    if (nSyncOutMode < 6)
    {
        printf("Negative Polarity (y/n)? ");
        bSyncOutNegativePolarity = ReadYesNo(true);
    }
}


void CInput::ReadImageSettings(unsigned int &nCameraId, bool &bEnable, int &nFormat, unsigned int &nWidth,
                               unsigned int &nHeight, float &fLeftCrop, float &fTopCrop, float &fRightCrop, float &fBottomCrop)
{
    int c;

    printf("Enter Camera ID (Default 1): ");
    nCameraId = ReadInt(1);

    printf("Enter Enable Camera (y/n)? ");
    bEnable = ReadYesNo(true);

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
    printf("Enter Image Width (Default 320): ");
    nWidth = ReadInt(320);
    
    printf("Enter Image Height (Default 200) : ");
    nHeight = ReadInt(200);
    
    printf("Enter Image Left Crop (%% of Width, Default 0 %%) : ");
    c = ReadInt(0);
    if (c < 0 || c > 100)
    {
        c = 0;
    }
    fLeftCrop = (float)c / (float)100.0;

    printf("Enter Image Top Crop (%% of Height, Default 0 %%) : ");
    c = ReadInt(0);
    if (c < 0 || c > 100)
    {
        c = 0;
    }
    fTopCrop = (float)c / (float)100.0;

    printf("Enter Image Right Crop (%% of Width, Default 100 %%) : ");
    c = ReadInt(100);
    if (c < 0 || c > 100)
    {
        c = 100;
    }
    fRightCrop = (float)c / (float)100.0;

    printf("Enter Image Bottom Crop (%% of Height, Default 100 %%) : ");
    c = ReadInt(100);
    if (c < 0 || c > 100)
    {
        c = 100;
    }
    fBottomCrop = (float)c / (float)100.0;
}

void CInput::ReadForceSettings(unsigned int &nForcePlateIndex, float afCorner[4][3])
{
    printf("Enter Force Plate Index : ");
    nForcePlateIndex = ReadInt(1);

    // Read Force Plate Parameters
    for (int nCorner = 0; nCorner < 4; nCorner++)
    {
        printf("Enter Force plate corner %d\nX : ", nCorner + 1);
        afCorner[nCorner][0] = ReadFloat(0.0);
        printf("Y : ");
        afCorner[nCorner][1] = ReadFloat(0.0);
        printf("Z : ");
        afCorner[nCorner][2] = ReadFloat(0.0);
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
    printf("  q : Quit\n\n");
    printf("Select 1 - c : ");

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
        default :
            break;
    }
    printf("\n");
    if (eCommand < 1 || eCommand > Reprocess)
    {
        return false;
    }
    return true;
}

void CInput::ReadFileName(char* pStr, int nStrLen)
{
    printf("Enter Name : ");
    gets_s(pStr, nStrLen);
}

float CInput::ReadFloat(float fDefault)
{
    char  pStr[128];
    float fVal;

    gets_s(pStr, 32);
    if (sscanf_s(pStr, "%f", &fVal) != 1)
    {
        printf("%f\n", fDefault);
        return fDefault;
    }
    return fVal;
}

bool CInput::ReadYesNo(bool bDefault)
{
    char c;

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

int CInput::ReadInt(int nDefault)
{
    char pStr[128];
    int  nVal;

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