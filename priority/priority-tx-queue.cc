#include "priority-tx-queue.h"
#include "priority-tag.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE("PriorityTxQueue");
NS_OBJECT_ENSURE_REGISTERED(PriorityTxQueue);

TypeId PriorityTxQueue::GetTypeId() {
    static TypeId tid = TypeId("ns3::PriorityTxQueue")
        .SetParent<Queue<Packet>>()
        .SetGroupName("Uav")
        .AddConstructor<PriorityTxQueue>();
        // .AddTraceSource("Enqueue", "Packet enqueued",
        //               MakeTraceSourceAccessor(&PriorityTxQueue::m_traceEnqueue),
        //               "ns3::Packet::TracedCallback")
        // .AddTraceSource("Dequeue", "Packet dequeued",
        //               MakeTraceSourceAccessor(&PriorityTxQueue::m_traceDequeue),
        //               "ns3::Packet::TracedCallback");
    return tid;
}

PriorityTxQueue::PriorityTxQueue() : m_cycleBudget(12500) {
}

void PriorityTxQueue::SetQosConfig(Ptr<QosConfig> qos) {
    //NS_LOG_DEBUG( "QOS: " << qos);
    m_qosConfig = qos;
    
    // Calculate byte budgets for each priority
    for(uint8_t i = 0; i < qos->GetNumPriorities(); i++) {
        m_priorityBudgets[i] = (qos->GetPriorityBandwidth(i) * m_cycleBudget) / 100;
        //NS_LOG_DEBUG("Priority " << (int)i << " budget: " << m_priorityBudgets[i] << " bytes");
    }
    ResetCycleCounters();
}

bool PriorityTxQueue::Enqueue(Ptr<Packet> p) {
    PriorityTag priorityTag;
    bool found = p->PeekPacketTag(priorityTag);
    uint8_t priority = found ? priorityTag.GetPriority() : 2; // Default to normal
    
    //NS_LOG_DEBUG("Enqueuing packet with priority " << (int)priority << " size: " << p->GetSize());
    m_queues[priority].push(p);
    //m_traceEnqueue(p);
    return true;
}

Ptr<Packet> PriorityTxQueue::Dequeue() 
{
    //NS_LOG_FUNCTION(this);
    
    // First check if we need to reset counters
    uint32_t totalSent = 0;
    for (const auto& counter : m_bytesSentThisCycle) {
        totalSent += counter.second;
    }
    
    if (totalSent >= m_cycleBudget) {
        ResetCycleCounters();
        return nullptr;
    }

    // Try each priority queue in order
    for (uint8_t prio = 0; prio < m_qosConfig->GetNumPriorities(); ++prio) {
        auto queueIt = m_queues.find(prio);
        if (queueIt == m_queues.end() || queueIt->second.empty()) {
            continue;
        }

        uint32_t remainingBudget = m_priorityBudgets[prio] - m_bytesSentThisCycle[prio];
        if (remainingBudget > 0) {
            Ptr<Packet> p = queueIt->second.front();
            if (!p) {
                queueIt->second.pop();
                continue;
            }

            uint32_t pktSize = p->GetSize();
            if (pktSize <= remainingBudget) {
                queueIt->second.pop();
                m_bytesSentThisCycle[prio] += pktSize;
                //NS_LOG_LOGIC("Dequeued packet size " << pktSize << " from queue " << (int)prio);
                return p;
            }
        }
    }
    
    //NS_LOG_LOGIC("No packets available for dequeue within budget");
    return nullptr;
}
Ptr<Packet> PriorityTxQueue::Remove() {
    for(uint8_t i = 0; i < m_qosConfig->GetNumPriorities(); i++) {
        auto queueIt = m_queues.find(i);
        if(queueIt != m_queues.end() && !queueIt->second.empty()) {
            Ptr<Packet> p = queueIt->second.front();
            queueIt->second.pop();
            return p;
        }
    }
    return nullptr;
}

Ptr<const Packet> PriorityTxQueue::Peek() const {
    for(uint8_t i = 0; i < m_qosConfig->GetNumPriorities(); i++) {
        auto queueIt = m_queues.find(i);
        auto bytesIt = m_bytesSentThisCycle.find(i);
        
        if(queueIt != m_queues.end() && !queueIt->second.empty() && 
           bytesIt != m_bytesSentThisCycle.end() && 
           bytesIt->second < m_qosConfig->GetPriorityBandwidth(i)) {
            return queueIt->second.front();
        }
    }
    return nullptr;
}

Ptr<const Packet> PriorityTxQueue::PeekByPriority(uint8_t priority) const {
    auto queueIt = m_queues.find(priority);
    if(queueIt == m_queues.end() || queueIt->second.empty()) {
        return nullptr;
    }
    return queueIt->second.front();
}

void PriorityTxQueue::SetCycleBudget(uint32_t cycleBudget) { 
    m_cycleBudget = cycleBudget; 
}

void PriorityTxQueue::ResetCycleCounters() {
    for(auto& counter : m_bytesSentThisCycle) {
        counter.second = 0;
    }
}

} // namespace ns3