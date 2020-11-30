#include "Output.h"
#include "RTProtocol.h"
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <float.h>
#include <locale.h>
#include <algorithm>

#define WRITE_ANALOG_HEADERS_TO_FILE


COutput::COutput()
{
    mbWriteLogFileHeader  = true;
    mfDist                = NULL;
    mbOutputModeScrolling = false;
    QueryPerformanceFrequency(&mPerformanceCounterFrequency);
    mOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    mFrameNumberResets = 0;
    mTimestampResets = 0;
}

void COutput::HandleDataFrame(FILE* logfile, bool bLogMinimum, CRTProtocol* poRTProtocol, CInput::EOperation operation, bool bOutputModeScrolling)
{
    CRTPacket*   poRTPacket;
    HANDLE       hConsole = GetStdHandle (STD_OUTPUT_HANDLE);

    mbOutputModeScrolling = bOutputModeScrolling;

    mnReceivedFrames++;

    if (poRTProtocol)
    {
        poRTPacket = poRTProtocol->GetRTPacket();
        if (poRTPacket->GetComponentCount() == 0 || poRTPacket->GetType() != CRTPacket::PacketData)
        {
            return;
        }
    }
    else
    {
        return;
    }

    unsigned int       nFrameNumber = poRTPacket->GetFrameNumber();
    unsigned long long nTimeStamp   = poRTPacket->GetTimeStamp();

    if (nTimeStamp == 0)
    {
        mTimestampResets++;
    }
    if (nFrameNumber == 1)
    {
        mFrameNumberResets++;
    }

    if (nFrameNumber == 1 && mnLastFrameNumber != 0)
    {
        // Start from the beginning in case we are running rt from file
        ResetCounters();
        mnReceivedFrames = 1;
    }

    // This is the first frame received
    if (mnLastFrameNumber == 0xffffffff)
    {
        QueryPerformanceCounter(&mnStartTime);
    }

    // Update packet receive time.
    LARGE_INTEGER nCurrentTime;
    QueryPerformanceCounter(&nCurrentTime);
    mfCurrentRecvTime = (1.0 * (nCurrentTime.QuadPart - mnStartTime.QuadPart)) / (1.0 * mPerformanceCounterFrequency.QuadPart);

    if (mnReceivedFrames > 1)
    {
        mnFrameNumberDiff = nFrameNumber - mnLastFrameNumber;

        if (mnFrameNumberDiff <= 0)
        {
            // Frame repeated (should never happen).
            QueryPerformanceCounter(&mnStartTime);
            ResetCounters();
            mnReceivedFrames = 1;
        }
        else
        {
            // New frame received.
            mfCameraFreq = (mnFrameNumberDiff * 1000000.0) / (nTimeStamp - mnLastTimeStamp);
            mnMaxFrameNumberDiff = max((unsigned int)mnFrameNumberDiff, mnMaxFrameNumberDiff);
            mfRecvTimeDiff    = mfCurrentRecvTime - mfLastRecvTime;
            mRecvTimeDiffs.push_back(mfRecvTimeDiff);
            mMaxRecvTimeDiff = max(mfRecvTimeDiff, mMaxRecvTimeDiff);
            mMinRecvTimeDiff = min(mfRecvTimeDiff, mMinRecvTimeDiff);

            int missingFrames = mnFrameNumberDiff - 1;

            if (missingFrames > 0)
            {
                mnMissingFrames += missingFrames;
                printf("\nMissing %d frame%s (total %d). Frame number: %d", missingFrames, (missingFrames == 1) ? "" : "s", mnMissingFrames, mnLastFrameNumber + 1);
                if (missingFrames > 1)
                {
                    printf(" to %d                \n\n", nFrameNumber - 1);
                }
                else
                {
                    printf("                             \n\n");
                }
            }
        }
    }
    mnLastTimeStamp = nTimeStamp;
    mnLastFrameNumber = nFrameNumber;
    mfLastRecvTime = mfCurrentRecvTime;

    mPrintPos.X = 0;
    mPrintPos.Y = 1;

    if (operation != CInput::Noise2D && operation != CInput::Statistics)
    {
        PrintHeader(logfile, poRTPacket, bLogMinimum);
    }

    if (bLogMinimum)
    {
        return;
    }

    if (operation == CInput::Statistics)
    {
        PrintStatistics(logfile, poRTPacket);
    }
    else if (operation == CInput::Noise2D)
    {
        Print2DNoise(poRTPacket);
    }
    else
    {
        PrintTimecode(logfile, poRTPacket);
        PrintData2D(logfile, poRTPacket);
        PrintData2DLin(logfile, poRTPacket);
        PrintData3D(logfile, poRTPacket, poRTProtocol);
        PrintData3DRes(logfile, poRTPacket, poRTProtocol);
        PrintData3DNoLabels(logfile, poRTPacket);
        PrintData3DNoLabelsRes(logfile, poRTPacket);
        PrintData6D(logfile, poRTPacket, poRTProtocol);
        PrintData6DRes(logfile, poRTPacket, poRTProtocol);
        PrintData6DEuler(logfile, poRTPacket, poRTProtocol);
        PrintData6DEulerRes(logfile, poRTPacket, poRTProtocol);
        PrintDataGazeVector(logfile, poRTPacket, poRTProtocol);
        PrintDataEyeTracker(logfile, poRTPacket, poRTProtocol);
        PrintAnalog(logfile, poRTPacket);
        PrintAnalogSingle(logfile, poRTPacket);
        PrintForce(logfile, poRTPacket);
        PrintForceSingle(logfile, poRTPacket);
        PrintImage(logfile, poRTPacket);
        PrintSkeleton(logfile, poRTPacket, poRTProtocol);
    }
} // PrintData

