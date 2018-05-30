#include <Arduino.h>

#include <lora_client.hpp>

#include <cppQueue.h>

const uint TranMax = 5;

Queue cache(TranMax, 8, LIFO);	// Instantiate queue

struct JhLoraClient : LoraClient
{
    enum LoraFlag 
    { 
        LoraTx, 
        LoraTxing,
        LoraRx,
        LoraRxing, 
    };

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
                uint8_t recvlen = 0;
                uint8_t buffer[TranMax] = { 0 };
                recvlen = PacketRx(buffer, sizeof(buffer));
                if (recvlen && recvlen <= sizeof(buffer))
                {
                    Logout.printf("RSSI is %d dBm\nPkt RSSI is %d dBm\n", (-164 + SPIRead(LR_RegRssiValue)), (-164 + SPIRead(LR_RegPktRssiValue)));
                    cache.push(buffer);
                    LoraState = LoraTx;
                }
                break;
            }
        }
    }

    LoraFlag LoraState;

    void Poll()
    {
        uint8_t buffer[TranMax] = { 0 };
        if(LoraTx == LoraState && cache.pop(buffer))
        {
            Logout.printf("Recv : %.*s\n", TranMax, buffer);
            uint8_t Verify = 0;
            if(EntryTx(sizeof(Verify)))
            {
                for(uint8_t i = 0; i < TranMax; i++) Verify += buffer[i];
                PacketTx(&Verify, sizeof(Verify));
                LoraState = LoraTxing;
            }
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
    LoraDevice.LoraState = JhLoraClient::LoraRxing;
    LoraDevice.ConfigSx1278(5, 26, 27);
    LoraDevice.SetOnDIORise(26, InterruptLoraRecvd, true);
    LoraDevice.EntryRx();
    // CAD
}

void loop()
{
    delay(50);
    LoraDevice.Poll();
}
