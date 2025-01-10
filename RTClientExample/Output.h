#ifndef OUTPUT_H
#define OUTPUT_H

#include <vector>
#include "RTProtocol.h"
#include "Input.h"

class CDataPacket;

class COutput
{
public:
         COutput();
    void HandleDataFrame(FILE* logfile, bool bLogMinimum, CRTProtocol* poRTProtocol, CInput::EOperation operation, bool bOutputModeScrolling);
    void PrintTimingData();
    void ResetCounters();
    void PrintEvent(CRTPacket::EEvent eEvent);
    void PrintGeneralSettings(CRTProtocol* poRTProtocol);
    void PrintCalibrationSettings(CRTProtocol* poRTProtocol);
    void PrintCalibrationSettings(const CRTProtocol::SCalibration &calibrationResult);
    void Print3DSettings(CRTProtocol* poRTProtocol);
    void Print6DOFSettings(CRTProtocol* poRTProtocol);
    void PrintGazeVectorSettings(CRTProtocol* poRTProtocol);
    void PrintEyeTrackerSettings(CRTProtocol* poRTProtocol);
    void PrintAnalogSettings(CRTProtocol* poRTProtocol);
    void PrintForceSettings(CRTProtocol* poRTProtocol);
    void PrintImageSettings(CRTProtocol* poRTProtocol);
    void PrintSkeletonSettings(CRTProtocol* poRTProtocol, bool skeletonGlobalReferenceFrame);
    void Reset2DNoiseCalc();

private:
    struct Marker
    {
        unsigned int nX;
        unsigned int nY;
    };

    void PrintHeader(FILE* logfile, CRTPacket* poRTPacket, bool bLogMinimum);
    void PrintTimecode(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol);
    void PrintStatistics(FILE* logfile, CRTPacket* poRTPacket);
    void PrintData2D(FILE* logfile, CRTPacket* poRTPacket);
    void PrintData2DLin(FILE* logfile, CRTPacket* poRTPacket);
    void PrintData3D(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol);
    void PrintData3DRes(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol);
    void PrintData3DNoLabels(FILE* logfile, CRTPacket* poRTPacket);
    void PrintData3DNoLabelsRes(FILE* logfile, CRTPacket* poRTPacket);
    void PrintData6D(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol);
    void PrintData6DRes(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol);
    void PrintData6DEuler(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol);
    void PrintData6DEulerRes(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol);
    void PrintDataGazeVector(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol);
    void PrintDataEyeTracker(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol);
    void PrintAnalog(FILE* logfile, CRTPacket* poRTPacket);
    void PrintAnalogSingle(FILE* logfile, CRTPacket* poRTPacket);
    void PrintForce(FILE* logfile, CRTPacket* poRTPacket);
    void PrintForceSingle(FILE* logfile, CRTPacket* poRTPacket);
    void PrintImage(FILE* logfile, CRTPacket* poRTPacket);
    void PrintSkeleton(FILE* logfile, CRTPacket* poRTPacket, CRTProtocol* poRTProtocol);
    void Print2DNoise(CRTPacket* poRTPacket);

    template< typename... Args>
    void WriteOutput(FILE* stream, char const * const fmt, Args... args)
    {
        if(!mbOutputModeScrolling)
        {
            fprintf(stream, "                                                                                                                  \r");
        }
        fprintf(stream, fmt, args...);
    }

    static const int mcnMaxCameras = 30;  // How many cameras can be measured noise on
    static const int mcnMaxMarkers = 30;  // How many markers can be measured noise on
    static const int mcnUseSamples = 100; // How many samples to calculate max noise upon 
    static const int mcnMaxNoise   = 30;  // Max noise in subpixels to display

    char          msDist[100];
    float*        mfDist;
    int           mn2DFrames;

    bool          mbWriteLogFileHeader;

    LARGE_INTEGER mnStartTime;
    LARGE_INTEGER mPerformanceCounterFrequency;
    double        mfLastScreenUpdateTime;
    Marker        masPrev2DMarkers[mcnMaxCameras][mcnMaxMarkers];

    HANDLE mOutputHandle;
    int  mnMaxPlotYPos;
    bool mbOutputModeScrolling;

    std::vector<double> mRecvTimeDiffs;
    double mfCurrentRecvTime;
    double mfLastRecvTime;
    double mfRecvTimeDiff;
    double mMaxRecvTimeDiff;
    double mMinRecvTimeDiff;
    double mfCameraFreq;
    unsigned long long mnLastTimeStamp;
    unsigned int mnLastFrameNumber;
    unsigned int mnMissingFrames;
    unsigned int mnReceivedFrames;
    int mnFrameNumberDiff;
    unsigned int mnMaxFrameNumberDiff;
    COORD mPrintPos;
    unsigned int mFrameNumberResets;
    unsigned int mTimestampResets;
};


#endif // OUTPUT_H