void COutput::PrintHeader(FILE* logfile, CRTPacket* poRTPacket, bool bLogMinimum)
{
    bool         bShowDropRate = false;
    unsigned int nMajorVersion;
    unsigned int nMinorVersion;

    poRTPacket->GetVersion(nMajorVersion, nMinorVersion);

    if (logfile == stdout && !mbOutputModeScrolling)
    {
        SetConsoleCursorPosition(mOutputHandle, mPrintPos);
        fprintf(logfile, "================================================================\n");
        fprintf(logfile, "Timestamp %10I64d \tFrame %d          \n\n",
            poRTPacket->GetTimeStamp(), mnLastFrameNumber);

        if (nMajorVersion == 1 && nMinorVersion > 2 &&
            poRTPacket->GetDropRate() != 0xffff &&
            poRTPacket->GetOutOfSyncRate() != 0xffff && bShowDropRate)
        {
            fprintf(logfile, "2D Drop Rate = %.1f%%, 2D Out Of Sync Rate = %.1f%%          \n",
                poRTPacket->GetDropRate() / 10.0,
                poRTPacket->GetOutOfSyncRate() / 10.0);
        }
        fprintf(logfile, "================================================================\n\n");
        mPrintPos.Y += 6;
    }
    else
    {
        if (bLogMinimum)
        {
            if (mbWriteLogFileHeader)
            {
                // Write log file header
                fprintf(logfile, "Timestamp\tFrameNo\tFrameNoDiff\tRecvTime\n");
                mbWriteLogFileHeader = false;
            }
            fprintf(logfile, "%10I64d\t%d\t%d\t%f\n",
                poRTPacket->GetTimeStamp(), mnLastFrameNumber, mnFrameNumberDiff, mfCurrentRecvTime);
            return;
        }
        else
        {
#ifdef WRITE_ANALOG_HEADERS_TO_FILE
            fprintf(logfile, "================================================================\n");
            fprintf(logfile, "Timestamp: %-10I64u \tFrameNo: %d\n", poRTPacket->GetTimeStamp(), mnLastFrameNumber);

            if (nMajorVersion == 1 && nMinorVersion > 2 &&
                poRTPacket->GetDropRate() != 0xffff &&
                poRTPacket->GetOutOfSyncRate() != 0xffff && bShowDropRate)
            {
                fprintf(logfile, "2D Drop Rate = %.1f%%, 2D Out Of Sync Rate = %.1f%%\n",
                    poRTPacket->GetDropRate() / 10.0,
                    poRTPacket->GetOutOfSyncRate() / 10.0);
            }
            fprintf(logfile, "================================================================\n");
#endif
        }
    }
}

void COutput::PrintTimecode(FILE* logfile, CRTPacket* poRTPacket)
{
    if (poRTPacket->GetComponentSize(CRTPacket::ComponentTimecode))
    {
        if (poRTPacket->IsTimeCodeAvailable())
        {
            if (logfile == stdout && !mbOutputModeScrolling)
            {
                SetConsoleCursorPosition(mOutputHandle, mPrintPos);
            }

            fprintf(logfile, "--------------- Timecode --------------\n");
            mPrintPos.Y++;

            int year, day, hours, minutes, seconds, tenth, frame;
            unsigned __int64 cameraTime;

            if (poRTPacket->GetTimecodeSMPTE(hours, minutes, seconds, frame))
            {
                fprintf(logfile, "SMPTE: Hours %02d Minutes %02d Seconds %02d Frame %02d\n", hours, minutes, seconds, frame);
            }
            if (poRTPacket->GetTimecodeIRIG(year, day, hours, minutes, seconds, tenth))
            {
                fprintf(logfile, "IRIG: Year %02d Day %03d Hour %02d Min %02d Sec %02d Sec/10 %d\n", year, day, hours, minutes, seconds, tenth);
            }
            if (poRTPacket->GetTimecodeCameraTime(cameraTime))
            {
                fprintf(logfile, "Camera Time: %I64u\n", cameraTime);
            }
            mPrintPos.Y++;

            fprintf(logfile, "\n");
            mPrintPos.Y++;
        }
    }
} // PrintTimecode

void COutput::PrintStatistics(FILE* logfile, CRTPacket* poRTPacket)
{
    if (logfile == stdout)
    {
        SetConsoleCursorPosition(mOutputHandle, mPrintPos);
    }
    printf("----------- Data Frame Statistics --------------\n\n");
    printf("Average receive frequency = %.0f Hz          \n", mnReceivedFrames / mfCurrentRecvTime);
    printf("Camera frequency          = %.0f Hz          \n\n", mfCameraFreq);
    printf("Received frames       = %d          \n", mnReceivedFrames);
    printf("Missing frames        = %d          \n", mnMissingFrames);
    printf("Last frame number     = %d          \n", mnLastFrameNumber);
    printf("Frame number diff     = %d          \n", mnFrameNumberDiff);
    printf("Max frame number diff = %d          \n\n", mnFrameNumberDiff);

    printf("Min time between frames = %d ms          \n", (int)(mMinRecvTimeDiff * 1000 + 0.5));
    printf("Max time between frames = %d ms          \n", (int)(mMaxRecvTimeDiff * 1000 + 0.5));
    double medianRecvTimeDiff = 0.0;
    if (mRecvTimeDiffs.size() > 0)
    {
        std::sort(mRecvTimeDiffs.begin(), mRecvTimeDiffs.end());
        medianRecvTimeDiff = mRecvTimeDiffs[mRecvTimeDiffs.size() / 2];
    }
    printf("Median time between frames = %d ms          \n\n", (int)(medianRecvTimeDiff * 1000 + 0.5));

    printf("Timestamp Reset Count    = %d            \n", mTimestampResets);
    printf("Frame number Reset Count = %d            \n", mFrameNumberResets);

    mPrintPos.Y += 15;
}

