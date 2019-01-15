#ifndef OPERATIONS_H
#define OPERATIONS_H


#include "Input.h"
#include "Output.h"
#include "RTProtocol.h"


class COperations
{
public:
    COperations(CInput* mpoInput, COutput* poOutput, CRTProtocol* poRTProtocol);
    void MonitorEvents();
    void DiscoverRTServers(char* tServerAddr = NULL, int nServerAddrLen = 0, unsigned short* pnPort = NULL);
    void ViewSettings();
    void ChangeSettings(CInput::EOperation eOperation);
    void DataTransfer(CInput::EOperation operation);
    void ControlQTM();

private:
    bool TakeQTMControl();
    bool ReleaseQTMControl();

private:
    CInput*      mpoInput;
    COutput*     mpoOutput;
    CRTProtocol* mpoRTProtocol;
    bool         mSkeletonGlobalReferenceFrame = false;
};


#endif
