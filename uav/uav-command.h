#ifndef UAV_COMMAND_H
#define UAV_COMMAND_H

#include "uav-application.h"

namespace ns3 {

class UavCommand : public UavApplication {
public:
    static TypeId GetTypeId();
    UavCommand();
    
    void StartApplication() override;
    void StopApplication() override;
    void SetPacketSize(uint32_t size);
    void SetInterval(Time interval);
    void SendCommand(Priority priority);

private:
    void SendCommandCheck();
    
    bool m_running;
    Time m_interval;
    uint32_t m_packetSize;
    EventId m_sendEvent;
    uint32_t m_sent;
};

} // namespace ns3

#endif