#ifndef UAV_VIDEO_CLIENT_H
#define UAV_VIDEO_CLIENT_H

#include "ns3/video-stream-client.h"
#include "../priority/priority-tx-queue.h"

namespace ns3 {

class Socket;
class Packet;
    
class UavVideoClient : public VideoStreamClient 
{
public: 
    static TypeId GetTypeId();

    UavVideoClient();
    virtual ~UavVideoClient();

private:

    virtual void HandleRead (Ptr<Socket> socket) override;

    //uint8_t m_priority;
};
}


#endif