void COutput::PrintData2D(FILE* logfile, CRTPacket* poRTPacket)
{
    unsigned int   nX, nY;
    unsigned short nXD, nYD;

    if (poRTPacket->GetComponentSize(CRTPacket::Component2d))
    {
        unsigned int nCamCount;
        unsigned int nMarkCount;
        unsigned int nMajorVersion;
        unsigned int nMinorVersion;

        poRTPacket->GetVersion(nMajorVersion, nMinorVersion);

        if (logfile == stdout && !mbOutputModeScrolling)
        {
            SetConsoleCursorPosition(mOutputHandle, mPrintPos);
        }

        fprintf(logfile, "------------------- 2D -------------------\n");
        mPrintPos.Y++;

        nCamCount = poRTPacket->Get2DCameraCount();
        if (nCamCount > mcnMaxCameras)
        {
            nCamCount = mcnMaxCameras;
        }

        for (unsigned int iCamera = 0; iCamera < nCamCount; iCamera++)
        {
            unsigned char nStatus = 0;

            if (iCamera > 0)
            {
                fprintf(logfile, "\n");
                mPrintPos.Y++;
            }

            if (nMajorVersion > 1 || nMinorVersion >= 8)
            {
                fprintf(logfile, "Camera %d  Status Flags: 0x%x\n", iCamera + 1, poRTPacket->Get2DStatusFlags(iCamera));
            }
            else
            {
                fprintf(logfile, "Camera %d\n", iCamera + 1);
            }
            mPrintPos.Y++;

            nMarkCount = poRTPacket->Get2DMarkerCount(iCamera);
            if (nMarkCount > mcnMaxMarkers)
            {
                nMarkCount = mcnMaxMarkers;
            }

            unsigned int iMarker;
            for (iMarker = 0; iMarker < nMarkCount; iMarker++)
            {
                poRTPacket->Get2DMarker(iCamera, iMarker, nX, nY, nXD, nYD);

                fprintf(logfile, "  X=%6d Y=%6d  Xsize=%4d Ysize=%4d\n", nX, nY, nXD, nYD);
                mPrintPos.Y++;
            }
            for (; iMarker < mcnMaxMarkers; iMarker++)
            {
                fprintf(logfile, "                                               \n");
                mPrintPos.Y++;
            }
        }

        if (logfile != stdout || mbOutputModeScrolling)
        {
            fprintf(logfile, "\n");
        }
        else
        {
            mnMaxPlotYPos = max(mPrintPos.Y, mnMaxPlotYPos);

            // Delete lines with markers that are not visible any more.
            while (mPrintPos.Y <= mnMaxPlotYPos)
            {
                SetConsoleCursorPosition(mOutputHandle, mPrintPos);
                printf("                                                                                                                ");
                mPrintPos.Y++;
            }
        }

        mn2DFrames++;
    }
} // PrintData2D

void COutput::PrintData2DLin(FILE* logfile, CRTPacket* poRTPacket)
{
    unsigned int   nX, nY;
    unsigned short nXD, nYD;

    if (poRTPacket->GetComponentSize(CRTPacket::Component2dLin))
    {
        unsigned int nMajorVersion;
        unsigned int nMinorVersion;

        poRTPacket->GetVersion(nMajorVersion, nMinorVersion);

        if (logfile == stdout && !mbOutputModeScrolling)
        {
            SetConsoleCursorPosition(mOutputHandle, mPrintPos);
        }

        fprintf(logfile, "-------------- 2D Linearized -------------\n");
        mPrintPos.Y++;

        for (unsigned int iCamera = 0; iCamera < poRTPacket->Get2DLinCameraCount(); iCamera++)
        {
            unsigned char nStatus = 0;
            if (nMajorVersion > 1 || nMinorVersion >= 8)
            {
                fprintf(logfile, "Camera %d  Status Flags: 0x%x\n", iCamera + 1, poRTPacket->Get2DLinStatusFlags(iCamera));
            }
            else
            {
                fprintf(logfile, "Camera %d\n", iCamera + 1);
            }
            mPrintPos.Y++;

            for (unsigned int iMarker = 0; iMarker < poRTPacket->Get2DLinMarkerCount(iCamera); iMarker++)
            {
                poRTPacket->Get2DLinMarker(iCamera, iMarker, nX, nY, nXD, nYD);

                fprintf(logfile, "  X=%6d Y=%6d  Xsize=%4d Ysize=%4d\n", nX, nY, nXD, nYD);
                mPrintPos.Y++;
            }
        }
        fprintf(logfile, "\n");
        mPrintPos.Y++;
    }
} // PrintData2DLin

void COutput::PrintData3D(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol)
{
    float fX, fY, fZ;

    if (poRTPacket->GetComponentSize(CRTPacket::Component3d))
    {
        if (poRTPacket->Get3DMarkerCount() > 0)
        {
            fprintf(logfile, "------------------- 3D -------------------\n"); 
            for (unsigned int i = 0; i < poRTPacket->Get3DMarkerCount(); i++)
            {
                const char* tmpStr = poRTProtocol->Get3DLabelName(i);
                fprintf(logfile, "%-16s : ", tmpStr ? tmpStr : "Unknown");

                if (poRTPacket->Get3DMarker(i, fX, fY, fZ))
                {
                    fprintf(logfile, "%9.3f %9.3f %9.3f", fX, fY, fZ);
                }
                fprintf(logfile, "\n");
            }
            fprintf(logfile, "\n");
        }
    }
}

void COutput::PrintData3DRes(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol)
{
    float fX, fY, fZ, fResidual;

    if (poRTPacket->GetComponentSize(CRTPacket::Component3dRes))
    {
        if (poRTPacket->Get3DResidualMarkerCount() > 0)
        {
            fprintf(logfile, "--------------- 3D Residual --------------\n"); 
            for (unsigned int i = 0; i < poRTPacket->Get3DResidualMarkerCount(); i++)

            {
                const char* tmpStr = poRTProtocol->Get3DLabelName(i);
                fprintf(logfile, "%-16s : ", tmpStr ? tmpStr : "Unknown");

                if (poRTPacket->Get3DResidualMarker(i, fX, fY, fZ, fResidual))
                {
                    fprintf(logfile, "%9.3f %9.3f %9.3f %9.3f", fX, fY, fZ, fResidual);
                }
                fprintf(logfile, "\n");
            }
            fprintf(logfile, "\n");
        }
    }
} // PrintData3DRes

void COutput::PrintData3DNoLabels(FILE* logfile, CRTPacket* poRTPacket)
{
    float        fX, fY, fZ;
    unsigned int nId;

    if (poRTPacket->GetComponentSize(CRTPacket::Component3dNoLabels))
    {
        if (poRTPacket->Get3DNoLabelsMarkerCount() > 0)
        {
            fprintf(logfile, "-------------- 3D No Labels --------------\n"); 
            for (unsigned int i = 0; i < poRTPacket->Get3DNoLabelsMarkerCount(); i++)
            {
                poRTPacket->Get3DNoLabelsMarker(i, fX, fY, fZ, nId);
                fprintf(logfile, "Marker%4d : %9.3f %9.3f %9.3f\n", nId, fX, fY, fZ);
            }
            fprintf(logfile, "\n");
        }
    }
} // PrintData3DNoLabels

