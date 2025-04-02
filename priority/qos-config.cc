#include "qos-config.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(QosConfig);

TypeId QosConfig::GetTypeId() {
    static TypeId tid = TypeId("ns3::QosConfig")
        .SetParent<Object>()
        .AddConstructor<QosConfig>();
    return tid;
}

QosConfig::QosConfig() : m_weights({50, 30, 15, 5}) {}

void QosConfig::SetQueueWeights(const std::vector<uint32_t>& weights) {
    m_weights = weights;
}

uint32_t QosConfig::GetPriorityBandwidth(uint8_t priority) const {
    return m_weights.at(priority);
}

uint8_t QosConfig::GetNumPriorities() const {
    return m_weights.size();
}

} // namespace ns3