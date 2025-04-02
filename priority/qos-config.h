#ifndef QOS_CONFIG_H
#define QOS_CONFIG_H

#include "ns3/object.h"
#include <vector>

namespace ns3 {

/**
 * \ingroup uav
 * \brief QoS Configuration for priority-based bandwidth allocation
 *
 * This class manages the weighted bandwidth allocation between different
 * priority levels. The weights determine what percentage of available
 * bandwidth each priority queue receives.
 *
 * Default weights (sum should be 100):
 * - Priority 0 (Critical): 50%
 * - Priority 1 (High): 30%
 * - Priority 2 (Normal): 15%
 * - Priority 3 (Low): 5%
 */
class QosConfig : public Object {
public:
    /**
     * \brief Get the TypeId for this class
     */
    static TypeId GetTypeId();
    
    /**
     * \brief Default constructor with default weights
     */
    QosConfig();
    
    /**
     * \brief Set the bandwidth allocation weights
     * \param weights Vector of weights (sum should equal 100)
     *
     * Example: {50, 30, 15, 5} for 4 priority levels
     */
    void SetQueueWeights(const std::vector<uint32_t>& weights);
    
    /**
     * \brief Get the bandwidth percentage for a priority level
     * \param priority The priority level (0-based index)
     * \return The allocated bandwidth percentage
     */
    uint32_t GetPriorityBandwidth(uint8_t priority) const;
    
    /**
     * \brief Get the number of configured priority levels
     * \return Number of priority levels
     */
    uint8_t GetNumPriorities() const;

private:
    std::vector<uint32_t> m_weights; ///< Bandwidth allocation weights
};

} // namespace ns3

#endif