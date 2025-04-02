#ifndef PRIORITY_TX_QUEUE_H
#define PRIORITY_TX_QUEUE_H

#include "ns3/queue.h"
#include "qos-config.h"
#include <map>
#include <queue>

namespace ns3 {
/**
 * \ingroup uav
 * \brief Priority-based transmission queue for QoS support
 *
 * This queue implements weighted fair queuing based on packet priorities.
 * It maintains separate sub-queues for each priority level and services
 * them according to configured bandwidth weights.
 */
class PriorityTxQueue : public Queue<Packet> {
public:
    /**
     * \brief Get the TypeId for this class
     */
    static TypeId GetTypeId();
    
    PriorityTxQueue();
    
    /**
     * \brief Set the QoS configuration
     * \param qos The QoS configuration object
     */
    void SetQosConfig(Ptr<QosConfig> qos);

    /**
     * \brief Set the cycle budget bytes
     * \param cycleBudget The size of cycle budget in bytes
     */
    void SetCycleBudget(uint32_t cycleBudget);
    
    // Overridden from Queue<Packet>
    bool Enqueue(Ptr<Packet> p) override;
    Ptr<Packet> Dequeue() override;
    Ptr<Packet> Remove() override;
    Ptr<const Packet> Peek() const override;
    
    /**
     * \brief Peek at a specific priority queue
     * \param priority The priority level to peek
     * \return The front packet of the specified priority queue
     */
    Ptr<const Packet> PeekByPriority(uint8_t priority) const;

private:
    void ResetCycleCounters();
    
    Ptr<QosConfig> m_qosConfig; ///< QoS configuration
    std::map<uint8_t, std::queue<Ptr<Packet>>> m_queues; ///< Priority sub-queues
    std::map<uint8_t, uint32_t> m_bytesSentThisCycle; ///< Bandwidth tracking
    uint32_t m_cycleBudget; // Total bytes per cycle (e.g., 10,000)
    std::map<uint8_t, uint32_t> m_priorityBudgets; // Byte budgets per priority

    // TracedCallback<Ptr<const Packet>> m_traceEnqueue;
    // TracedCallback<Ptr<const Packet>> m_traceDequeue;
};

} // namespace ns3

#endif