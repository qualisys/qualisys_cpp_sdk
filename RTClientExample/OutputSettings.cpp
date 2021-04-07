#include "Output.h"
#include "RTProtocol.h"
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <float.h>
#include <functional>


void COutput::PrintGeneralSettings(CRTProtocol* poRTProtocol)
{
    CRTProtocol::SPoint       sPoint;
    unsigned int              nMarkerExposure, nMarkerExposureMin, nMarkerExposureMax;
    unsigned int              nMarkerThreshold, nMarkerThresholdMin, nMarkerThresholdMax;
    unsigned int              nVideoExposure, nVideoExposureMin, nVideoExposureMax;
    unsigned int              nVideoFlashTime, nVideoFlashTimeMin, nVideoFlashTimeMax;
    float                     fvRotationMatrix[3][3];
    int                       nOrientation;
    unsigned int              nMarkerResolutionWidth, nMarkerResolutionHeight, nVideoResolutionWidth, nVideoResolutionHeight;
    unsigned int              nMarkerFOVLeft, nMarkerFOVTop, nMarkerFOVRight, nMarkerFOVBottom;
    unsigned int              nVideoFOVLeft, nVideoFOVTop, nVideoFOVRight, nVideoFOVBottom;


    printf("\n================ General Settings ================\n\n");

    unsigned int nCaptureFrequency;
    float        fCaptureTime;
    bool         bStartOnExtTrig;
    bool         bStartOnTriggerNO;
    bool         bStartOnTriggerNC;
    bool         bStartOnTrigSoftware;
    CRTProtocol::EProcessingActions eProcessingActions[3];

    poRTProtocol->GetGeneralSettings(
        nCaptureFrequency, fCaptureTime,
        bStartOnExtTrig, bStartOnTriggerNO, bStartOnTriggerNC, bStartOnTrigSoftware,
        eProcessingActions[0], eProcessingActions[1], eProcessingActions[2]);

    printf("Capture frequency: %d\n", nCaptureFrequency);
    printf("Capture time: %f\n",      fCaptureTime);
    
    // Start on external trigger is redundant when we inspect StartOnTrigger(NO/NC/Software) individually.
    //printf("Start on external trigger: %s\n", bStartOnExtTrig ? "True" : "False");
    printf("Start on trigger NO (Normally Open): %s\n", bStartOnTriggerNO ? "True" : "False");
    printf("Start on trigger NC (Normally Closed): %s\n", bStartOnTriggerNC ? "True" : "False");
    printf("Start on software trigger: %s\n\n", bStartOnTrigSoftware ? "True" : "False");

    const char* processings[3] = { "----- Processing Actions-----", "----- Real-Time Processing Actions-----", "----- Reprocessing Actions-----" };

    unsigned int majorVersion;
    unsigned int minorVersion;
    poRTProtocol->GetVersion(majorVersion, minorVersion);
    auto actionsCount = (majorVersion > 1 || minorVersion > 13) ? 3 : 1;

    for (auto i = 0; i < 3; i++)
    {
        printf("%s\n",processings[i]);

        if (majorVersion > 1 || minorVersion > 13)
        {
            if (eProcessingActions[i] & CRTProtocol::ProcessingPreProcess2D)
            {
                printf("2D pre-processing\n");
            }
        }
        if (eProcessingActions[i] & CRTProtocol::ProcessingTracking2D)
        {
            printf("2D Tracking\n");
        }
        else if (eProcessingActions[i] & CRTProtocol::ProcessingTracking3D)
        {
            printf("3D Tracking\n");
        }
        if (eProcessingActions[i] & CRTProtocol::ProcessingTwinSystemMerge)
        {
            printf("Twin system merge\n");
        }
        if (eProcessingActions[i] & CRTProtocol::ProcessingSplineFill)
        {
            printf("Spline Fill\n");
        }
        if (eProcessingActions[i] & CRTProtocol::ProcessingAIM)
        {
            printf("AIM\n");
        }
        if (eProcessingActions[i] & CRTProtocol::Processing6DOFTracking)
        {
            printf("6DOF Tracking\n");
        }
        if (eProcessingActions[i] & CRTProtocol::ProcessingForceData)
        {
            printf("Force Calculation\n");
        }
        if (eProcessingActions[i] & CRTProtocol::ProcessingGazeVector)
        {
            printf("Gaze Vector Calculation\n");
        }
        if (eProcessingActions[i] & CRTProtocol::ProcessingExportTSV)
        {
            printf("Export TSV\n");
        }
        if (eProcessingActions[i] & CRTProtocol::ProcessingExportC3D)
        {
            printf("Export C3D\n");
        }
        if (eProcessingActions[i] & CRTProtocol::ProcessingExportMatlabFile)
        {
            printf("Export MATLAB File\n");
        }

        if (eProcessingActions[i] & CRTProtocol::ProcessingExportAviFile)
        {
            printf("Export AVI File\n");
        }
    }

    bool                       bEnabled;
    CRTProtocol::ESignalSource eSignalSource;
    bool                       bSignalModePeriodic;
    unsigned int               nFreqMultiplier;
    unsigned int               nFreqDivisor;
    unsigned int               nFreqTolerance;
    float                      fNominalFrequency;
    bool                       bNegativeEdge;
    unsigned int               nSignalShutterDelay;
    float                      fNonPeriodicTimeout;

    poRTProtocol->GetExtTimeBaseSettings(bEnabled, eSignalSource, bSignalModePeriodic, nFreqMultiplier,
        nFreqDivisor, nFreqTolerance, fNominalFrequency, bNegativeEdge,
        nSignalShutterDelay, fNonPeriodicTimeout);

    printf("\n----- External Time Base-----\n\n");
    printf("External Time Base:   %s\n", bEnabled ? "Enabled" : "Disabled");
    if (bEnabled)
    {
        printf("Signal Source:        ");
        switch (eSignalSource)
        {
        case CRTProtocol::SourceControlPort :
            printf("Control Port\n");
            break;
        case CRTProtocol::SourceIRReceiver :
            printf("IR Receiver\n");
            break;
        case CRTProtocol::SourceSMPTE :
            printf("SMPTE\n");
            break;
        case CRTProtocol::SourceVideoSync :
            printf("Video Sync\n");
            break;
        }
        printf("Signal Mode Periodic: %s\n", bSignalModePeriodic ? "True" : "False");
        if (bSignalModePeriodic)
        {
            printf("Frequency Multiplier: %d\n", nFreqMultiplier);
            printf("Frequency Divisor:    %d\n", nFreqDivisor);
            if (eSignalSource != CRTProtocol::SourceSMPTE)
            {
                printf("Frequency Tolerance:  %d ppm\n", nFreqTolerance);
            }
            if (fNominalFrequency == -1)
            {
                printf("Nominal Frequency:    None\n");
            }
            else
            {
                printf("Nominal Frequency:    %f Hz\n", fNominalFrequency);
            }
        }
        if (eSignalSource == CRTProtocol::SourceControlPort || eSignalSource == CRTProtocol::SourceVideoSync)
        {
            printf("Signal Edge:          %s\n", bNegativeEdge ? "Negative" : "Positive"); 
        }
        printf("Signal Shutter Delay: %d us\n", nSignalShutterDelay);
        if ((eSignalSource == CRTProtocol::SourceControlPort || eSignalSource == CRTProtocol::SourceIRReceiver ||
            eSignalSource == CRTProtocol::SourceVideoSync) && !bSignalModePeriodic)
        {
            printf("Non Periodic Timeout: %f s\n\n", fNonPeriodicTimeout);
        }
    }

    CRTProtocol::SSettingsGeneralExternalTimestamp timestampSettings;
    poRTProtocol->GetExtTimestampSettings(timestampSettings);

    printf("\n----- Timestamp-----\n\n");
    printf("Timestamp: %s\n", timestampSettings.bEnabled ? "Enabled" : "Disabled");
    if (timestampSettings.bEnabled)
    {
        printf("Type:      %s\n", timestampSettings.nType == CRTProtocol::Timestamp_SMPTE ? "SMPTE" : timestampSettings.nType == CRTProtocol::Timestamp_IRIG ? "IRIG" : "CameraTime");
        printf("Frequency: %d Hz\n", timestampSettings.nFrequency);
    }    
    
    unsigned int nMajorVersion;
    unsigned int nMinorVersion;

    poRTProtocol->GetRTPacket()->GetVersion(nMajorVersion, nMinorVersion);

    if (nMajorVersion > 1 || nMinorVersion > 20)
    {
        std::string first, second, third;
        poRTProtocol->GetEulerAngles(first, second, third);
        printf("\nEuler rotation names: First = %s  Second = %s  Third = %s\n", first.c_str(), second.c_str(), third.c_str());
    }

    unsigned int                  nID;
    CRTProtocol::ECameraModel     eModel;
    bool                          bUnderwater;
    bool                          bSupportsHwSync;
    unsigned int                  nSerial;
    CRTProtocol::ECameraMode      eMode;
    CRTProtocol::EVideoResolution videoResolution;
    CRTProtocol::EVideoAspectRatio videoAspectRatio;
    unsigned int                  nVideoFrequency;

    printf("\n----- Camera Settings-----\n\n");

    for (unsigned int iCamera = 0; iCamera < poRTProtocol->GetCameraCount(); iCamera++)
    {
        if (poRTProtocol->GetCameraSettings(iCamera, nID, eModel, bUnderwater, bSupportsHwSync, nSerial, eMode))
        {
            printf("Camera ID: %d\n", nID);
            switch (eModel)
            {
            case CRTProtocol::ModelMacReflex :
                printf("  Model: MacReflex  ");
                break;
            case CRTProtocol::ModelProReflex120 :
                printf("  Model: ProReflex 120  ");
                break;
            case CRTProtocol::ModelProReflex240 :
                printf("  Model: ProReflex 240  ");
                break;
            case CRTProtocol::ModelProReflex500 :
                printf("  Model: ProReflex 500  ");
                break;
            case CRTProtocol::ModelProReflex1000 :
                printf("  Model: ProReflex 1000  ");
                break;
            case CRTProtocol::ModelOqus100 :
                printf("  Model: Oqus 100  ");
                break;
            case CRTProtocol::ModelOqus200C :
                printf("  Model: Oqus 200 C  ");
                break;
            case CRTProtocol::ModelOqus300 :
                printf("  Model: Oqus 300  ");
                break;
            case CRTProtocol::ModelOqus300Plus :
                printf("  Model: Oqus 300 Plus  ");
                break;
            case CRTProtocol::ModelOqus400 :
                printf("  Model: Oqus 400  ");
                break;
            case CRTProtocol::ModelOqus500 :
                printf("  Model: Oqus 500  ");
                break;
            case CRTProtocol::ModelOqus500Plus :
                printf("  Model: Oqus 500 Plus  ");
                break;
            case CRTProtocol::ModelOqus700 :
                printf("  Model: Oqus 700  ");
                break;
            case CRTProtocol::ModelOqus700Plus :
                printf("  Model: Oqus 700 Plus  ");
                break;
            case CRTProtocol::ModelOqus600Plus:
                printf("  Model: Oqus 600 Plus  ");
                break;
            case CRTProtocol::ModelMiqusM1:
                printf("  Model: Miqus M1  ");
                break;
            case CRTProtocol::ModelMiqusM3:
                printf("  Model: Miqus M3  ");
                break;
            case CRTProtocol::ModelMiqusM5:
                printf("  Model: Miqus M5  ");
                break;
            case CRTProtocol::ModelMiqusSyncUnit:
                printf("  Model: Miqus Sync Unit  ");
                break;
            case CRTProtocol::ModelMiqusVideo:
                printf("  Model: Miqus Video   ");
                break;
            case CRTProtocol::ModelMiqusVideoColor:
                printf("  Model: Miqus Video Color   ");
                break;
            case CRTProtocol::ModelArqusA5:
                printf("  Model: Arqus A5   ");
                break;
            case CRTProtocol::ModelArqusA9:
                printf("  Model: Arqus A9   ");
                break;
            case CRTProtocol::ModelArqusA12:
                printf("  Model: Arqus A12   ");
                break;
            case CRTProtocol::ModelArqusA26:
                printf("  Model: Arqus A26   ");
                break;
            }
            printf("  %s\n", bUnderwater ? "Underwater" : "");
            printf("  Serial: %d\n", nSerial);
            printf("  Supports Hardware Sync: %s\n", bSupportsHwSync ? "True" : "False");

            if (eModel != CRTProtocol::ModelMiqusSyncUnit)
            {
                switch (eMode)
                {
                case CRTProtocol::ModeMarker:
                    printf("  Mode: Marker\n");
                    break;
                case CRTProtocol::ModeMarkerIntensity:
                    printf("  Mode: Marker Intensity\n");
                    break;
                case CRTProtocol::ModeVideo:
                    printf("  Mode: Video\n");
                    break;
                }
            }
        }

        if (eModel != CRTProtocol::ModelMiqusSyncUnit)
        {
            if (poRTProtocol->GetCameraMarkerSettings(iCamera, nMarkerExposure, nMarkerExposureMin, nMarkerExposureMax,
                nMarkerThreshold, nMarkerThresholdMin, nMarkerThresholdMax))
            {
                printf("  Marker Exposure:   ");
                printf("Current: %-7d", nMarkerExposure);
                printf("Min: %-7d", nMarkerExposureMin);
                printf("Max: %-7d\n", nMarkerExposureMax);

                printf("  Marker Threshold:  ");
                printf("Current: %-7d", nMarkerThreshold);
                printf("Min: %-7d", nMarkerThresholdMin);
                printf("Max: %-7d\n", nMarkerThresholdMax);
            }

            if (poRTProtocol->GetCameraVideoSettings(iCamera, videoResolution, videoAspectRatio, nVideoFrequency, nVideoExposure, nVideoExposureMin, nVideoExposureMax,
                nVideoFlashTime, nVideoFlashTimeMin, nVideoFlashTimeMax))
            {
                switch (videoResolution)
                {
                    case CRTProtocol::VideoResolution1080p :
                        printf("  Video Resolution: 1080p\n");
                        break;
                    case CRTProtocol::VideoResolution720p:
                        printf("  Video Resolution: 720p\n");
                        break;
                    case CRTProtocol::VideoResolution540p:
                        printf("  Video Resolution: 540p\n");
                        break;
                    case CRTProtocol::VideoResolution480p:
                        printf("  Video Resolution: 480p\n");
                        break;
                }
                switch (videoAspectRatio)
                {
                    case CRTProtocol::VideoAspectRatio16x9:
                        printf("  Video Aspect Ratio: 16:9\n");
                        break;
                    case CRTProtocol::VideoAspectRatio4x3:
                        printf("  Video Aspect Ratio: 4:3\n");
                        break;
                    case CRTProtocol::VideoAspectRatio1x1:
                        printf("  Video Aspect Ratio: 1:1\n");
                        break;
                }
                printf("  Video Frequency: %d Hz\n", nVideoFrequency);
                printf("  Video Exposure:    ");
                printf("Current: %-7d", nVideoExposure);
                printf("Min: %-7d", nVideoExposureMin);
                printf("Max: %-7d\n", nVideoExposureMax);
                printf("  Video Flash Time:  ");
                printf("Current: %-7d", nVideoFlashTime);
                printf("Min: %-7d", nVideoFlashTimeMin);
                printf("Max: %-7d\n", nVideoFlashTimeMax);
            }

            if (poRTProtocol->GetCameraPosition(iCamera, sPoint, fvRotationMatrix))
            {
                printf("  Position:  X: %f  Y: %f  Z: %f\n", sPoint.fX, sPoint.fY, sPoint.fZ);

                printf("  Rotation Matrix:\n");
                printf("   %9f ", fvRotationMatrix[0][0]);
                printf("   %9f ", fvRotationMatrix[1][0]);
                printf("   %9f\n", fvRotationMatrix[2][0]);
                printf("   %9f ", fvRotationMatrix[0][1]);
                printf("   %9f ", fvRotationMatrix[1][1]);
                printf("   %9f\n", fvRotationMatrix[2][1]);
                printf("   %9f ", fvRotationMatrix[0][2]);
                printf("   %9f ", fvRotationMatrix[1][2]);
                printf("   %9f\n", fvRotationMatrix[2][2]);
            }

            if (poRTProtocol->GetCameraOrientation(iCamera, nOrientation))
            {
                printf("  Rotation: %d\n", nOrientation);
            }

            if (poRTProtocol->GetCameraResolution(iCamera, nMarkerResolutionWidth, nMarkerResolutionHeight,
                nVideoResolutionWidth, nVideoResolutionHeight))
            {
                printf("  Marker Resolution:  Width: %-6d Height: %d\n", nMarkerResolutionWidth, nMarkerResolutionHeight);

                printf("  Video Resolution:   Width: %-6d Height: %d\n", nVideoResolutionWidth, nVideoResolutionHeight);
            }

            if (poRTProtocol->GetCameraFOV(iCamera, nMarkerFOVLeft, nMarkerFOVTop, nMarkerFOVRight, nMarkerFOVBottom,
                nVideoFOVLeft, nVideoFOVTop, nVideoFOVRight, nVideoFOVBottom))
            {
                printf("  Marker FOV:  Left: %-5d Top: %-5d Right: %-5d Bottom: %d\n",
                    nMarkerFOVLeft, nMarkerFOVTop, nMarkerFOVRight, nMarkerFOVBottom);
                printf("  Video FOV:   Left: %-5d Top: %-5d Right: %-5d Bottom: %d\n\n",
                    nVideoFOVLeft, nVideoFOVTop, nVideoFOVRight, nVideoFOVBottom);
            }
        }

        if (bSupportsHwSync)

        {
            CRTProtocol::ESyncOutFreqMode eSyncOutMode;
            unsigned int                  nSyncOutValue;
            float                         fSyncOutDutyCycle;
            bool                          bSyncOutNegativePolarity;

            for (unsigned int portNumber = 1; portNumber < 4; portNumber++)
            {
                if (poRTProtocol->GetCameraSyncOutSettings(iCamera, portNumber, eSyncOutMode, nSyncOutValue, fSyncOutDutyCycle,
                    bSyncOutNegativePolarity))
                {
                    if (portNumber == 1 || portNumber == 2)
                    {
                        switch (eSyncOutMode)
                        {
                        case CRTProtocol::ModeShutterOut:
                            printf("  Sync Out%d Mode:   Shutter Out\n", portNumber);
                            printf("   Signal Polarity: %s\n", bSyncOutNegativePolarity ? "Negative" : "Positive");
                            break;
                        case CRTProtocol::ModeMultiplier:
                            printf("  Sync Out%d Mode:   Multiplier = %d\n", portNumber, nSyncOutValue);
                            printf("   Duty Cycle:      %f %%\n", fSyncOutDutyCycle);
                            printf("   Signal Polarity: %s\n", bSyncOutNegativePolarity ? "Negative" : "Positive");
                            break;
                        case CRTProtocol::ModeDivisor:
                            printf("  Sync Out%d Mode:   Divisor = %d\n", portNumber, nSyncOutValue);
                            printf("   Duty Cycle:      %f %%\n", fSyncOutDutyCycle);
                            printf("   Signal Polarity: %s\n", bSyncOutNegativePolarity ? "Negative" : "Positive");
                            break;
                        case CRTProtocol::ModeIndependentFreq:
                            printf("  Sync Out%d Mode:   Actual Frequency = %d\n", portNumber, nSyncOutValue);
                            printf("   Duty Cycle:      %f %%\n", fSyncOutDutyCycle);
                            printf("   Signal Polarity: %s\n", bSyncOutNegativePolarity ? "Negative" : "Positive");
                            break;
                        case CRTProtocol::ModeMeasurementTime:
                            printf("  Sync Out%d Mode:   Measurement Time\n", portNumber);
                            printf("   Signal Polarity: %s\n", bSyncOutNegativePolarity ? "Negative" : "Positive");
                            break;
                        case CRTProtocol::ModeFixed100Hz:
                            printf("  Sync Out%d Mode: Fixed 100 Hz\n", portNumber);
                            break;
                        }
                    }
                    if (portNumber == 3)
                    {
                        printf("  Sync Out MT Signal Polarity: %s\n", bSyncOutNegativePolarity ? "Negative" : "Positive");
                    }
                }
            }
        }

        float focus = std::numeric_limits<float>::quiet_NaN();
        float aperture = std::numeric_limits<float>::quiet_NaN();
        if (poRTProtocol->GetCameraLensControlSettings(iCamera, &focus, &aperture))
        {
            printf("  Lens focus: %f\n", focus);
            printf("  Lens aperture: %f\n", aperture);
        }

        bool autoExposureEnabled = false;
        float autoExposureCompensation = std::numeric_limits<float>::quiet_NaN();
        if (poRTProtocol->GetCameraAutoExposureSettings(iCamera, &autoExposureEnabled, &autoExposureCompensation))
        {
            printf("  AutoExposure enabled: %s\n", autoExposureEnabled ? "on" : "off");
            if (autoExposureEnabled)
            {
                printf("  AutoExposure compensation: %f\n", autoExposureCompensation);
            }
        }

        bool autoWhiteBalanceEnabled = false;
        if (poRTProtocol->GetCameraAutoWhiteBalance(iCamera, &autoWhiteBalanceEnabled))
        {
            printf("  AutoWhiteBalance enabled: %s\n", autoWhiteBalanceEnabled ? "on" : "off");
        }
    }
}