void COutput::PrintData3DNoLabelsRes(FILE* logfile, CRTPacket* poRTPacket)
{
    float        fX, fY, fZ, fResidual;
    unsigned int nId;

    if (poRTPacket->GetComponentSize(CRTPacket::Component3dNoLabelsRes))
    {
        if (poRTPacket->Get3DNoLabelsResidualMarkerCount() > 0)
        {                     
            fprintf(logfile, "---------- 3D No Labels Residual ---------\n"); 
            for (unsigned int i = 0; i < poRTPacket->Get3DNoLabelsResidualMarkerCount(); i++)
            {
                poRTPacket->Get3DNoLabelsResidualMarker(i, fX, fY, fZ, nId, fResidual);
                fprintf(logfile, "Marker%4d : %9.3f %9.3f %9.3f%9.3f\n", nId, fX, fY, fZ, fResidual);
            }
            fprintf(logfile, "\n");
        }
    }
} // PrintData3DNoLabelsRes

void COutput::PrintData6D(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol)
{
    float fX, fY, fZ;
    float afRotMatrix[9];

    if (poRTPacket->GetComponentSize(CRTPacket::Component6d))
    {
        unsigned int nCount = poRTPacket->Get6DOFBodyCount();

        if (nCount > 0)
        {
            fprintf(logfile, "------------------ 6 DOF -----------------\n"); 
            for (unsigned int i = 0; i < nCount; i++)
            {
                char* label = (char*)poRTProtocol->Get6DOFBodyName(i);
                char  emptyString[] = "";
                if (label == NULL)
                {
                    label = emptyString;
                }
                poRTPacket->Get6DOFBody(i, fX, fY, fZ, afRotMatrix);

                fprintf(logfile,  "%15s : %9.3f %9.3f %9.3f   %9.3f %9.3f %9.3f %9.3f %9.3f %9.3f %9.3f %9.3f %9.3f\n",
                    label, fX, fY, fZ, afRotMatrix[0], afRotMatrix[1], afRotMatrix[2], afRotMatrix[3],
                    afRotMatrix[4], afRotMatrix[5], afRotMatrix[6], afRotMatrix[7], afRotMatrix[8]);
            }
            fprintf(logfile, "\n");
        }
    }
} // PrintData6D

void COutput::PrintData6DRes(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol)
{
    float fX, fY, fZ, fResidual;
    float afRotMatrix[9];

    if (poRTPacket->GetComponentSize(CRTPacket::Component6dRes))
    {
        unsigned int nCount = poRTPacket->Get6DOFResidualBodyCount();

        if (nCount > 0)
        {
            fprintf(logfile, "------------- 6 DOF Residual -------------\n"); 
            for (unsigned int i = 0; i < nCount; i++)
            {
                char* label = (char*)poRTProtocol->Get6DOFBodyName(i);
                char  emptyString[] = "";
                if (label == NULL)
                {
                    label = emptyString;
                }
                poRTPacket->Get6DOFResidualBody(i, fX, fY, fZ, afRotMatrix, fResidual);

                fprintf(logfile,  "%15s : %9.3f %9.3f %9.3f   %9.3f %9.3f %9.3f %9.3f %9.3f %9.3f %9.3f %9.3f %9.3f   %9.3f\n",
                    label, fX, fY, fZ, afRotMatrix[0], afRotMatrix[1], afRotMatrix[2], afRotMatrix[3],
                    afRotMatrix[4], afRotMatrix[5], afRotMatrix[6], afRotMatrix[7], afRotMatrix[8], fResidual);
            }
            fprintf(logfile, "\n");
        }
    }
} // PrintData6DRes

void COutput::PrintData6DEuler(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol)
{
    float fX, fY, fZ, fAng1, fAng2, fAng3;

    if (poRTPacket->GetComponentSize(CRTPacket::Component6dEuler))
    {
        unsigned int nCount = poRTPacket->Get6DOFEulerBodyCount();

        if (nCount > 0)
        {
            fprintf(logfile, "--------------- 6 DOF Euler --------------\n"); 
            for (unsigned int i = 0; i < nCount; i++)
            {
                char* label = (char*)poRTProtocol->Get6DOFBodyName(i);
                char  emptyString[] = "";
                if (label == NULL)
                {
                    label = emptyString;
                }
                poRTPacket->Get6DOFEulerBody(i, fX, fY, fZ, fAng1, fAng2, fAng3);

                fprintf(logfile,  "%15s : %9.3f %9.3f %9.3f   %9.3f %9.3f %9.3f\n", label, fX, fY, fZ, fAng1, fAng2, fAng3);
            }
            fprintf(logfile, "\n");
        }
    }
} // PrintData6DEuler

void COutput::PrintData6DEulerRes(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol)
{
    float fX, fY, fZ, fAng1, fAng2, fAng3, fResidual;

    if (poRTPacket->GetComponentSize(CRTPacket::Component6dEulerRes))
    {
        unsigned int nCount = poRTPacket->Get6DOFEulerResidualBodyCount();

        if (nCount > 0)
        {
            fprintf(logfile, "---------- 6 DOF Euler Residual ----------\n"); 
            for (unsigned int i = 0; i < nCount; i++)
            {
                char* label = (char*)poRTProtocol->Get6DOFBodyName(i);
                char  emptyString[] = "";
                if (label == NULL)
                {
                    label = emptyString;
                }
                poRTPacket->Get6DOFEulerResidualBody(i, fX, fY, fZ, fAng1, fAng2, fAng3, fResidual);

                fprintf(logfile,  "%15s : %9.3f %9.3f %9.3f   %9.3f %9.3f %9.3f   %9.3f\n",
                    label, fX, fY, fZ, fAng1, fAng2, fAng3, fResidual);
            }
            fprintf(logfile, "\n");
        }
    }
} // PrintData6DEulerRes

