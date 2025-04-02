#ifndef UAV_VIDEO_SERVER_H
#define UAV_VIDEO_SERVER_H

#include "ns3/video-stream-server.h"
#include "../priority/priority-tag.h"

namespace ns3 {

class Socket;
class Packet;

class UavVideoServer : public VideoStreamServer
{
public:
    static TypeId GetTypeId();
    UavVideoServer();
    
protected:
    virtual void SendPacket(ClientInfo* client, uint32_t packetSize) override;
    
private:
    uint8_t CalculateVideoPriority(uint16_t videoLevel) const;
};

} // namespace ns3

#endif