void COutput::PrintCalibrationSettings(CRTProtocol* poRTProtocol)
{
    CRTProtocol::SCalibration calibrationResult;

    poRTProtocol->GetCalibrationSettings(calibrationResult);
    PrintCalibrationSettings(calibrationResult);
}

void COutput::PrintCalibrationSettings(const CRTProtocol::SCalibration &calibrationResult)
{
    printf("\n================ Calibration Result ================\n\n");

    printf("Result:                %s\n", calibrationResult.calibrated ? "Success" : "Failed");
    printf("Source:                %s\n", calibrationResult.source.c_str());
    printf("Created:               %s\n", calibrationResult.created.c_str());
    printf("QTM version:           %s\n", calibrationResult.qtm_version.c_str());
    printf("Type:                  %s\n", calibrationResult.type == CRTProtocol::regular ? "Regular" : (calibrationResult.type == CRTProtocol::refine ? "Refine" : "Fixed"));
    printf("Refit residual:        %f\n", calibrationResult.refit_residual);
    printf("Wand length:           %f\n", calibrationResult.wand_length);
    printf("Max frames:            %d\n", calibrationResult.max_frames);
    printf("Short arm end:         %f\n", calibrationResult.short_arm_end);
    printf("Long arm end:          %f\n", calibrationResult.long_arm_end);
    printf("Long arm middle:       %f\n", calibrationResult.long_arm_middle);
    printf("Result std dev:        %f\n", calibrationResult.result_std_dev);
    printf("Result min max diff:   %f\n", calibrationResult.result_min_max_diff);
    printf("Result refit residual: %f\n", calibrationResult.result_refit_residual);
    printf("Result consecutive:    %d\n", calibrationResult.result_consecutive);

    int cameraIndex = 1;

    for (const auto &camera : calibrationResult.cameras)
    {
        printf("\nCamera %d\n", cameraIndex++);
        printf("---------------------------\n");
        printf("Active:               %s\n", camera.active ? "True" : "False");
        printf("Calibrated:           %s\n", camera.calibrated ? "True" : "False");
        printf("Message:              %s\n", camera.message.c_str());
        printf("Point count:          %d\n", camera.point_count);
        printf("Average residual:     %f\n", camera.avg_residual);
        printf("Serial:               %d\n", camera.serial);
        printf("Model:                %s\n", camera.model.c_str());
        printf("View rotation:        %d\n", camera.view_rotation);
        printf("FOV marker:           %d, %d, %d, %d\n", camera.fov_marker.left, camera.fov_marker.top, camera.fov_marker.right, camera.fov_marker.bottom);
        printf("FOV marker max:       %d, %d, %d, %d\n", camera.fov_marker_max.left, camera.fov_marker_max.top, camera.fov_marker_max.right, camera.fov_marker_max.bottom);
        printf("FOV video:            %d, %d, %d, %d\n", camera.fov_video.left, camera.fov_video.top, camera.fov_video.right, camera.fov_video.bottom);
        printf("FOV video max:        %d, %d, %d, %d\n", camera.fov_video_max.left, camera.fov_video_max.top, camera.fov_video_max.right, camera.fov_video_max.bottom);
        printf("Transform:            x= %.2f, y= %.2f, z= %.2f, r11= %.2f, r12= %.2f, r13= %.2f, r21= %.2f, r22= %.2f, r23= %.2f, r31= %.2f, r32= %.2f, r33= %.2f\n",
            camera.transform.x, camera.transform.y, camera.transform.z, camera.transform.r11, camera.transform.r12, camera.transform.r13,
            camera.transform.r21, camera.transform.r22, camera.transform.r23, camera.transform.r31, camera.transform.r32, camera.transform.r33);
        printf("Intrinsic:            FocalLength= %.2f, SensorMinU= %.2f, SensorMaxU= %.2f, SensorMinV= %.2f, SensorMaxV= %.2f\n", camera.intrinsic.focal_length, camera.intrinsic.sensor_min_u, camera.intrinsic.sensor_max_u, camera.intrinsic.sensor_min_v, camera.intrinsic.sensor_max_v);
        printf("                      FocalLengthU= %.2f, FocalLengthV= %.2f, CenterPointU= %.2f, CenterPointV= %.2f, Skew= %.2f\n", camera.intrinsic.focal_length_u, camera.intrinsic.focal_length_v, camera.intrinsic.center_point_u, camera.intrinsic.center_point_v, camera.intrinsic.skew);
        printf("                      RadialDistortion1= %.2f, RadialDistortion= 2%.2f, RadialDistortion3= %.2f, TangentalDistortion1= %.2f, TangentalDistortion2= %.2f\n", camera.intrinsic.radial_distortion_1, camera.intrinsic.radial_distortion_2, camera.intrinsic.radial_distortion_3, camera.intrinsic.tangental_distortion_1, camera.intrinsic.tangental_distortion_2);
    }
    printf("\n");
}

