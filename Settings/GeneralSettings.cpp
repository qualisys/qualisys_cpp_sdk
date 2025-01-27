#include "RTProtocol.h"



class CRTProtocol
{
public:
    struct SSettingsGeneral
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
};
