#include "Operations.h"

COperations::COperations(CInput* poInput, COutput* poOutput, CRTProtocol* poRTProtocol)
{
    mpoInput      = poInput;
    mpoOutput     = poOutput;
    mpoRTProtocol = poRTProtocol;
}


void COperations::DiscoverRTServers(char* tServerAddr, int nServerAddrLen, unsigned short* pnPort)
{
	unsigned int nAddr;
	unsigned short nBasePort;
	std::string message;
    if (mpoRTProtocol->DiscoverRTServer(0, false)) // Use random UDP server port.
    {
        unsigned int nResponsCount = mpoRTProtocol->GetNumberOfDiscoverResponses();

        if (nResponsCount == 0)
        {
            printf("No QTM RT Servers found.\n\n");
            return;
        }

        printf("QTM RT Servers found:\n\n");

        for (unsigned int nResponse = 0; nResponse < nResponsCount; nResponse++)
        {
            if (mpoRTProtocol->GetDiscoverResponse(nResponse, nAddr, nBasePort, message))
            {
                if (nBasePort > 0)
                {
                    char tAddrStr[32];
                    sprintf_s(tAddrStr, sizeof(tAddrStr), "%d.%d.%d.%d", 0xff & nAddr, 0xff & (nAddr >> 8), 0xff & (nAddr >> 16), 0xff & (nAddr >> 24));
                    if (tServerAddr != NULL)
                    {
                        printf("%-2d : ", nResponse + 1);
                    }
                    printf("%-15s %d - %s\n", tAddrStr, nBasePort, message.c_str());
                }
                else
                {
                    printf("%d.%d.%d.%d\t- %s\n", 0xff & nAddr, 0xff & (nAddr >> 8), 0xff & (nAddr >> 16), 0xff & (nAddr >> 24), message.c_str());
                }
            }
            else
            {
                break;
            }
        };
    }
    if (tServerAddr != NULL)
    {
        printf("\nSelect QTM RT Server to connect to (1 - %d): ", mpoRTProtocol->GetNumberOfDiscoverResponses());
        int nSelection = mpoInput->ReadInt("", 0);
        if (mpoRTProtocol->GetDiscoverResponse(nSelection - 1, nAddr, *pnPort, message))
        {
            sprintf_s(tServerAddr, nServerAddrLen, "%d.%d.%d.%d", 0xff & nAddr, 0xff & (nAddr >> 8), 0xff & (nAddr >> 16), 0xff & (nAddr >> 24));
        }
        else
        {
            tServerAddr[0] = 0;
        }
    }

    printf("\n");

} // DiscoverRTServers


void COperations::MonitorEvents()
{
    CRTPacket::EEvent       eEvent;
    CRTPacket::EPacketType  ePacketType;
    unsigned int            nEventCount = 0;

    printf("\n\nWaiting for QTM Events. Abort with any key.\n\n");

    while (mpoInput->CheckKeyPressed() == false)
    {
        auto response = mpoRTProtocol->Receive(ePacketType, false);

        if (response == CNetwork::ResponseType::error || ePacketType == CRTPacket::PacketError)
        {
            break;
        }
        if (response == CNetwork::ResponseType::success && ePacketType == CRTPacket::PacketEvent)
        {
            printf("#%d ", nEventCount++);

            if (mpoRTProtocol->GetRTPacket()->GetEvent(eEvent))
            {
                mpoOutput->PrintEvent(eEvent);
            }
        }
    }
} // MonitorEvents