void COutput::PrintDataGazeVector(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol)
{
    if (poRTPacket->GetComponentSize(CRTPacket::ComponentGazeVector))
    {
        unsigned int nCount = poRTPacket->GetGazeVectorCount();

        if (nCount > 0)
        {
            fprintf(logfile, "--------------- Gaze Vector --------------\n"); 
            for (unsigned int i = 0; i < nCount; i++)
            {
                char* tLabel = (char*)poRTProtocol->GetGazeVectorName(i);
                char  emptyString[] = "";
                if (tLabel == NULL)
                {
                    tLabel = emptyString;
                }
                CRTPacket::SGazeVector sGazeVector;
                unsigned int nSampleCount = poRTPacket->GetGazeVectorSampleCount(i);
                if (nSampleCount > 0)
                {
                    fprintf(logfile, "%18s Sample number %d\n", tLabel, poRTPacket->GetGazeVectorSampleNumber(i));
                    for (unsigned int nSampleIndex = 0; nSampleIndex < nSampleCount; nSampleIndex++)
                    {
                        if (poRTPacket->GetGazeVector(i, nSampleIndex, sGazeVector))
                        {
                            fprintf(logfile, "  Position %-8.2f %-8.2f %-8.2f   Direction %-5.2f %-5.2f %-5.2f\n",
                                    sGazeVector.fPosX, sGazeVector.fPosY, sGazeVector.fPosZ,
                                    sGazeVector.fX, sGazeVector.fY, sGazeVector.fZ);
                        }
                    }
                }
            }
            fprintf(logfile, "\n");
        }
    }
} // PrintDataGazeVector

void COutput::PrintDataEyeTracker(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol)
{
    if (poRTPacket->GetComponentSize(CRTPacket::ComponentEyeTracker))
    {
        unsigned int nCount = poRTPacket->GetEyeTrackerCount();

        if (nCount > 0)
        {
            fprintf(logfile, "--------------- Eye Tracker --------------\n");
            for (unsigned int i = 0; i < nCount; i++)
            {
                char* tLabel = (char*)poRTProtocol->GetEyeTrackerName(i);
                char  emptyString[] = "";
                if (tLabel == NULL)
                {
                    tLabel = emptyString;
                }
                CRTPacket::SEyeTracker sEyeTracker;
                unsigned int nSampleCount = poRTPacket->GetEyeTrackerSampleCount(i);
                if (nSampleCount > 0)
                {
                    fprintf(logfile, "%s - Sample number %d\n", tLabel, poRTPacket->GetEyeTrackerSampleNumber(i));
                    for (unsigned int nSampleIndex  = 0; nSampleIndex < nSampleCount; nSampleIndex++)
                    {
                        if (poRTPacket->GetEyeTrackerData(i, nSampleIndex, sEyeTracker))
                        {
                            fprintf(logfile, "Left Pupil Diameter %-8.2f  Right Pupil Diameter %-8.2f\n", sEyeTracker.leftPupilDiameter, sEyeTracker.rightPupilDiameter);
                        }
                    }
                }
            }
            fprintf(logfile, "\n");
        }
    }
} // PrintDataEyeTracker

void COutput::PrintAnalog(FILE* logfile, CRTPacket* poRTPacket)
{
    if (poRTPacket->GetComponentSize(CRTPacket::ComponentAnalog))
    {
        unsigned int nCount = poRTPacket->GetAnalogDeviceCount();

        if (nCount > 0)
        {
            float        fAnalogValue;
            unsigned int nDeviceId;
            unsigned int nChannelCount;
            unsigned int nSampleCount;
            unsigned int nSampleNumber;
            unsigned int nMajorVersion;
            unsigned int nMinorVersion;

            poRTPacket->GetVersion(nMajorVersion, nMinorVersion);

            if (logfile == stdout)
            {
                // Don't log to file.

                printf("----------------- Analog -----------------\n"); 
                                  
                if (nMajorVersion == 1 && nMinorVersion == 0)
                {
                    for (unsigned int i = 0; i < poRTPacket->GetAnalogChannelCount(0); i++)
                    {
                        poRTPacket->GetAnalogData(0, i, 0, fAnalogValue);
                        if (!_isnan(fAnalogValue))
                        {
                            printf("Channel%3d: %6.3f\n", i, fAnalogValue);
                        }
                    }
                }
                else
                {
                    for (unsigned int iDevice = 0; iDevice < nCount; iDevice++)
                    {
                        nDeviceId     = poRTPacket->GetAnalogDeviceId(iDevice);
                        nChannelCount = poRTPacket->GetAnalogChannelCount(iDevice);
                        nSampleCount  = poRTPacket->GetAnalogSampleCount(iDevice);
                        nSampleNumber = poRTPacket->GetAnalogSampleNumber(iDevice);

                        if (nSampleCount > 0)
                        {
                            if (nCount < 1 || nCount > 32 || nChannelCount < 1 || nChannelCount > 512)
                            {
                                printf("Analog Device Error\n");
                                break;
                            }

                            printf("Analog Device %2d\nAnalog Frame %d\n", iDevice + 1, nSampleNumber);

                            for (unsigned int iChannel = 0; iChannel < nChannelCount; iChannel++)
                            {
                                printf("Ch%3d:\n", iChannel + 1);
                                for (unsigned int iSample = 0; iSample < nSampleCount; iSample++)
                                {
                                    poRTPacket->GetAnalogData(iDevice, iChannel, iSample, fAnalogValue);
                                    {
                                        printf("%6.3f\t", fAnalogValue);
                                        if ((iSample + 1) % 10 == 0)
                                        {
                                            printf("\n");
                                        }
                                    }
                                }
                                printf("\n");
                            }
                        }
                    }
                }
            }
            else 
            {
                // Log to file

#ifdef WRITE_ANALOG_HEADERS_TO_FILE
                fprintf(logfile, "----------------- Analog -----------------\n"); 
#endif
                if (nMajorVersion == 1 && nMinorVersion == 0)
                {
                    nChannelCount = poRTPacket->GetAnalogChannelCount(0);
                    if (mbWriteLogFileHeader)
                    {
                        // Write log file header
                        fprintf(logfile, "FrameNo\tTimestamp\t");
                        for (unsigned int a = 1; a <= poRTPacket->GetAnalogChannelCount(0); a++)
                        {
                            fprintf(logfile, "Ch_%d\t", a);
                        }
                        fprintf(logfile, "\n");
                        mbWriteLogFileHeader = false;
                    }
                    bool bNoData  = true;

                    for (unsigned int i = 0; i < nChannelCount; i++)
                    {
                        poRTPacket->GetAnalogData(0, i, 0, fAnalogValue);
                        bNoData = bNoData && _isnan(fAnalogValue);
                    }

                    if (!bNoData)
                    {
                        int nFrameNumber = poRTPacket->GetFrameNumber();
                        setlocale(LC_NUMERIC, "Swedish");
                        fprintf(logfile, "%d\t%I64d\t", nFrameNumber, poRTPacket->GetTimeStamp());
                        for (unsigned int i = 0; i < nChannelCount; i++)
                        {
                            if (_isnan(fAnalogValue))
                            {
                                fprintf(logfile, "\t");
                            }
                            else
                            {
                                fprintf(logfile, "%6.3f\t", fAnalogValue);
                            }
                        }
                        fprintf(logfile, "\n");
                    }
                }
                else
                {
                    for (unsigned int iDevice = 0; iDevice < nCount; iDevice++)
                    {
                        nDeviceId     = poRTPacket->GetAnalogDeviceId(iDevice);
                        nChannelCount = poRTPacket->GetAnalogChannelCount(iDevice);
                        nSampleCount  = poRTPacket->GetAnalogSampleCount(iDevice);
                        nSampleNumber = poRTPacket->GetAnalogSampleNumber(iDevice);

                        if (nSampleCount > 0)
                        {
                            if (nCount < 1 || nCount > 32 ||
                                nChannelCount < 1 || nChannelCount > 512)
                            {
                                printf("Analog Device Error\n");
                                break;
                            }
                            for (unsigned int iSample = 0; iSample < nSampleCount; iSample++)
                            {
                                bool bNoData = true;
                                for (unsigned int iChannel = 0; iChannel < nChannelCount; iChannel++)
                                {
                                    poRTPacket->GetAnalogData(iDevice, iChannel, iSample, fAnalogValue);
                                    bNoData = bNoData && _isnan(fAnalogValue);
                                }
                                if (!bNoData)
                                {
                                    setlocale(LC_NUMERIC, "Swedish");
#ifdef WRITE_ANALOG_HEADERS_TO_FILE
                                    fprintf(logfile, "Frame %8d ", nSampleNumber + iSample);
#else
                                    fprintf(logfile, "%8d ", nSampleNumber + iSample);
#endif
                                    for (unsigned int iChannel = 0; iChannel < nChannelCount; iChannel++)
                                    {
                                        if (poRTPacket->GetAnalogData(iDevice, iChannel, iSample, fAnalogValue))
                                        {
                                            fprintf(logfile, "%6.3f ", fAnalogValue);
                                        }
                                        else
                                        {
                                            fprintf(logfile, " ");
                                        }
                                    }
                                    fprintf(logfile, "\n");
                                }
                            }
                        }
                    }
                }
            }
            fprintf(logfile, "\n");
        }
    }
} // PrintAnalog

