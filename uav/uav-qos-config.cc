#include "uav-qos-config.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(UavQosConfig);

TypeId UavQosConfig::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UavQosConfig")
        .SetParent<QosConfig>()
        .AddConstructor<UavQosConfig>();
    return tid;
}

UavQosConfig::UavQosConfig()
    : m_currentMode(NORMAL)
{
    // Initialize with default weights
    SetQueueWeights({50, 30, 15, 5});
}

void UavQosConfig::SetOperationMode(OperationMode mode)
{
    m_currentMode = mode;
    
    switch(mode) {
        case EMERGENCY:
            SetQueueWeights({70, 20, 5, 5});
            break;
        case LOW_BANDWIDTH:
            SetQueueWeights({40, 30, 20, 10});
            break;
        case HIGH_QUALITY_VIDEO:
            SetQueueWeights({20, 20, 35, 25});
            break;
        default: // NORMAL
            SetQueueWeights({50, 25, 15, 10});
    }
}

UavQosConfig::OperationMode UavQosConfig::GetCurrentMode() const
{
    return m_currentMode;
}

} // namespace ns3