#include "Operations.h"

#define LATEST_SELECTABLE_MAJOR_VERSION 1
#define LATEST_SELECTABLE_MINOR_VERSION 22
#define PROGRAM_VERSION 0

int main(int argc, char **argv)
{
    CInput::EOperation eOperation;
    int                nMajorVersion;
    int                nMinorVersion;
    bool               bBigEndian;
    CInput*            poInput      = NULL;
    COutput*           poOutput     = NULL;
    CRTProtocol*       poRTProtocol = NULL;
    COperations*       poOperations = NULL;

    poInput = new CInput();

    printf("\n*****************************************\n");
    printf("   Qualisys RT Client Example v %d.%d.%d\n", LATEST_SELECTABLE_MAJOR_VERSION, LATEST_SELECTABLE_MINOR_VERSION, PROGRAM_VERSION);
    printf("*****************************************");

    // By default assume you want to connect to QTM at the same machine - just for testing
    char pServerAddr[32] = "localhost";

    // Check the command line for the server address
    if (argc > 1)
    {
        strcpy_s(pServerAddr, sizeof(pServerAddr), argv[1]);
    }

    // The base port (as entered in QTM, TCP/IP port number, in the RT output tab of the workspace options.
    unsigned short nBasePort = 22222;
    if (argc > 2)
    {
        nBasePort = (unsigned short)strtoul(argv[2], NULL, 10);
    }
    
    poOutput     = new COutput();
    poRTProtocol = new CRTProtocol();
    poOperations = new COperations(poInput, poOutput, poRTProtocol);

    bool bDiscoverRTServer;
    if (poInput->ReadDiscoverRTServers(bDiscoverRTServer, pServerAddr) == false)
    {
        return 1;
    }

    if (bDiscoverRTServer)
    {
        poOperations->DiscoverRTServers(pServerAddr, sizeof(pServerAddr), &nBasePort);
    }

    // Set latest selectable version.
    nMajorVersion = LATEST_SELECTABLE_MAJOR_VERSION;
    nMinorVersion = LATEST_SELECTABLE_MINOR_VERSION;

    if (poInput->ReadVersion(nMajorVersion, nMinorVersion) == false)
    {
        return 1;
    }

    if (poInput->ReadByteOrder(bBigEndian) == false)
    {
        return 1;
    }

    while (poInput->ReadOperation(eOperation))
    {
        if (eOperation == CInput::DiscoverRTServers)
        {
            poOperations->DiscoverRTServers();
        }
        else
        {
            unsigned short udpServerPort = 0; // Use random port.
            if (poRTProtocol->Connect(pServerAddr, nBasePort, &udpServerPort, nMajorVersion, nMinorVersion, bBigEndian))
            {
                char pVer[64];
                if (poRTProtocol->GetQTMVersion(pVer, sizeof(pVer)))
                {
                    printf("Connected. %s.\n", pVer);
                }
            }
            else
            {
                printf("\nFailed to connect to QTM RT Server. %s\n\n", poRTProtocol->GetErrorString());
                system("pause");
                break;
            }

            switch (eOperation)
            {
            case CInput::DataTransfer :
            case CInput::Statistics:
            case CInput::Noise2D:
                poOperations->DataTransfer(eOperation);
                break;
            case CInput::MonitorEvents :
                poOperations->MonitorEvents();
                break;
            case CInput::DiscoverRTServers :
                poOperations->DiscoverRTServers();
                break;
            case CInput::ViewSettings :
                poOperations->ViewSettings();
                break;
            case CInput::ChangeGeneralSystemSettings :
            case CInput::ChangeExtTimebaseSettings:
            case CInput::ChangeExtTimestampSettings:
            case CInput::ChangeProcessingActionsSettings :
            case CInput::ChangeCameraSettings :
            case CInput::ChangeCameraSyncOutSettings :
            case CInput::Change6dSettings:
            case CInput::ChangeForceSettings:
            case CInput::ChangeImageSettings :
            case CInput::ChangeSkeletonSettings :
                poOperations->ChangeSettings(eOperation);
                break;
            case CInput::ControlQTM :
                poOperations->ControlQTM();
                break;
            default  :
                break;
            }

            poRTProtocol->Disconnect(); // Disconnect from the server
            printf("\n\nDisconnected from QTM RT server.\n\n");
            
            system("pause");
            system("cls");
        }
    }

    delete poOperations;
    delete poRTProtocol;
    delete poOutput;
    delete poInput;

    return 1;
} // main