#ifndef UAV_TELEMETRY_H
#define UAV_TELEMETRY_H

#include "uav-application.h"

namespace ns3 {

class UavTelemetry : public UavApplication {
public:
    static TypeId GetTypeId();
    UavTelemetry();
    
    void StartApplication() override;
    void StopApplication() override;
    void SetPacketSize(uint32_t size);
    void SetInterval(Time interval);

private:
    void SendTelemetryUpdate();
    
    bool m_running;
    Time m_interval;
    uint32_t m_packetSize;
    EventId m_sendEvent;
};

} // namespace ns3

#endif