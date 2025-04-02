#ifndef UAV_APPLICATION_H
#define UAV_APPLICATION_H

#include "ns3/application.h"
#include "ns3/socket.h"
#include "../priority/priority-tag.h"
#include "../priority/priority-tx-queue.h"

namespace ns3 {

enum Priority {
    PRIO_CRITICAL = 0,  // Emergency commands
    PRIO_HIGH = 1,      // Telemetry alerts
    PRIO_NORMAL = 2,    // Regular telemetry
    PRIO_LOW = 3        // Sensor data
};

class UavApplication : public Application {
public:
    
    static TypeId GetTypeId();
    
    void SetSocket(Ptr<Socket> socket);
    void SendWithPriority(Ptr<Packet> packet, Priority priority);

protected:
    virtual void DoInitialize();
    Ptr<Socket> m_socket;
};

} // namespace ns3

#endif