void COutput::Print3DSettings(CRTProtocol* poRTProtocol)
{
    printf("\n================== 3D Settings ===================\n\n");

    switch (poRTProtocol->Get3DUpwardAxis())
    {
    case CRTProtocol::XPos :
        printf("Axis upwards: +X\n");
        break;
    case CRTProtocol::XNeg :
        printf("Axis upwards: -X\n");
        break;
    case CRTProtocol::YPos :
        printf("Axis upwards: +Y\n");
        break;
    case CRTProtocol::YNeg :
        printf("Axis upwards: -Y\n");
        break;
    case CRTProtocol::ZPos :
        printf("Axis upwards: +Z\n");
        break;
    case CRTProtocol::ZNeg :
        printf("Axis upwards: -Z\n");
        break;
    }

    const char* pTmpStr = poRTProtocol->Get3DCalibrated();
    if (pTmpStr[0] == 0)
    {
        printf("Calibration Time: Not Calibrated\n");
    }
    else
    {
        printf("Calibration Time: %s\n", pTmpStr);
    }

    unsigned int nCount = poRTProtocol->Get3DLabeledMarkerCount();
    printf("There are %d labeled markers\n", nCount);
    for (unsigned int iLabel = 0; iLabel < nCount; iLabel++)
    {
        printf("Marker %2d: %s", iLabel + 1, poRTProtocol->Get3DLabelName(iLabel));
        printf("   Color: %.6X", poRTProtocol->Get3DLabelColor(iLabel));
        printf("   Type:  %s\n", poRTProtocol->Get3DTrajectoryType(iLabel));
    }

    nCount = poRTProtocol->Get3DBoneCount();
    printf("\nThere are %d bones\n", nCount);
    for (unsigned int iBone = 0; iBone < nCount; iBone++)
    {
        printf("Bone %2d: From: %s -> To: %s\n", iBone + 1, poRTProtocol->Get3DBoneFromName(iBone), poRTProtocol->Get3DBoneToName(iBone));
    }

    printf("\n");
}

