#include "priority-tag.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("PriorityTag");
NS_OBJECT_ENSURE_REGISTERED(PriorityTag);

TypeId PriorityTag::GetTypeId() {
    static TypeId tid = TypeId("ns3::PriorityTag")
        .SetParent<Tag>()
        .AddConstructor<PriorityTag>();
    return tid;
}

TypeId PriorityTag::GetInstanceTypeId() const {
    return GetTypeId();
}

void PriorityTag::Serialize(TagBuffer buf) const {
    NS_LOG_DEBUG("Serialized Priority " << m_priority);
    buf.WriteU8(m_priority);
}

void PriorityTag::Deserialize(TagBuffer buf) {
    m_priority = buf.ReadU8();
    NS_LOG_DEBUG("Deserialized priority: " << (int)m_priority);
}

uint32_t PriorityTag::GetSerializedSize() const {
    return 1; // Single byte tag
}

void PriorityTag::Print(std::ostream &os) const {
    os << "Priority=" << (int)m_priority;
}

uint8_t PriorityTag::GetPriority() const { 
    NS_LOG_DEBUG("Getting priority: " << (int)m_priority);
    return m_priority; 
}

void PriorityTag::SetPriority(uint8_t priority) { 
    NS_LOG_DEBUG("Setting priority: " << (int)priority);
    m_priority = priority; 
}
} // namespace ns3