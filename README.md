# qualisys_cpp_sdk
C++ sdk for talking to Qualisys Track Manager software.

At the moment the C++ sdk source code is part of QTM installer (Qualisys/Qualisys Track Manager/RT Protocol in installation directory).

Below is a simple example of 6dof streaming.

```cpp

#include "RTProtocol.h"
#include "RTPacket.h"
#include <conio.h>


int main(int argc, char **argv)
{
    CRTProtocol rtProtocol;

    const char           serverAddr[] = "localhost";
    const unsigned short basePort     = 22222;
    const int            majorVersion = 1;
    const int            minorVersion = 18;
    const bool           bigEndian    = false;
    const char           password[]   = "password";

    if (!rtProtocol.Connect(serverAddr, basePort, 0, majorVersion, minorVersion, bigEndian))
        return 1;

    if (!rtProtocol.TakeControl(password))
        return 1;

    if (!rtProtocol.StartRTOnFile())
        return 1;
      
    bool dataAvailable;
    if (!rtProtocol.Read6DOFSettings(dataAvailable))
        return 1;

    if (!dataAvailable)
        return 1;

    if (!rtProtocol.StreamFrames(CRTProtocol::RateAllFrames, 0, 0, NULL, CRTProtocol::cComponent6d))
        return 1;

    printf("Streaming 6DOF data\n\n");
    
    bool abort = false;
    
    while (!abort)
    {
        CRTPacket::EPacketType packetType;

        if (rtProtocol.ReceiveRTPacket(packetType, true) > 0)
        {
            if (packetType == CRTPacket::PacketData)
            {
                float fX, fY, fZ;
                float rotationMatrix[9];

                CRTPacket* rtPacket = rtProtocol.GetRTPacket();

                printf("Timestamp %10I64d \tFrame %d\n", rtPacket->GetTimeStamp(), rtPacket->GetFrameNumber());
                printf("======================================================================================================================\n");

                for (unsigned int i = 0; i < rtPacket->Get6DOFBodyCount(); i++)
                {
                    if (rtPacket->Get6DOFBody(i, fX, fY, fZ, rotationMatrix))
                    {
                        const char* pTmpStr = rtProtocol.Get6DOFBodyName(i);
                        if (pTmpStr)
                        {
                            printf("%-12s ", pTmpStr);
                        }
                        else
                        {
                            printf("Unknown     ");
                        }
                        printf("Pos: %9.3f %9.3f %9.3f    Rot: %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f\n",
                            fX, fY, fZ, rotationMatrix[0], rotationMatrix[1], rotationMatrix[2],
                            rotationMatrix[3], rotationMatrix[4], rotationMatrix[5], rotationMatrix[6], rotationMatrix[7], rotationMatrix[8]);
                    }
                }
                printf("\n");
            }
        }

        if (abort = (_kbhit() != 0))
        {
            _getch(); // Consume key pressed
        }
    }
    rtProtocol.StopCapture();
    return 1;
}
```