void COutput::Print6DOFSettings(CRTProtocol* poRTProtocol)
{
    unsigned int nMajorVersion;
    unsigned int nMinorVersion;

    poRTProtocol->GetRTPacket()->GetVersion(nMajorVersion, nMinorVersion);

    int nBodies = poRTProtocol->Get6DOFBodyCount();

    printf("\n================== 6DOF Settings =================\n\n");

    if (nMajorVersion > 1 || (nMinorVersion > 15 && nMinorVersion < 21))
    {
        std::string first, second, third;
        poRTProtocol->GetEulerAngles(first, second, third);
        printf("Euler angle names:\n");
        printf("  First = %s\n  Second = %s\n  Third = %s\n\n", first.c_str(), second.c_str(), third.c_str());
    }

    std::vector<CRTProtocol::SSettings6DOFBody> bodiesSettings;
    if (nMajorVersion > 1 || nMinorVersion > 20)
    {
        poRTProtocol->Get6DOFBodySettings(bodiesSettings);
    }

    if (nBodies > 0)
    {
        if (nBodies == 1)
        {
            printf("There is 1 6DOF body\n\n");
        }
        else
        {
            printf("There are %d 6DOF Bodies\n\n", nBodies);
        }
        for (int iBody = 0; iBody < nBodies; iBody++)
        {
            auto color = poRTProtocol->Get6DOFBodyColor(iBody);
            printf("Body #%d - %s\n", iBody + 1, poRTProtocol->Get6DOFBodyName(iBody));
            if (nMajorVersion == 1 && nMinorVersion < 21)
            {
                printf("  Color: R=%.2X G=%.2X B=%.2X\n", color & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff);
            }
            else
            {
                printf("  Color:                 R=%.2X G=%.2X B=%.2X\n", color & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff);
                printf("  Bone length tolerance: %f\n", bodiesSettings[iBody].boneLengthTolerance);
                printf("  Max residual:          %f\n", bodiesSettings[iBody].maxResidual);
                printf("  Min markers in body:   %d\n", bodiesSettings[iBody].minMarkersInBody);
                if (!bodiesSettings[iBody].mesh.name.empty())
                {
                    printf("  Mesh - %s\n", bodiesSettings[iBody].mesh.name.c_str());
                    printf("    Mesh position: %f, %f, %f\n", bodiesSettings[iBody].mesh.position.fX, bodiesSettings[iBody].mesh.position.fY, bodiesSettings[iBody].mesh.position.fZ);
                    printf("    Mesh rotation: %f, %f, %f\n", bodiesSettings[iBody].mesh.rotation.fX, bodiesSettings[iBody].mesh.rotation.fY, bodiesSettings[iBody].mesh.rotation.fZ);
                    printf("    Mesh scale:    %f\n", bodiesSettings[iBody].mesh.scale);
                    printf("    Mesh opacity:  %f\n", bodiesSettings[iBody].mesh.opacity);
                }
                if (!bodiesSettings[iBody].filterPreset.empty())
                {
                    printf("  Filter preset:         %s\n", bodiesSettings[iBody].filterPreset.c_str());
                }

                if (bodiesSettings[iBody].origin.type == CRTProtocol::GlobalOrigin)
                {
                    printf("  Translation origin:    Global\n");
                    printf("  Rotation origin:       Global\n");
                }
                else if (bodiesSettings[iBody].origin.type == CRTProtocol::RelativeOrigin)
                {
                    printf("  Translation origin:    Relative body #%d\n", bodiesSettings[iBody].origin.relativeBody - 1);
                    printf("  Rotation origin:       Relative body #%d\n", bodiesSettings[iBody].origin.relativeBody - 1);
                }
                else
                {
                    printf("  Translation origin:    Fixed\n");
                    printf("    Translation: %f, %f, %f\n", bodiesSettings[iBody].origin.position.fX, bodiesSettings[iBody].origin.position.fY, bodiesSettings[iBody].origin.position.fZ);
                    printf("  Rotation origin:       Fixed\n");
                    printf("    Rotation: %9f, %9f, %9f\n", bodiesSettings[iBody].origin.rotation[0], bodiesSettings[iBody].origin.rotation[1], bodiesSettings[iBody].origin.rotation[2]);
                    printf("              %9f, %9f, %9f\n", bodiesSettings[iBody].origin.rotation[3], bodiesSettings[iBody].origin.rotation[4], bodiesSettings[iBody].origin.rotation[5]);
                    printf("              %9f, %9f, %9f\n", bodiesSettings[iBody].origin.rotation[6], bodiesSettings[iBody].origin.rotation[7], bodiesSettings[iBody].origin.rotation[8]);
                }
            }

            for (unsigned int iPoint = 0; iPoint < poRTProtocol->Get6DOFBodyPointCount(iBody); iPoint++)
            {
                CRTProtocol::SPoint point;
                auto color = poRTProtocol->Get6DOFBodyPoint(iBody, iPoint, point);
                printf("  Point - %s\n", (nMajorVersion > 1 || nMinorVersion > 20) ? bodiesSettings[iBody].points[iPoint].name.c_str() : "");
                printf("    Position: %f, %f, %f\n", point.fX, point.fY, point.fZ);
                if (nMajorVersion > 1 || nMinorVersion > 20)
                {
                    printf("    Virtual = %s\n", bodiesSettings[iBody].points[iPoint].virtual_ ? "True" : "False");
                    printf("    Physical id = %d\n", bodiesSettings[iBody].points[iPoint].physicalId);
                }
            }
            printf("\n");
        }
    }
}