void COutput::PrintAnalogSingle(FILE* logfile, CRTPacket* poRTPacket)
{
    if (poRTPacket->GetComponentSize(CRTPacket::ComponentAnalogSingle))
    {
        unsigned int nCount = poRTPacket->GetAnalogSingleDeviceCount();

        if (nCount > 0)
        {
            float        fAnalogValue;
            unsigned int nDeviceId;
            unsigned int nChannelCount;

            if (logfile == stdout)
            {
                // Don't log to file.

                fprintf(logfile, "-------------- Analog Single -------------\n"); 

                for (unsigned int iDevice = 0; iDevice < nCount; iDevice++)
                {
                    nDeviceId     = poRTPacket->GetAnalogSingleDeviceId(iDevice);
                    nChannelCount = poRTPacket->GetAnalogSingleChannelCount(iDevice);

                    if (nCount < 1 || nCount > 32 || nChannelCount < 1 || nChannelCount > 512)
                    {
                        printf("Analog Device Error\n");
                        break;
                    }
                    if (poRTPacket->GetAnalogSingleData(iDevice, 0, fAnalogValue))
                    {
                        printf("Analog Device %2d\n", iDevice + 1);

                        for (unsigned int iChannel = 0; iChannel < nChannelCount; iChannel++)
                        {
                            if (poRTPacket->GetAnalogSingleData(iDevice, iChannel, fAnalogValue))
                            {
                                fprintf(logfile, "Ch%3d: ", iChannel + 1);
                                fprintf(logfile, "%6.3f ", fAnalogValue);
                                fprintf(logfile, "\n");
                            }
                        }
                    }
                }
            }
            else 
            {
                // Log to file

#ifdef WRITE_ANALOG_HEADERS_TO_FILE
                fprintf(logfile, "-------------- Analog Single -------------\n"); 
#endif
                for (unsigned int iDevice = 0; iDevice < nCount; iDevice++)
                {
                    nDeviceId     = poRTPacket->GetAnalogSingleDeviceId(iDevice);
                    nChannelCount = poRTPacket->GetAnalogSingleChannelCount(iDevice);

                    if (nCount < 1 || nCount > 32 || nChannelCount < 1 || nChannelCount > 512)
                    {
                        printf("Analog Device Error\n");
                        break;
                    }
                    bool bNoData = true;
                    for (unsigned int iChannel = 0; iChannel < nChannelCount; iChannel++)
                    {
                        bNoData = !poRTPacket->GetAnalogSingleData(iDevice, iChannel, fAnalogValue);
                    }
                    if (!bNoData)
                    {
                        setlocale(LC_NUMERIC, "Swedish"); // To get ',' instead of '.'
                        for (unsigned int iChannel = 0; iChannel < nChannelCount; iChannel++)
                        {
                            if (poRTPacket->GetAnalogSingleData(iDevice, iChannel, fAnalogValue))
                            {
                                fprintf(logfile, "%6.3f ", fAnalogValue);
                            }
                            else
                            {
                                fprintf(logfile, " ");
                            }
                        }
                        fprintf(logfile, "\n");
                    }
                }
            }
            fprintf(logfile, "\n");
        }
    }
} // PrintAnalogSingle

