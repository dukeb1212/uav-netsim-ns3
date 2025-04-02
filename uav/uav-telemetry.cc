#include "uav-telemetry.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("UavTelemetry");
NS_OBJECT_ENSURE_REGISTERED(UavTelemetry);

TypeId UavTelemetry::GetTypeId() {
    static TypeId tid = TypeId("ns3::UavTelemetry")
        .SetParent<UavApplication>()
        .AddConstructor<UavTelemetry>()
        .AddAttribute("Interval", "Transmission interval",
                     TimeValue(Seconds(1.0)),
                     MakeTimeAccessor(&UavTelemetry::m_interval),
                     MakeTimeChecker())
        .AddAttribute("PacketSize", "Telemetry packet size",
                     UintegerValue(150),
                     MakeUintegerAccessor(&UavTelemetry::m_packetSize),
                     MakeUintegerChecker<uint32_t>());
    return tid;
}

UavTelemetry::UavTelemetry() 
    : m_running(false),
      m_interval(Seconds(1.0)),
      m_packetSize(128) {}

void UavTelemetry::StartApplication() {
    m_running = true;
    SendTelemetryUpdate();
}

void UavTelemetry::StopApplication() {
    m_running = false;
    if(m_sendEvent.IsRunning()) {
        Simulator::Cancel(m_sendEvent);
    }
}

void UavTelemetry::SetPacketSize(uint32_t size) {
    m_packetSize = size;
}

void UavTelemetry::SetInterval(Time interval) {
    m_interval = interval;
}

void UavTelemetry::SendTelemetryUpdate() {
    if(!m_running) {
        NS_LOG_WARN("Attempted to send while not running");
        return;
    }
    
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    NS_LOG_INFO("Sending telemetry update, size: " << m_packetSize);
    SendWithPriority(packet, PRIO_NORMAL);
    
    m_sendEvent = Simulator::Schedule(m_interval, &UavTelemetry::SendTelemetryUpdate, this);
}

} // namespace ns3