void COutput::PrintGazeVectorSettings(CRTProtocol* poRTProtocol)
{
    int nGazeVectorCount = poRTProtocol->GetGazeVectorCount();

    if (nGazeVectorCount > 0)
    {
        printf("\n============== Gaze Vector Settings ==============\n\n");

        for (int iVector = 0; iVector < nGazeVectorCount; iVector++)
        {
            printf("Gaze vector #%d\n", iVector + 1);
            printf("  Name:  %s\n", poRTProtocol->GetGazeVectorName(iVector));
            printf("  Frequency: %.1f\n", poRTProtocol->GetGazeVectorFrequency(iVector));
            printf("  Hardware sync: %s\n", poRTProtocol->GetGazeVectorHardwareSyncUsed(iVector) ? "True" : "False");
            printf("  Filter: %s\n", poRTProtocol->GetGazeVectorFilterUsed(iVector) ? "True" : "False");
            printf("\n");
        }
    }
}

void COutput::PrintEyeTrackerSettings(CRTProtocol* poRTProtocol)
{
    int eyeTrackerCount = poRTProtocol->GetEyeTrackerCount();

    if (eyeTrackerCount > 0)
    {
        printf("\n============== Eye Tracker Settings ==============\n\n");

        for (int eyeTracker = 0; eyeTracker < eyeTrackerCount; eyeTracker++)
        {
            printf("Eye tracker #%d\n", eyeTracker + 1);
            printf("  Name:  %s\n", poRTProtocol->GetEyeTrackerName(eyeTracker));
            printf("  Frequency: %.1f\n", poRTProtocol->GetEyeTrackerFrequency(eyeTracker));
            printf("  Hardware sync: %s\n", poRTProtocol->GetGazeVectorHardwareSyncUsed(eyeTracker) ? "True" : "False");
            printf("\n");
        }
    }
}