void COperations::ViewSettings()
{
    if (mpoRTProtocol->ReadGeneralSettings() == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read general settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else
    {
        mpoOutput->PrintGeneralSettings(mpoRTProtocol);
    }
 
    if (mpoRTProtocol->ReadCalibrationSettings() == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read calibration settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else
    {
        mpoOutput->PrintCalibrationSettings(mpoRTProtocol);
    }

    bool bDataAvailable;
    if (mpoRTProtocol->Read3DSettings(bDataAvailable) == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read 3D settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else if (bDataAvailable)
    {
        mpoOutput->Print3DSettings(mpoRTProtocol);
    }

    if (mpoRTProtocol->Read6DOFSettings(bDataAvailable) == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read 6DOF settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else if (bDataAvailable)
    {
        mpoOutput->Print6DOFSettings(mpoRTProtocol);
    }

    if (mpoRTProtocol->ReadGazeVectorSettings(bDataAvailable) == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read Gaze vector settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else if (bDataAvailable)
    {
        mpoOutput->PrintGazeVectorSettings(mpoRTProtocol);
    }

    if (mpoRTProtocol->ReadEyeTrackerSettings(bDataAvailable) == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read eye tracker settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else if (bDataAvailable)
    {
        mpoOutput->PrintEyeTrackerSettings(mpoRTProtocol);
    }

    if (mpoRTProtocol->ReadAnalogSettings(bDataAvailable) == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read Analog settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else if (bDataAvailable)
    {
        mpoOutput->PrintAnalogSettings(mpoRTProtocol);
    }

    if (mpoRTProtocol->ReadForceSettings(bDataAvailable) == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read Force settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else if (bDataAvailable)
    {
        mpoOutput->PrintForceSettings(mpoRTProtocol);
    }

    if (mpoRTProtocol->ReadImageSettings(bDataAvailable) == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read Image settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else if (bDataAvailable)
    {
        mpoOutput->PrintImageSettings(mpoRTProtocol);
    }

    if (mpoRTProtocol->ReadSkeletonSettings(bDataAvailable, mSkeletonGlobalReferenceFrame) == false)
    {
        if (mpoRTProtocol->Connected())
        {
            printf("Read Skeleton settings failed. %s\n\n", mpoRTProtocol->GetErrorString());
        }
    }
    else if (bDataAvailable)
    {
        mpoOutput->PrintSkeletonSettings(mpoRTProtocol, mSkeletonGlobalReferenceFrame);
    }
} // ViewSettings


void COperations::ChangeSettings(CInput::EOperation eOperation)
{
    unsigned int              nCameraId;
    bool                      bEnable;
    int                       nImageFormat;
    unsigned int              nWidth;
    unsigned int              nHeight;
    float                     fLeftCrop;
    float                     fTopCrop;
    float                     fRightCrop;
    float                     fBottomCrop;
    char                      pPassword[31];
    bool                      bGotControl;

    pPassword[0] = 0;
    bGotControl  = false;

    do 
    {
        // Take control over QTM
        if (mpoRTProtocol->TakeControl(pPassword))
        {
            bGotControl = true;

            if (eOperation == CInput::ChangeGeneralSystemSettings)
            {
                printf("\n\nInput General Settings\n\n");

                unsigned int nCaptureFrequency;
                float fCaptureTime;
                bool bExternalTrigger;
                bool startOnTrigNO;
                bool startOnTrigNC;
                bool startOnTrigSoftware;

                mpoInput->ReadGeneralSettings(nCaptureFrequency, fCaptureTime, bExternalTrigger, startOnTrigNO, startOnTrigNC, startOnTrigSoftware);

                if (mpoRTProtocol->SetGeneralSettings(&nCaptureFrequency, &fCaptureTime, &bExternalTrigger, &startOnTrigNO, &startOnTrigNC, &startOnTrigSoftware, NULL, NULL, NULL))
                {
                    printf("Change General Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change General Settings Failed\n\n");
                }
            }

            if (eOperation == CInput::ChangeExtTimebaseSettings)
            {
                bool         bEnabled;
                int          nSignalSource;
                bool         bSignalModePeriodic;
                unsigned int nMultiplier;
                unsigned int nDivisor;
                unsigned int nFrequencyTolerance;
                float        fNominalFrequency;
                bool         bNegativeEdge;
                unsigned int nSignalShutterDelay;
                float        fNonPeriodicTimeout;

                printf("\n\nInput External Time Base Settings\n\n");

                mpoInput->ReadExtTimeBaseSettings(bEnabled,          nSignalSource, bSignalModePeriodic,
                                                         nMultiplier,       nDivisor,      nFrequencyTolerance,
                                                         fNominalFrequency, bNegativeEdge, nSignalShutterDelay,
                                                         fNonPeriodicTimeout);

                if (mpoRTProtocol->SetExtTimeBaseSettings(&bEnabled,            (CRTProtocol::ESignalSource*)&nSignalSource,
                                                         &bSignalModePeriodic, &nMultiplier,       &nDivisor,      
                                                         &nFrequencyTolerance, &fNominalFrequency, &bNegativeEdge,
                                                         &nSignalShutterDelay, &fNonPeriodicTimeout))
                {
                    printf("Change External Time Base Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change External Time Base Settings Failed\n\n");
                }
            }

            if (eOperation == CInput::ChangeExtTimestampSettings)
            {
                printf("\n\nInput Timestamp Settings\n\n");

                CRTProtocol::SSettingsGeneralExternalTimestamp timestampSettings;
                mpoInput->ReadTimestampSettings(timestampSettings);
                if (mpoRTProtocol->SetExtTimestampSettings(timestampSettings))
                {
                    printf("Change Timestamp Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change Timestamp Settings Failed\n\n");
                }
            }

            if (eOperation == CInput::ChangeProcessingActionsSettings)
            {
                CRTProtocol::EProcessingActions eProcessingActions = CRTProtocol::ProcessingNone;
                CRTProtocol::EProcessingActions eRtProcessingActions = CRTProtocol::ProcessingNone;
                CRTProtocol::EProcessingActions eReprocessingActions = CRTProtocol::ProcessingNone;

                mpoInput->ReadProcessingActionsSettings(eProcessingActions, eRtProcessingActions, eReprocessingActions);

                if (mpoRTProtocol->SetGeneralSettings(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &eProcessingActions, &eRtProcessingActions, &eReprocessingActions))
                {
                    printf("Change General Settings Processing Actions Succeeded\n\n");
                }
                else
                {
                    printf("Change General Settings Processing Actions Failed\n\n");
                }
            }

            if (eOperation == CInput::ChangeCameraSettings)
            {
                printf("\n\nInput Camera Settings\n\n");

                int           nMode;
                unsigned int  nVideoFrequency  = -1;
                float         fVideoExposure   = -1;
                float         fVideoFlashTime  = -1;
                float         fMarkerExposure  = -1;
                float         fMarkerThreshold = -1;
                CRTProtocol::EVideoResolution videoResolution;
                CRTProtocol::EVideoAspectRatio videoAspectRatio;
                CRTProtocol::EVideoResolution* pVideoResolution;
                CRTProtocol::EVideoAspectRatio* pVideoAspectRatio;
                int           nRotation;
                unsigned int* pnVideoFrequency;
                float*        pfVideoExposure;
                float*        pfVideoFlashTime;
                float*        pfMarkerExposure;
                float*        pfMarkerThreshold;
                float         fFocus;
                float         fAperture;
                bool          autoExposure;
                float         exposureCompensation;
                bool          autoWhiteBalance;

                mpoInput->ReadCameraSettings(nCameraId, nMode, videoResolution, videoAspectRatio, nVideoFrequency, 
                                             fVideoExposure, fVideoFlashTime, fMarkerExposure,
                                             fMarkerThreshold, nRotation, fFocus, fAperture, autoExposure, exposureCompensation, autoWhiteBalance);

                pnVideoFrequency  = (nVideoFrequency == -1) ? NULL : &nVideoFrequency;
                pfVideoExposure   = (fVideoExposure == -1) ? NULL : &fVideoExposure;
                pfVideoFlashTime  = (fVideoFlashTime == -1) ? NULL : &fVideoFlashTime;
                pfMarkerExposure  = (fMarkerExposure == -1) ? NULL : &fMarkerExposure;
                pfMarkerThreshold = (fMarkerThreshold == -1) ? NULL : &fMarkerThreshold;
                pVideoResolution = (videoResolution == CRTProtocol::VideoResolutionNone) ? NULL : &videoResolution;
                pVideoAspectRatio = (videoAspectRatio == CRTProtocol::VideoAspectRatioNone) ? NULL : &videoAspectRatio;

                if (mpoRTProtocol->SetCameraSettings(nCameraId, (CRTProtocol::ECameraMode*)&nMode, 
                                                     pfMarkerExposure, pfMarkerThreshold, &nRotation))
                {
                    printf("Change Camera Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change Camera Settings Failed\n\n");
                }

                if (mpoRTProtocol->SetCameraVideoSettings(nCameraId, pVideoResolution, pVideoAspectRatio, pnVideoFrequency, pfVideoExposure, pfVideoFlashTime))
                {
                    printf("Change Camera Video Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change Camera Video Settings Failed\n\n");
                }

                if (mpoRTProtocol->SetCameraLensControlSettings(nCameraId, fFocus, fAperture))
                {
                    printf("Change Camera Lens Control Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change Camera Lens Control Settings Failed\n\n");
                }

                if (mpoRTProtocol->SetCameraAutoExposureSettings(nCameraId, autoExposure, exposureCompensation))
                {
                    printf("Change Camera Lens Control Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change Camera Lens Control Settings Failed\n\n");
                }

                if (mpoRTProtocol->SetCameraAutoWhiteBalance(nCameraId, autoWhiteBalance))
                {
                    printf("Change Camera Auto White Balance Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change Camera Auto White Balance Settings Failed\n\n");
                }
            }

            if (eOperation == CInput::ChangeCameraSyncOutSettings)
            {
                printf("\n\nInput Camera Sync Out Settings\n\n");

                int          portNumber;
                int          nSyncOutMode;
                unsigned int nSyncOutValue;
                float        fSyncOutDutyCycle;
                bool         bSyncOutNegativePolarity;

                mpoInput->ReadCameraSyncOutSettings(nCameraId, portNumber, nSyncOutMode, nSyncOutValue, fSyncOutDutyCycle,
                                                     bSyncOutNegativePolarity);

                if (mpoRTProtocol->SetCameraSyncOutSettings(nCameraId, portNumber, (CRTProtocol::ESyncOutFreqMode*)&nSyncOutMode,
                                                           &nSyncOutValue, &fSyncOutDutyCycle,
                                                           &bSyncOutNegativePolarity))
                {
                    printf("Change Camera Sync Out Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change Camera Syn Out Settings Failed\n\n");
                }
            }

            if (eOperation == CInput::ChangeImageSettings)
            {
                printf("\n\nInput Image Settings\n\n");

                mpoInput->ReadImageSettings(nCameraId, bEnable, nImageFormat, nWidth, nHeight,
                                            fLeftCrop, fTopCrop, fRightCrop, fBottomCrop);

                if (mpoRTProtocol->SetImageSettings(nCameraId, &bEnable, (CRTPacket::EImageFormat*)&nImageFormat, &nWidth,
                                                    &nHeight, &fLeftCrop, &fTopCrop, &fRightCrop, &fBottomCrop))
                {
                    printf("Change Image Settings Succeeded\n\n");
                }
            }

            if (eOperation == CInput::ChangeForceSettings)
            {
                float        aCorner[4][3];
                unsigned int nPlateID;

                printf("\n\nInput Force Settings\n\n");

                mpoInput->ReadForceSettings(nPlateID, aCorner);

                if (mpoRTProtocol->SetForceSettings(nPlateID,
                                                    (CRTProtocol::SPoint*)aCorner[0], (CRTProtocol::SPoint*)aCorner[1],
                                                    (CRTProtocol::SPoint*)aCorner[2], (CRTProtocol::SPoint*)aCorner[3]))
                {
                    printf("Change Force Settings Succeeded\n\n");
                }
            }

            if (eOperation == CInput::Change6dSettings)
            {
                std::vector<CRTProtocol::SSettings6DOFBody> bodiesSettings;
                CRTProtocol::SSettings6DOFBody bodySettings;

                printf("\n\nInput 6D Settings\n\n");

                bodySettings.name = mpoInput->ReadString("Enter rigid body name: ");
                mpoInput->Read6DSettings(bodySettings.color, bodySettings.maxResidual, bodySettings.minMarkersInBody, bodySettings.boneLengthTolerance, bodySettings.filterPreset);
                
                if (mpoInput->ReadYesNo("Change: Mesh? (y/n): ", false))
                {
                    mpoInput->Read6DSettingsMesh(bodySettings.mesh);
                }
                mpoInput->Read6DSettingsOrigin(bodySettings.origin);
                mpoInput->Read6DSettingsPoints(bodySettings.points);

                bodiesSettings.push_back(bodySettings);

                if (mpoRTProtocol->Set6DOFBodySettings(bodiesSettings))
                {
                    printf("Change 6D Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change 6D Settings Failed. %s\n\n", mpoRTProtocol->GetErrorString());
                }
            }

            if (eOperation == CInput::ChangeSkeletonSettings)
            {
                std::vector<CRTProtocol::SSettingsSkeletonHierarchical> skeletonsSettings;
                CRTProtocol::SSettingsSkeletonHierarchical skeletonSettings;

                printf("\n\nRename skeletons\n\n");

                std::string name = mpoInput->ReadString("Enter skeleton suffix: ");

                bool available = false;
                if (!mpoRTProtocol->ReadSkeletonSettings(available))
                {
                    printf("Read skeleton Settings Failed. %s\n\n", mpoRTProtocol->GetErrorString());
                }
                mpoRTProtocol->GetSkeletons(skeletonsSettings);
                for (auto& skeleton : skeletonsSettings)
                {
                    skeleton.name += "_";
                    skeleton.name += name;
                }

                if (mpoRTProtocol->SetSkeletonSettings(skeletonsSettings))
                {
                    printf("Change skeleton Settings Succeeded\n\n");
                }
                else
                {
                    printf("Change skeleton Settings Failed. %s\n\n", mpoRTProtocol->GetErrorString());
                }
            }

            if (mpoRTProtocol->ReleaseControl() == false)
            {
                printf("Failed to release QTM control.\n\n");
            }
        }
        else
        {
            if (strncmp("Wrong or missing password", mpoRTProtocol->GetErrorString(), 25) == 0)
            {
                mpoInput->ReadClientControlPassword(pPassword, sizeof(pPassword));
                printf("\n");
            }
            else
            {
                printf("Failed to take control over QTM. %s\n\n", mpoRTProtocol->GetErrorString());
            }
        }
    } while (!bGotControl && pPassword[0] != 0);

} // ChangeSettings


void COperations::DataTransfer(CInput::EOperation operation)
{
    CRTPacket::EPacketType      ePacketType;
    unsigned int                nComponentType;
    char                        selectedAnalogChannels[256];
    CRTProtocol::EStreamRate    eStreamRate;
    int                         nRateArgument = 0;
    FILE*                       logfile = NULL;
    bool                        bStreamTCP, bStreamUDP, bLogToFile, bOnlyTimeAndFrameNumber;
    unsigned short              nUDPPort;
    char                        tUDPAddress[256];
    bool                        bOutputModeScrolling = false;

    mpoOutput->ResetCounters();

    if (operation == CInput::Noise2D)
    {
        nComponentType = CRTProtocol::cComponent2d;
        bStreamTCP = true;
        bStreamUDP = false;
        bLogToFile = false;
        bOnlyTimeAndFrameNumber = false;
        nUDPPort = 0;
        tUDPAddress[0] = 0;
        eStreamRate = CRTProtocol::RateAllFrames;

        mpoOutput->Reset2DNoiseCalc();
    }
    else
    {
        if (!mpoInput->ReadDataComponents(nComponentType, selectedAnalogChannels, sizeof(selectedAnalogChannels), mSkeletonGlobalReferenceFrame))
        {
            return;
        }

        if (!mpoInput->ReadDataTest(operation != CInput::Statistics, bStreamTCP, bStreamUDP, bLogToFile, bOnlyTimeAndFrameNumber, nUDPPort, tUDPAddress, sizeof(tUDPAddress)))
        {
            return;
        }
        if (operation == CInput::Statistics)
        {
            eStreamRate = CRTProtocol::RateAllFrames;
        }
        else if (bStreamTCP || bStreamUDP)
        {
            if (!mpoInput->ReadStreamRate(eStreamRate, nRateArgument))
            {
                return;
            }
        }
    }

    if (bLogToFile)
    {
        std::string filename;
        filename = mpoInput->ReadString("Enter Name : ");

        if (fopen_s(&logfile, filename.c_str(), "w") != 0)
        {
            printf("\n\nOpen/Create log file failed.\n");
            return;
        }
    }
    else
    {
        logfile = stdout;
        if (!mpoInput->ReadUseScrolling(bOutputModeScrolling))
        {
            return;
        }
    }
    Sleep(100);

    // Clear screen
    system("cls");

    if (logfile != stdout)
    {
        printf("\nMeasurement started. Press any key to abort.");
    }

    mpoRTProtocol->ReadGeneralSettings();

    bool bDataAvailable;

    if ((nComponentType & CRTProtocol::cComponent3d)         |
        (nComponentType & CRTProtocol::cComponent3dRes)      |
        (nComponentType & CRTProtocol::cComponent3dNoLabels) |
        (nComponentType & CRTProtocol::cComponent3dNoLabelsRes))
    {
        mpoRTProtocol->Read3DSettings(bDataAvailable);
    }

    if ((nComponentType & CRTProtocol::cComponent6d)      |
        (nComponentType & CRTProtocol::cComponent6dRes)   |
        (nComponentType & CRTProtocol::cComponent6dEuler) |
        (nComponentType & CRTProtocol::cComponent6dEulerRes))
    {
        mpoRTProtocol->Read6DOFSettings(bDataAvailable);
    }

    if ((nComponentType & CRTProtocol::cComponentGazeVector))
    {
        mpoRTProtocol->ReadGazeVectorSettings(bDataAvailable);
    }

    if ((nComponentType & CRTProtocol::cComponentEyeTracker))
    {
        mpoRTProtocol->ReadEyeTrackerSettings(bDataAvailable);
    }

    if ((nComponentType & CRTProtocol::cComponentAnalog) |
        (nComponentType & CRTProtocol::cComponentAnalogSingle))
    {
        mpoRTProtocol->ReadAnalogSettings(bDataAvailable);
    }

    if ((nComponentType & CRTProtocol::cComponentForce) |
        (nComponentType & CRTProtocol::cComponentForceSingle))
    {
        mpoRTProtocol->ReadForceSettings(bDataAvailable);
    }

    if (nComponentType == CRTProtocol::cComponentImage)
    {
        if (mpoRTProtocol->ReadImageSettings(bDataAvailable))
        {
            if (TakeQTMControl())
            {
                unsigned int nCameraId         = 1;
                CRTProtocol::ECameraMode nMode = CRTProtocol::ModeVideo;

                CRTPacket::EEvent eEvent;
                bool              bConnected = true;

                mpoRTProtocol->GetState(eEvent);
                if (eEvent == CRTPacket::EventConnectionClosed)
                {
                    bConnected = false;
                    if (mpoRTProtocol->NewMeasurement())
                    {
                        bConnected = true;
                    }
                }
                
                if (bConnected)
                {
                    if (mpoRTProtocol->SetCameraSettings(nCameraId, &nMode, NULL, NULL, NULL))
                    {
                        CRTPacket::EImageFormat nImageFormat = CRTPacket::FormatJPG;
                        bool bEnable;
                        unsigned int nWidth;
                        unsigned int nHeight;
                        float fLeftCrop;
                        float fTopCrop;
                        float fRightCrop;
                        float fBottomCrop;

                        mpoInput->ReadImageSettings(nCameraId, bEnable, (int&)nImageFormat, nWidth, nHeight,
                            fLeftCrop, fTopCrop, fRightCrop, fBottomCrop);
                        
                        system("cls");

                        if (mpoRTProtocol->SetImageSettings(nCameraId, &bEnable, &nImageFormat, &nWidth,
                            &nHeight, &fLeftCrop, &fTopCrop, &fRightCrop, &fBottomCrop) == false)
                        {
                            printf("Change Image Settings Failed. %s\n\n", mpoRTProtocol->GetErrorString());
                        }
                    }
                    else
                    {
                        printf("Change General Settings Failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                }
                else
                {
                    printf("Unable to start new measurement. %s\n\n", mpoRTProtocol->GetErrorString());
                }

                if (ReleaseQTMControl() == false)
                {
                    return;
                }
            }
        }
    }

    if ((nComponentType & CRTProtocol::cComponentSkeleton))
    {
        mpoRTProtocol->ReadSkeletonSettings(bDataAvailable, mSkeletonGlobalReferenceFrame);
    }

    auto componentOptions = CRTProtocol::SComponentOptions();
    componentOptions.mAnalogChannels = selectedAnalogChannels;
    componentOptions.mSkeletonGlobalData = mSkeletonGlobalReferenceFrame;

    bool bAbort = false;

    if (bStreamTCP || bStreamUDP)
    {
        // Start streaming data frames.
        if (bStreamUDP)
        {
            char* pAddr = NULL;
            unsigned short nPort = mpoRTProtocol->GetUdpServerPort();
            if (nUDPPort > 0)
            {
                nPort = nUDPPort;
            }
            if (strlen(tUDPAddress) > 0)
            {
                pAddr = tUDPAddress;
            }
            bAbort = !mpoRTProtocol->StreamFrames(eStreamRate, nRateArgument, nPort, pAddr, nComponentType, componentOptions);
        }
        else
        {
            bAbort = !mpoRTProtocol->StreamFrames(eStreamRate, nRateArgument, 0, NULL, nComponentType, componentOptions);
        }
    }

    // Main data read loop
    while (!bAbort)
    {
        if (!bStreamTCP && !bStreamUDP)
        {
            bAbort = (mpoRTProtocol->GetCurrentFrame(nComponentType, componentOptions) == false);
        }

        if (mpoRTProtocol->Receive(ePacketType, true) == CNetwork::ResponseType::success)
        {
            switch (ePacketType) 
            {
                case CRTPacket::PacketError : // sHeader.nType 0 indicates an error
                    {
                        CRTPacket* poRTPacket = mpoRTProtocol->GetRTPacket();
                        if (bStreamTCP || bStreamUDP)
                        {
                            fprintf(stderr, "Error at StreamFrames: %s\n", poRTPacket->GetErrorString());
                        }
                        else
                        {
                            fprintf(stderr, "Error at SendCurrentFrame: %s\n", poRTPacket->GetErrorString());
                        }
                        break;
                    }
                case CRTPacket::PacketData:         // Data received
                    mpoOutput->HandleDataFrame(logfile, bOnlyTimeAndFrameNumber, mpoRTProtocol, operation, bOutputModeScrolling);
                    break;
                case CRTPacket::PacketNoMoreData :   // No more data
                    break;
                default:
                    break;
            }
        }
        bAbort = mpoInput->CheckKeyPressed();
    }

    if (bStreamTCP || bStreamUDP)
    {
        mpoRTProtocol->StreamFramesStop();
    }

    if (operation != CInput::Noise2D && (operation != CInput::Statistics))
    {
        mpoOutput->PrintTimingData();
    }

    if (logfile != stdout && logfile != NULL)
    {
        fclose(logfile);
    }

} // DataTransfer

void COperations::ControlQTM()
{
    CInput::EQTMCommand eCommand;
    std::string filename;

    // Take control over QTM
    if (TakeQTMControl())
    {
        while (mpoInput->ReadQTMCommand(eCommand))
        {
            switch (eCommand)
            {
                case CInput::New :
                    if (mpoRTProtocol->NewMeasurement())
                    {
                        printf("Creating new connection.\n\n");
                    }
                    else
                    {
                        printf("New failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::Close :
                    if (mpoRTProtocol->CloseMeasurement())
                    {
                        printf("Closing connection.\n\n");
                    }
                    else
                    {
                        printf("Close failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::Start :
                    if (mpoRTProtocol->StartCapture())
                    {
                        printf("Starting measurement.\n\n");
                    }
                    else
                    {
                        printf("Start failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::StartRTFromFile :
                    if (mpoRTProtocol->StartRTOnFile())
                    {
                        printf("Starting RT from file.\n\n");
                    }
                    else
                    {
                        printf("Start RT from file failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::Stop :
                    if (mpoRTProtocol->StopCapture())
                    {
                        printf("Stopping measurement\n\n");
                    }
                    else
                    {
                        printf("Stop failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::Load :
                    filename = mpoInput->ReadString("Enter Name : ");
                    if (mpoRTProtocol->LoadCapture(filename.c_str()))
                    {
                        printf("Measurement loaded.\n\n");
                    }
                    else
                    {
                        printf("Load failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::Save :
                    {
                        filename = mpoInput->ReadString("Enter Name : ");
                        bool bOverWrite = mpoInput->ReadYesNo("Overwrite existing measurement (y/n)? ", false);
                        char tNewFileName[300];
                        if (mpoRTProtocol->SaveCapture(filename.c_str(), bOverWrite, tNewFileName, sizeof(tNewFileName)))
                        {
                            if (strlen(tNewFileName) == 0)
                            {
                                printf("Measurement saved.\n\n");
                            }
                            else
                            {
                                printf_s("Measurement saved as : %s.\n\n", tNewFileName);
                            }
                        }
                        else
                        {
                            printf("Save failed. %s\n\n", mpoRTProtocol->GetErrorString());
                        }
                    }
                    break;
                case CInput::LoadProject :
                    filename = mpoInput->ReadString("Enter Name : ");
                    if (mpoRTProtocol->LoadProject(filename.c_str()))
                    {
                        printf("Project loaded.\n\n");
                    }
                    else
                    {
                        printf("Load failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::GetCaptureC3D :
                    filename = mpoInput->ReadString("Enter Name : ");
                    if (mpoRTProtocol->GetCapture(filename.c_str(), true))
                    {
                        printf("C3D file written successfully to : %s.\n\n", filename.c_str());
                    }
                    else
                    {
                        printf("GetCaptureC3D failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::GetCaptureQTM :
                    filename = mpoInput->ReadString("Enter Name : ");
                    if (mpoRTProtocol->GetCapture(filename.c_str(), false))
                    {
                        printf("QTN file written successfully to : %s.\n\n", filename.c_str());
                    }
                    else
                    {
                        printf("GetCaptureQTM failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::Trig :
                    if (mpoRTProtocol->SendTrig())
                    {
                        printf("Trig sent\n\n");
                    }
                    else
                    {
                        printf("Trig failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::SetQTMEvent :
                    filename = mpoInput->ReadString("Enter Name : ");
                    if (mpoRTProtocol->SetQTMEvent(filename.c_str()))
                    {
                        printf("QTM event set.\n\n");
                    }
                    else
                    {
                        printf("SetQTMEvent failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::Reprocess:
                    if (mpoRTProtocol->Reprocess())
                    {
                        printf("Reprocessing started.\n\n");
                    }
                    else
                    {
                        printf("Reprocess failed. %s\n\n", mpoRTProtocol->GetErrorString());
                    }
                    break;
                case CInput::Calibrate:
                case CInput::CalibrateRefine:
                    {
                        CRTProtocol::SCalibration calibrationResult;

                        if (mpoRTProtocol->Calibrate(eCommand == CInput::CalibrateRefine, calibrationResult))
                        {
                            printf("Calibration started...\n");
                            mpoOutput->PrintCalibrationSettings(calibrationResult);
                        }
                        else
                        {
                            printf("Calibration failed. %s\n\n", mpoRTProtocol->GetErrorString());
                        }
                        break;
                    }
                default :
                    break;
            }
        }
    }
} // ControlQTM


bool COperations::TakeQTMControl()
{
    char pPassword[256];

    pPassword[0] = 0;

    do 
    {
        // Take control over QTM
        if (mpoRTProtocol->TakeControl(pPassword))
        {
            return true;
        }
        else
        {
            if (strncmp("Wrong or missing password", mpoRTProtocol->GetErrorString(), 25) == 0)
            {
                mpoInput->ReadClientControlPassword(pPassword, sizeof(pPassword));
                printf("\n");
            }
            else
            {
                printf("Failed to take control over QTM. %s\n\n", mpoRTProtocol->GetErrorString());
            }
        }
    } while (pPassword[0] != 0);

    return false;
} // TakeQTMControl


bool COperations::ReleaseQTMControl()
{
    if (mpoRTProtocol->ReleaseControl() == false)
    {
        printf("Failed to release control over QTM.\n\n");
        return false;
    }
    return true;
} // ReleaseQTMControl