void COutput::PrintForce(FILE* logfile, CRTPacket* poRTPacket)
{
    if (poRTPacket->GetComponentSize(CRTPacket::ComponentForce))
    {
        unsigned int nCount = poRTPacket->GetForcePlateCount();

        if (nCount > 0)
        {
            bool bForcePrinted = false;
            CRTPacket::SForce sForce;
            unsigned int        nForceNumber;

            for (unsigned int iPlate = 0; iPlate < nCount; iPlate++)
            {
                unsigned int nForceCount = poRTPacket->GetForceCount(iPlate);

                if (nForceCount > 0)
                {
                    if (!bForcePrinted)
                    {
                        fprintf(logfile, "----------------- Force ------------------\n"); 
                       bForcePrinted = true;
                    }
                    fprintf(logfile, "Force Plate ID: %d\n",  poRTPacket->GetForcePlateId(iPlate));
                    nForceNumber = poRTPacket->GetForceNumber(iPlate);

                    for (unsigned int iForce = 0; iForce < nForceCount; iForce++)
                    {
                        if (poRTPacket->GetForceData(iPlate, iForce, sForce))
                        {
                            fprintf(logfile, "Frame %8d Force:\tX=%f,\tY=%f,\tZ=%f\n", nForceNumber++, 
                                sForce.fForceX, sForce.fForceY, sForce.fForceZ);
                            fprintf(logfile, "               Moment:\tX=%f,\tY=%f,\tZ=%f\n",
                                sForce.fMomentX, sForce.fMomentY, sForce.fMomentZ);
                            fprintf(logfile, "               Point:\tX=%f,\tY=%f,\tZ=%f\n", 
                                sForce.fApplicationPointX, sForce.fApplicationPointY, sForce.fApplicationPointZ);
                        }
                    }
                }
            }
            fprintf(logfile, "\n");
        }
    }
} // PrintForce

void COutput::PrintForceSingle(FILE* logfile, CRTPacket* poRTPacket)
{
    if (poRTPacket->GetComponentSize(CRTPacket::ComponentForceSingle))
    {
        unsigned int nCount = poRTPacket->GetForceSinglePlateCount();

        if (nCount > 0)
        {
            bool bForcePrinted = false;
            CRTPacket::SForce sForce;

            for (unsigned int iPlate = 0; iPlate < nCount; iPlate++)
            {
                if (poRTPacket->GetForceSingleData(iPlate, sForce))
                {
                    if (!bForcePrinted)
                    {
                        fprintf(logfile, "-------------- Force Single --------------\n"); 
                        bForcePrinted = true;
                    }
                    fprintf(logfile, "Force Plate ID: %d\n",  poRTPacket->GetForceSinglePlateId(iPlate));

                    fprintf(logfile, "Force:\tX=%f,\tY=%f,\tZ=%f\n",
                        sForce.fForceX, sForce.fForceY, sForce.fForceZ);
                    fprintf(logfile, "Moment:\tX=%f,\tY=%f,\tZ=%f\n",
                        sForce.fMomentX, sForce.fMomentY, sForce.fMomentZ);
                    fprintf(logfile, "Point:\tX=%f,\tY=%f,\tZ=%f\n", 
                        sForce.fApplicationPointX, sForce.fApplicationPointY, sForce.fApplicationPointZ);
                }
            }
            fprintf(logfile, "\n");
        }
    }
} // PrintForceSingle

void COutput::PrintImage(FILE* logfile, CRTPacket* poRTPacket)
{
    if (poRTPacket->GetComponentSize(CRTPacket::ComponentImage))
    {
        unsigned int nCameraId, nWidth, nHeight;
        CRTPacket::EImageFormat eFormat;
        float fCropLeft, fCropTop, fCropRight, fCropBottom;
        unsigned int nImageSize = 0;
        bool         bSuccess   = true;

        unsigned int nCount = poRTPacket->GetImageCameraCount();

        for (unsigned int i = 0; i < nCount; i++)
        {
            nCameraId  = poRTPacket->GetImageCameraId(i);
            bSuccess  &= poRTPacket->GetImageFormat(i, eFormat);
            bSuccess  &= poRTPacket->GetImageSize(i, nWidth, nHeight);
            bSuccess  &= poRTPacket->GetImageCrop(i, fCropLeft, fCropTop, fCropRight, fCropBottom);
            nImageSize = poRTPacket->GetImageSize(i);

            if (nCameraId > 0 && bSuccess && nImageSize > 0)
            {
                fprintf(logfile, "----------------- Image ------------------\n"); 
                fprintf(logfile, "Format: %s\n", eFormat == CRTPacket::FormatRawGrayscale ? "Raw Grayscale" : 
                    eFormat == CRTPacket::FormatRawBGR ? "Raw BGR" :
                    eFormat == CRTPacket::FormatJPG ? "JPG" : "PNG");
                fprintf(logfile, "Image Size: %d bytes\n", nImageSize);
                fprintf(logfile, "Width: %d\n",       nWidth);
                fprintf(logfile, "Height: %d\n",      nHeight);
                fprintf(logfile, "Left Crop: %f\n",   fCropLeft);
                fprintf(logfile, "Top Crop: %f\n",    fCropTop);
                fprintf(logfile, "Right Crop: %f\n",  fCropRight);
                fprintf(logfile, "Bottom Crop: %f\n", fCropBottom);
            }
        }
    }
} // PrintImage

void COutput::PrintSkeleton(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol)
{
    if (poRTPacket->GetComponentSize(CRTPacket::ComponentSkeleton))
    {
        unsigned int skeletonCount = poRTPacket->GetSkeletonCount();

        if (skeletonCount > 0)
        {
            fprintf(logfile, "---------------- Skeleton ---------------\n");
            for (unsigned int skeletonIndex = 0; skeletonIndex < skeletonCount; skeletonIndex++)
            {
                unsigned int segmentCount = poRTPacket->GetSkeletonSegmentCount(skeletonIndex);
                
                if (segmentCount > 0)
                {
                    const char* name = poRTProtocol->GetSkeletonName(skeletonIndex);
                    if (name == NULL)
                    {
                        fprintf(logfile, "Skeleton name missing.\n\n");
                        return;
                    }

                    fprintf(logfile, "Skeleton name: %s   Segment count: %d\n", name, segmentCount);

                    CRTPacket::SSkeletonSegment segments[50];
                    if (!poRTPacket->GetSkeletonSegments(skeletonIndex, segments, sizeof(segments)))
                    {
                        fprintf(logfile, "Bad skeleton segments.\n\n");
                        return;
                    }

                    for (unsigned int segmentIndex = 0; segmentIndex < segmentCount; segmentIndex++)
                    {
                        CRTProtocol::SSettingsSkeletonSegment segmentInfo;
                        poRTProtocol->GetSkeletonSegment(skeletonIndex, segmentIndex, &segmentInfo);
                        fprintf(logfile, "%-20s  id: %d  Pos: %9.3f, %9.3f, %9.3f   Rot: %7.4f, %7.4f, %7.4f, %7.4f\n", segmentInfo.name.c_str(), segments[segmentIndex].id,
                            segments[segmentIndex].positionX, segments[segmentIndex].positionY, segments[segmentIndex].positionZ,
                            segments[segmentIndex].rotationX, segments[segmentIndex].rotationY, segments[segmentIndex].rotationZ, segments[segmentIndex].rotationW);
                    }

                    fprintf(logfile, "\n");
                }
            }
            fprintf(logfile, "\n");
        }
    }
} // PrintDataSkeleton