void COutput::PrintAnalogSettings(CRTProtocol* poRTProtocol)
{
    unsigned int nMajorVersion;
    unsigned int nMinorVersion;

    poRTProtocol->GetRTPacket()->GetVersion(nMajorVersion, nMinorVersion);

    unsigned int nCount = poRTProtocol->GetAnalogDeviceCount();

    if (nCount > 0)
    {
        unsigned int nDeviceID, nChannels, nFrequency;
        float        fMinRange, fMaxRange;
        char*        pName;
        char*        pUnit;

        printf("\n================ Analog Settings =================\n\n");

        if (nMajorVersion == 1 && nMinorVersion == 0)
        {
            if (nCount > 0)
            {
                poRTProtocol->GetAnalogDevice(0, nDeviceID, nChannels, pName, nFrequency, pUnit, fMinRange, fMaxRange);

                printf("Analog parameters\n");
                printf("  Channels:  %d\n",   nChannels);
                printf("  Frequency: %d\n",   nFrequency);
                printf("  Unit:      %s\n",   pUnit);
                printf("  Range Min: %f\n",   fMinRange);
                printf("  Range Max: %f\n\n", fMaxRange);
            }
        }
        else
        {
            printf("Analog parameters\n");
            for (unsigned int iDevice = 0; iDevice < nCount; iDevice++)
            {
                poRTProtocol->GetAnalogDevice(iDevice, nDeviceID, nChannels, pName,
                    nFrequency, pUnit, fMinRange, fMaxRange);

                printf("  Analog Device %d, %s\n", nDeviceID, pName);
                printf("    Channels:  %d\n", nChannels);
                printf("    Frequency: %d\n",   nFrequency);
                if (nMajorVersion == 1 && nMinorVersion < 11)
                {
                    printf("    Unit:      %s\n",   pUnit);
                }
                printf("    Range Min: %f\n",   fMinRange);
                printf("    Range Max: %f\n", fMaxRange);
                for (unsigned int iChannel = 0; iChannel < nChannels; iChannel++)
                {
                    printf("    Channel %d\n", iChannel + 1);
                    printf("      Label: %s\n", poRTProtocol->GetAnalogLabel(iDevice, iChannel));
                    if (nMajorVersion > 1 || nMinorVersion > 10)
                    {
                        printf("      Unit:  %s\n", poRTProtocol->GetAnalogUnit(iDevice, iChannel));
                    }
                }
            }
            printf("\n");
        }
    }
}

