// #include <Arduino.h>

// #include <lora_client.hpp>

// struct JhLoraClient : LoraClient
// {
//     enum LoraFlag 
//     { 
//         LoraTx, 
//         LoraRx 
//     };

//     LoraFlag LoraState;

//     uint8_t RunDelay = 500, Verify, TranFinish = 0;

//     void Interrupt()
//     {
//         Logout.printf("InterruptLora: %d\n", LoraState);
//         if(LoraRx == LoraState)
//         {
//             Logout.println("RX Done");
//             uint8_t recvlen = 0;
//             uint8_t buffer[64] = { 0 };
//             recvlen = PacketRx(buffer, sizeof(buffer));
//             if (recvlen && recvlen <= sizeof(buffer))
//             {
//                 Logout.printf("RSSI is %d dBm\nPkt RSSI is %d dBm\n", (-164 + SPIRead(LR_RegRssiValue)), (-164 + SPIRead(LR_RegPktRssiValue)));
//                 Logout.printf("Recv Len %d Data : %s\n", recvlen, buffer);
//                 // if(TranFinish != 0 && Verify == buffer[0])
//                 // {
//                 //     TranFinish = 0;
//                 // }
//                 // LoraTranAdjust(!TranFinish, &RunDelay);
//             }
//         }
//         else
//         {
//             Logout.println("TX Done");
//             LoraState = LoraTx;
//             SPIRead(LR_RegIrqFlags);
//             LoraSx1278::LoRaClearIrq();
//             LoraSx1278::Standby();
//             EntryRx();
//         }
//     }

//     void Poll()
//     {       
//         if(LoraState == LoraTx && !TranFinish++)
//         {
//             static uint64_t sent_count = 0;
//             uint8_t buffer[64] = { 0 };
//             sprintf((char *)buffer, "%020d0123456789", sent_count);
//             sent_count++;

//             Verify = 0;
//             for(uint8_t i = 0; i < 30; i++) Verify += buffer[i];

//             // // Logout.println("Sending");

//             // // delay(RunDelay);
            
//             // // Logout.println("RunDelay");

//             EntryTx(30);
            
//             Logout.printf("Result : %d Send : %s\n", PacketTx(buffer, 30), buffer);
//         }
//     }
// };

// JhLoraClient LoraDevice; 

// static void InterruptLoraRecvd(void)
// {
//     LoraDevice.Interrupt();
// }

// void setup()
// {
//     Serial.begin(115200);

//     LoraDevice.LoraState = JhLoraClient::LoraTx;
//     LoraDevice.ConfigSx1278(5, 26, 27);
//     LoraDevice.SetOnDIORise(26, InterruptLoraRecvd, true);

// }

// void loop()
// {
//     delay(50);

//     LoraDevice.Poll();
// }

#include <Arduino.h>

#include <lora_client.hpp>

#include <cppQueue.h>

const uint TranMax = 5;

struct JhLoraClient : LoraClient
{
    enum LoraFlag 
    { 
        LoraTx, 
        LoraTxing,
        LoraRx,
        LoraRxing, 
    };

    LoraFlag LoraState;

    uint16_t RunDelay = 2000;
    uint8_t Verify = 0;

    void Interrupt()
    {
        Logout.printf("InterruptLora: %d\n", LoraState);
        switch(LoraState)
        {
            case LoraTxing:
            {
                SPIRead(LR_RegIrqFlags);
                LoraSx1278::LoRaClearIrq();
                LoraSx1278::Standby();
                Logout.println("TX Done");
                LoraState = LoraRxing;
                EntryRx();
                break;
            }
            case LoraRxing:
            {
                Logout.println("RX Done");
                uint8_t recvlen = 0, Result = false, Temp = 0;
                recvlen = PacketRx(&Temp, sizeof(Temp));
                if (recvlen && recvlen <= sizeof(Temp))
                {
                    Logout.printf("RSSI is %d dBm Pkt RSSI is %d dBm\n", (-164 + SPIRead(LR_RegRssiValue)), (-164 + SPIRead(LR_RegPktRssiValue)));
                    if(Verify == Temp)
                    {
                        TimeOut = 0;
                        Result = true;
                        LoraState = LoraTx;
                    }
                    LoraClient::LoraTranAdjust(Result, &RunDelay);
                }
                break;
            }
        }
    }

    unsigned long TimeOut = 0;

    void Poll()
    {
        if(LoraTx == LoraState || (millis() - TimeOut) > 10000)
        {
            TimeOut = millis();
            srand(TimeOut);
            uint8_t buffer[64] = { 0 };
            sprintf((char *)buffer, "%04d", rand() % 10000);

            Verify = 0;
            for(uint8_t i = 0; i < TranMax; i++) Verify += buffer[i];
           
            Logout.printf("RunDelay %d\n", RunDelay);

            LoraClient::LoraTranAdjust(LoraRxing != LoraState, &RunDelay);
            delay(RunDelay);
 
            if(EntryTx(TranMax))
            {
                PacketTx(buffer, TranMax);

                Logout.printf("Result : %d Send : %s\n", TranMax, buffer);
            }

            LoraState = LoraTxing;
        }
    }
};

JhLoraClient LoraDevice; 

static void InterruptLoraRecvd(void)
{
    LoraDevice.Interrupt();
}

void setup()
{
    Serial.begin(115200);
    LoraDevice.LoraState = JhLoraClient::LoraTx;
    LoraDevice.ConfigSx1278(5, 26, 27);
    LoraDevice.SetOnDIORise(26, InterruptLoraRecvd, true);
    // CAD
}

void loop()
{
    delay(1000);
    LoraDevice.Poll();
}
