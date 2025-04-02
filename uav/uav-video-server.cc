#include "uav-video-server.h"
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"

NS_LOG_COMPONENT_DEFINE("UavVideoServer");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(UavVideoServer);

TypeId UavVideoServer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UavVideoServer")
        .SetParent<VideoStreamServer>()
        .AddConstructor<UavVideoServer>();
    return tid;
}

UavVideoServer::UavVideoServer() {}

void UavVideoServer::SendPacket(ClientInfo* client, uint32_t packetSize)
{
    NS_LOG_FUNCTION(this << client << packetSize);
    
    if (!client || !m_socket) {
        NS_LOG_ERROR("Invalid client or socket");
        return;
    }
    
    uint8_t priority = CalculateVideoPriority(client->m_videoLevel);
    
    uint8_t dataBuffer[packetSize];
    sprintf((char*)dataBuffer, "%u", client->m_sent);
    Ptr<Packet> p = Create<Packet>(reinterpret_cast<const uint8_t*>(dataBuffer), packetSize);
    
    PriorityTag priorityTag(priority);
    p->AddPacketTag(priorityTag);
    
    if (m_socket->SendTo(p, 0, client->m_address) < 0)
    {
        NS_LOG_WARN("Failed to send video packet with priority " << (int)priority);
    }
}


uint8_t UavVideoServer::CalculateVideoPriority(uint16_t videoLevel) const
{
    // Higher quality = higher priority by default
    if (videoLevel >= 4) return 2; // Normal priority for HD/4K
    if (videoLevel >= 2) return 3; // Low for 720p/480p
    return 3; // Low priority for very low quality
}

} // namespace ns3