void COutput::PrintForceSettings(CRTProtocol* poRTProtocol)
{
    unsigned int nCount = poRTProtocol->GetForcePlateCount();

    if (nCount > 0)
    {
        unsigned int        nPlateID, nAnalogDeviceID, nFrequency;
        float               fLength, fWidth;
        char*               pType;
        char*               pName;
        CRTProtocol::SPoint sCorners[4];
        CRTProtocol::SPoint sOrigin;
        unsigned int        nChannelNo;
        float               fConversionFactor;

        printf("\n================ Force Settings ==================\n\n");

        for (unsigned int iPlate = 0; iPlate < nCount; iPlate++)
        {
            poRTProtocol->GetForcePlate(iPlate, nPlateID, nAnalogDeviceID, nFrequency, pType, pName, fLength, fWidth);

            printf("Force plate ID %d\n", nPlateID);
            printf("  Plate type: %s\n", pType);
            if (nAnalogDeviceID != 0)
            {
                printf("  Analog device id: %d\n", nAnalogDeviceID);
            }
            printf("  Frequency: %d\n", nFrequency);
            printf("  Length: %-8.2f Width: %-8.2f\n", fLength, fWidth);

            poRTProtocol->GetForcePlateLocation(iPlate, sCorners);

            printf("  Corner 1: ");
            printf("X = %-8.2f ",   sCorners[0].fX);
            printf("Y = %-8.2f ",   sCorners[0].fY);
            printf("Z = %-8.2f \n", sCorners[0].fZ);
            printf("  Corner 2: ");
            printf("X = %-8.2f ",   sCorners[1].fX);
            printf("Y = %-8.2f ",   sCorners[1].fY);
            printf("Z = %-8.2f \n", sCorners[1].fZ);
            printf("  Corner 3: ");
            printf("X = %-8.2f ",   sCorners[2].fX);
            printf("Y = %-8.2f ",   sCorners[2].fY);
            printf("Z = %-8.2f \n", sCorners[2].fZ);
            printf("  Corner 4: ");
            printf("X = %-8.2f ",   sCorners[3].fX);
            printf("Y = %-8.2f ",   sCorners[3].fY);
            printf("Z = %-8.2f \n", sCorners[3].fZ);

            poRTProtocol->GetForcePlateOrigin(iPlate, sOrigin);

            printf("  Origin:   ");
            printf("X = %-8.2f ",   sOrigin.fX);
            printf("Y = %-8.2f ",   sOrigin.fY);
            printf("Z = %-8.2f \n", sOrigin.fZ);

            if (poRTProtocol->GetForcePlateChannelCount(iPlate) > 0)
            {
                printf("  Channels\n");
            }

            for (unsigned int iChannel = 0; iChannel < poRTProtocol->GetForcePlateChannelCount(iPlate); iChannel++)
            {
                poRTProtocol->GetForcePlateChannel(iPlate, iChannel, nChannelNo, fConversionFactor);
                printf("    Channel number: %2d", nChannelNo);
                printf("    Conversion factor: %10f\n", fConversionFactor);
            }

            float fvCalMatrix[12][12];
            unsigned int nRows = 6;
            unsigned int nColumns = 6;
            if (poRTProtocol->GetForcePlateCalibrationMatrix(iPlate, fvCalMatrix, &nRows, &nColumns))
            {
                printf("  Calibration Matrix:\n");
                for (unsigned int iRow = 0; iRow < nRows; iRow++)
                {
                    printf("    ");
                    for (unsigned int iCol = 0; iCol < nColumns; iCol++)
                    {
                        printf("%11f ", fvCalMatrix[iRow][iCol]);
                    }
                    printf("\n");
                }
            }
        }
        printf("\n");
    }
}

void COutput::PrintImageSettings(CRTProtocol* poRTProtocol)
{
    unsigned int            nCameraID, nWidth, nHeight;
    float                   fCropLeft, fCropTop, fCropRight, fCropBottom;
    bool                    bEnabled;
    CRTPacket::EImageFormat eFormat;

    if (poRTProtocol->GetImageCameraCount() > 0)
    {
        printf("\n================ Image Settings ==================\n\n");

        for (unsigned int iCamera = 0; iCamera < poRTProtocol->GetImageCameraCount(); iCamera++)
        {
            poRTProtocol->GetImageCamera(iCamera, nCameraID, bEnabled, eFormat, nWidth, nHeight, fCropLeft, fCropTop,
                fCropRight, fCropBottom);
            printf("Camera ID %d\n", nCameraID);
            switch (eFormat)
            {
            case CRTPacket::FormatRawGrayscale :
                printf("  Image Format = RAWGrayscale\n");
                break;
            case CRTPacket::FormatRawBGR :
                printf("  Image Format = RAWBGR\n");
                break;
            case CRTPacket::FormatJPG :
                printf("  Image Format = JPG\n");
                break;
            case CRTPacket::FormatPNG :
                printf("  Image Format = PNG\n");
                break;
            }
            printf("  Width = %d  Height = %d\n", nWidth, nHeight);
            printf("  Crop:  Left = %d %%  Top = %d %%  Right = %d %%  Bottom = %d %%\n\n",
                (int)(fCropLeft * 100), (int)(fCropTop * 100), (int)(fCropRight * 100), (int)(fCropBottom * 100));
        }
    }
}

