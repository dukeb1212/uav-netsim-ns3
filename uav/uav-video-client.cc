#include "uav-video-client.h"
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "uav-application.h"

NS_LOG_COMPONENT_DEFINE("UavVideoClient");

namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED(UavVideoClient);

TypeId UavVideoClient::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UavVideoStreamClient")
                        .SetParent<VideoStreamClient>()
                        .AddConstructor<UavVideoClient>();
    return tid;
}

UavVideoClient::UavVideoClient() 
{
}

UavVideoClient::~UavVideoClient() {}




void UavVideoClient::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
  {
    socket->GetSockName (localAddress);
    if (InetSocketAddress::IsMatchingType (from))
    {
      uint8_t recvData[packet->GetSize()];
      packet->CopyData (recvData, packet->GetSize ());
      uint32_t frameNum;
      sscanf ((char *) recvData, "%u", &frameNum);

      if (frameNum == m_lastRecvFrame)
      {
        m_frameSize += packet->GetSize ();
      }
      else
      {
        if (frameNum > 0)
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client received frame " << frameNum-1 << " and " << m_frameSize << " bytes from " <<  InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());
        }

        m_currentBufferSize++;
        m_lastRecvFrame = frameNum;
        m_frameSize = packet->GetSize ();
      }

      // The rebuffering event has happend 3+ times, which suggest the client to lower the video quality.
      if (m_rebufferCounter >= 3)
      {
        if (m_videoLevel > 1)
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s: Lower the video quality level!");
          m_videoLevel--;
          // reflect the change to the server
          uint8_t dataBuffer[10];
          sprintf((char *) dataBuffer, "%hu", m_videoLevel);
          Ptr<Packet> levelPacket = Create<Packet> (dataBuffer, 10);

          PriorityTag priorityTag(PRIO_HIGH);
          levelPacket->AddPacketTag(priorityTag);
          socket->SendTo (levelPacket, 0, from);
          m_rebufferCounter = 0;
        }
      }
      
      // If the current buffer size supports 5+ seconds video, we can try to increase the video quality level.
      if (m_currentBufferSize > 5 * m_frameRate)
      {
        if (m_videoLevel < MAX_VIDEO_LEVEL)
        {
          m_videoLevel++;
          // reflect the change to the server
          uint8_t dataBuffer[10];
          sprintf((char *) dataBuffer, "%hu", m_videoLevel);
          Ptr<Packet> levelPacket = Create<Packet> (dataBuffer, 10);
          socket->SendTo (levelPacket, 0, from);
          m_currentBufferSize = m_frameRate;
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds() << "s: Increase the video quality level to " << m_videoLevel);
        }
      }
    }
  }
}

}