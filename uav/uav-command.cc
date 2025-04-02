#include "uav-command.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("UavCommand");
NS_OBJECT_ENSURE_REGISTERED(UavCommand);

TypeId UavCommand::GetTypeId() {
    static TypeId tid = TypeId("ns3::UavCommand")
        .SetParent<UavApplication>()
        .AddConstructor<UavCommand>()
        .AddAttribute("Interval", "Transmission interval",
                     TimeValue(Seconds(3.0)),
                     MakeTimeAccessor(&UavCommand::m_interval),
                     MakeTimeChecker())
        .AddAttribute("PacketSize", "Telemetry packet size",
                     UintegerValue(100),
                     MakeUintegerAccessor(&UavCommand::m_packetSize),
                     MakeUintegerChecker<uint32_t>());
    return tid;
}

UavCommand::UavCommand() 
    : m_running(false),
      m_interval(Seconds(3.0)),
      m_packetSize(100),
      m_sent(0) {}

void UavCommand::StartApplication() {
    m_running = true;
    SendCommandCheck();
}

void UavCommand::StopApplication() {
    m_running = false;
    if(m_sendEvent.IsRunning()) {
        Simulator::Cancel(m_sendEvent);
    }
}

void UavCommand::SetPacketSize(uint32_t size) {
    m_packetSize = size;
}

void UavCommand::SetInterval(Time interval) {
    m_interval = interval;
}

void UavCommand::SendCommandCheck() {
    if(!m_running) {
        NS_LOG_WARN("Attempted to send while not running");
        return;
    }
    if (m_sent >= 50) m_interval = Seconds(3.0);
    else m_interval = Seconds(0.05);
    
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    NS_LOG_INFO("Sending command check, size: " << m_packetSize);
    SendWithPriority(packet, PRIO_HIGH);
    m_sent++;
    
    m_sendEvent = Simulator::Schedule(m_interval, &UavCommand::SendCommandCheck, this);
}

void UavCommand::SendCommand(Priority priority) {
    if(!m_running) {
        NS_LOG_WARN("Attempted to send while not running");
        return;
    }
    
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    NS_LOG_INFO("Sending command check, size: " << m_packetSize);
    SendWithPriority(packet, priority);
}

} // namespace ns3