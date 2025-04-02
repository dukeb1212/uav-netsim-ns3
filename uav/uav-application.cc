#include "uav-application.h"
#include "../priority/priority-tx-queue.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("UavApplication");
NS_OBJECT_ENSURE_REGISTERED(UavApplication);

TypeId
UavApplication::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UavApplication")
        .SetParent<Application>()
        .AddConstructor<UavApplication>();
    return tid;
}

void
UavApplication::DoInitialize()
{
    Application::DoInitialize();
    
    // Ensure socket is properly configured
    if (!m_socket) {
        NS_FATAL_ERROR("Socket not set for UAV application");
    }
}

void
UavApplication::SetSocket(Ptr<Socket> socket)
{
    m_socket = socket;
}

void
UavApplication::SendWithPriority(Ptr<Packet> packet, Priority priority) {
    if (!m_socket) {
        NS_LOG_ERROR("No socket available for transmission");
        return;
    }

    PriorityTag priorityTag(static_cast<uint8_t>(priority));
    packet->AddPacketTag(priorityTag);
    NS_LOG_DEBUG("Sending packet with priority " << (int)priority << " size: " << packet->GetSize());
    
    int bytesSent = m_socket->Send(packet);
    if (bytesSent > 0) {
        NS_LOG_DEBUG("Bytes sent: " << bytesSent);
    } else {
        NS_LOG_ERROR("Failed to send packet");
    }
}

} // namespace ns3