void COutput::PrintSkeletonSettings(CRTProtocol* poRTProtocol, bool skeletonGlobalReferenceFrame)
{
    unsigned int majorVersion;
    unsigned int minorVersion;
    poRTProtocol->GetVersion(majorVersion, minorVersion);

    if (poRTProtocol->GetSkeletonCount() > 0)
    {
        printf("\n================ Skeleton Settings ==================\n");

        for (unsigned int iSkeleton = 0; iSkeleton < poRTProtocol->GetSkeletonCount(); iSkeleton++)
        {
            CRTProtocol::SSettingsSkeletonHierarchical skeleton;
            uint32_t level = 0;

            if ((majorVersion > 1 || minorVersion > 20) && poRTProtocol->GetSkeleton(iSkeleton, skeleton))
            {
                printf("\nSkeleton Name: %s  Scale: %f", skeleton.name.c_str(), skeleton.scale);

                std::function<void(const CRTProtocol::SSettingsSkeletonSegmentHierarchical, uint32_t&)> recurseSegments = [&](const CRTProtocol::SSettingsSkeletonSegmentHierarchical& segment, uint32_t& level)
                {
                    level++;
                    std::string indent = std::string(level * 3, ' ');

                    printf("\n");
                    printf("%sSegment name: %s   ID: %d   Solver: %s\n", indent.c_str(), segment.name.c_str(), segment.id, segment.solver.c_str());
                    printf("%s   Position: %.2f, %.2f, %.2f\n", indent.c_str(), segment.position.x, segment.position.y, segment.position.z);
                    printf("%s   Rotation: %.2f, %.2f, %.2f %.2f\n", indent.c_str(), segment.rotation.x, segment.rotation.y, segment.rotation.z, segment.rotation.w);
                    
                    if (segment.defaultPosition.IsValid())
                    {
                        printf("%s   Default Position: %.2f, %.2f, %.2f\n", indent.c_str(), segment.defaultPosition.x, segment.defaultPosition.y, segment.defaultPosition.z);
                    }
                    if (segment.defaultRotation.IsValid())
                    {
                        printf("%s   Default Rotation: %.2f, %.2f, %.2f %.2f\n", indent.c_str(), segment.defaultRotation.x, segment.defaultRotation.y, segment.defaultRotation.z, segment.defaultRotation.w);
                    }

                    if (!segment.degreesOfFreedom.empty())
                    {
                        printf("%s   Degrees of freedom:\n", indent.c_str());
                    }

                    for (auto dof : segment.degreesOfFreedom)
                    {
                        if (!std::isnan(dof.lowerBound) || !std::isnan(dof.upperBound) || !dof.couplings.empty() || !std::isnan(dof.goalValue) || !std::isnan(dof.goalWeight))
                        {
                            printf("%s      %s: ", indent.c_str(), CRTProtocol::SkeletonDofToString(dof.type));
                            if (!std::isnan(dof.lowerBound) || !std::isnan(dof.upperBound))
                            {
                                printf("Constraint: %.4f, %.4f ", dof.lowerBound, dof.upperBound);
                            }
                            for (auto& coupling : dof.couplings)
                            {
                                printf("Coupling: %s, %s, %.4f ", coupling.segment.c_str(), CRTProtocol::SkeletonDofToString(coupling.degreeOfFreedom), coupling.coefficient);
                            }
                            if (!std::isnan(dof.goalValue) || !std::isnan(dof.goalWeight))
                            {
                                printf("Goal: %.4f, %.4f", dof.goalValue, dof.goalWeight);
                            }
                            printf("\n");
                        }
                    }

                    if (segment.endpoint.IsValid())
                    {
                        printf("%s   Endpoint: %.2f, %.2f, %.2f\n", indent.c_str(), segment.endpoint.x, segment.endpoint.y, segment.endpoint.z);
                    }

                    for (const auto& marker : segment.markers)
                    {
                        printf("%s   Marker name: %s\n", indent.c_str(), marker.name.c_str());
                        printf("%s      Position: %.2f, %.2f, %.2f\n", indent.c_str(), marker.position.x, marker.position.y, marker.position.z);
                        printf("%s      Weight: %.2f\n", indent.c_str(), marker.weight);
                    }
                    for (const auto& rigidBody : segment.bodies)
                    {
                        printf("%s   Rigid body name: %s\n", indent.c_str(), rigidBody.name.c_str());
                        printf("%s      Position: %.2f, %.2f, %.2f\n", indent.c_str(), rigidBody.position.x, rigidBody.position.y, rigidBody.position.z);
                        printf("%s      Rotation: %.2f, %.2f, %.2f, %.2f\n", indent.c_str(), rigidBody.rotation.x, rigidBody.rotation.y, rigidBody.rotation.z, rigidBody.rotation.w);
                        printf("%s      Weight: %.2f\n", indent.c_str(), rigidBody.weight);
                    }
                    for (const auto& childSegment : segment.segments)
                    {
                        recurseSegments(childSegment, level);
                    }
                    level--;
                };
                recurseSegments(skeleton.rootSegment, level);
            }


            CRTProtocol::SSettingsSkeletonSegment segment;
            unsigned int segmentCount = poRTProtocol->GetSkeletonSegmentCount(iSkeleton);
            const char* name = poRTProtocol->GetSkeletonName(iSkeleton);

            printf("\nSkeleton: %s  Segment count: %d\n", name, segmentCount);
            for (unsigned int segmentIndex = 0; segmentIndex < segmentCount; segmentIndex++)
            {
                poRTProtocol->GetSkeletonSegment(iSkeleton, segmentIndex, &segment);
                printf("  Segment id: %2d  name: %-19s  Parent id: %-4d  Parent index: %-4d  ", segment.id, segment.name.c_str(), segment.parentId, segment.parentIndex);
                printf("%s  Pos: %8.2f, %8.2f, %8.2f  Rot: %4.2f, %4.2f, %4.2f, %4.2f\n", skeletonGlobalReferenceFrame ? "Global" : "Local",
                    segment.positionX, segment.positionY, segment.positionZ,
                    segment.rotationX, segment.rotationY, segment.rotationZ, segment.rotationW);
            }
        }
    }
}
