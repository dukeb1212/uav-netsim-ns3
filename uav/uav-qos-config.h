#ifndef UAV_QOS_CONFIG_H
#define UAV_QOS_CONFIG_H

#include "../priority/qos-config.h"

namespace ns3 {

class UavQosConfig : public QosConfig
{
public:
    enum OperationMode {
        NORMAL,
        EMERGENCY,
        LOW_BANDWIDTH,
        HIGH_QUALITY_VIDEO
    };
    
    static TypeId GetTypeId();
    UavQosConfig();
    
    void SetOperationMode(OperationMode mode);
    OperationMode GetCurrentMode() const;
    
private:
    OperationMode m_currentMode;
};

} // namespace ns3

#endif