void COutput::Print2DNoise(CRTPacket* poRTPacket)
{
    unsigned int   nX, nY;
    unsigned short nXD, nYD;

    if (poRTPacket->GetComponentSize(CRTPacket::Component2d))
    {
        float        fDistMax;
        unsigned int nCamCount;
        unsigned int nMarkCount;
        unsigned int nMajorVersion;
        unsigned int nMinorVersion;

        poRTPacket->GetVersion(nMajorVersion, nMinorVersion);
        
        SetConsoleCursorPosition(mOutputHandle, mPrintPos);
        printf("------------------- 2D Noise -------------------\n\n");

        nCamCount = poRTPacket->Get2DCameraCount();
        if (nCamCount > mcnMaxCameras)
        {
            nCamCount = mcnMaxCameras;
        }

        mPrintPos.Y += 2;

        for (unsigned int iCamera = 0; iCamera < nCamCount; iCamera++)
        {
            unsigned char nStatus = 0;

            nMarkCount = poRTPacket->Get2DMarkerCount(iCamera);
            if (nMarkCount > mcnMaxMarkers)
            {
                nMarkCount = mcnMaxMarkers;
            }

            for (unsigned int iMarker = 0; iMarker < nMarkCount; iMarker++)
            {
                poRTPacket->Get2DMarker(iCamera, iMarker, nX, nY, nXD, nYD);

                // Noise calculation

                if ((iCamera < mcnMaxCameras - 1) && (iMarker < mcnMaxMarkers))
                {
                    if (mn2DFrames > 0)
                    {
                        // Calculate distance in sub pixels
                        int dX = abs((int)(nX - masPrev2DMarkers[iCamera][iMarker].nX));
                        int dY = abs((int)(nY - masPrev2DMarkers[iCamera][iMarker].nY));
                        int nIndex = (iCamera * mcnMaxMarkers * mcnUseSamples + iMarker * mcnUseSamples) +
                            (mn2DFrames - 1) % mcnUseSamples;
                        mfDist[nIndex] = sqrtf((float)(dX * dX + dY * dY));

                        // Find max distance for this marker
                        nIndex = (iCamera * mcnMaxMarkers * mcnUseSamples + iMarker * mcnUseSamples);
                        fDistMax = 0;
                        for (int i = nIndex; i < nIndex + mcnUseSamples; i++)
                        {
                            if (fDistMax < mfDist[i])
                            {
                                fDistMax = mfDist[i];
                            }
                        }

                        // Output position and noise
                        sprintf_s(msDist, sizeof(msDist), "<");
                        int i;
                        for (i = 1; i <= floor(fDistMax) && i <= mcnMaxNoise; i++)
                        {
                            strcat_s(msDist, sizeof(msDist), "-");
                        }
                        for (int j = i; j <= mcnMaxNoise; j++)
                        {
                            strcat_s(msDist, sizeof(msDist), " ");
                        }
                        strcat_s(msDist, sizeof(msDist), ">          ");

                        SetConsoleCursorPosition(mOutputHandle, mPrintPos);
                        printf("Camera=%2d Marker= %3d X=%6d Y=%6d  Xsize= %4d Ysize= %4d  Dist=%8.1f %s", iCamera, iMarker, nX, nY, nXD, nYD, fDistMax, msDist);
                        mnMaxPlotYPos = max(mPrintPos.Y, mnMaxPlotYPos);
                        mPrintPos.Y++;
                    }

                    masPrev2DMarkers[iCamera][iMarker].nX = nX;
                    masPrev2DMarkers[iCamera][iMarker].nY = nY;
                }
            }
        }

        // Delete lines with markers that are not visible any more.
        while (mPrintPos.Y <= mnMaxPlotYPos)
        {
            SetConsoleCursorPosition(mOutputHandle, mPrintPos);
            printf("                                                                                                                ");
            mPrintPos.Y++;
        }
        mn2DFrames++;
    }
} // Print2DNoise

void COutput::PrintTimingData()
{
    printf("\n\nReceived %d data frames in %f seconds\n\n", mnReceivedFrames, mfCurrentRecvTime);
    printf("Average receive frequency = %.1f\n", mnReceivedFrames / mfCurrentRecvTime);
    printf("Camera frequency = %.1f Hz\n", mfCameraFreq);
    printf("Missed frames = %d\n", mnMissingFrames);
    printf("Max frame receive time diff = %f ms\n", mMaxRecvTimeDiff);
    printf("Min frame receive time diff = %f ms\n\n", mMinRecvTimeDiff);
} // PrintTimingData

void COutput::ResetCounters()
{
    // Reset statistic counters
    mfRecvTimeDiff         = 0.0;
    mMaxRecvTimeDiff       = DBL_MIN;
    mMinRecvTimeDiff       = DBL_MAX;
    mRecvTimeDiffs.clear();
    mfLastRecvTime         = 0.0;
    mfLastScreenUpdateTime = 0.0;
    mnLastFrameNumber      = 0xffffffff;
    mnMaxFrameNumberDiff   = 0;
    mnMissingFrames        = 0;
    mnReceivedFrames       = 0;
    mnFrameNumberDiff      = 0;
    mnMaxFrameNumberDiff   = 0;
    mnLastTimeStamp        = 0;
} // ResetCounters

void COutput::PrintEvent(CRTPacket::EEvent eEvent)
{
    char pStr[256];
    if (CRTProtocol::GetEventString(eEvent, pStr))
    {
        printf("%s\n", pStr);
    }
}

void COutput::Reset2DNoiseCalc()
{
    if (mfDist != NULL)
    {
        free(mfDist);
    }
    mfDist = (float*)malloc(sizeof(float)*((mcnMaxCameras * mcnMaxMarkers * mcnUseSamples) + (mcnMaxMarkers * mcnUseSamples) + (2 * mcnUseSamples)));
    mfDist[mcnMaxCameras * mcnMaxMarkers * mcnUseSamples] = 0.0;
    mn2DFrames = 0;
    mnMaxPlotYPos = 0;
}