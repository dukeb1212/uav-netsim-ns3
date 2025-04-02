#ifndef PRIORITY_TAG_H
#define PRIORITY_TAG_H

#include "ns3/tag.h"

namespace ns3 {

/**
 * \ingroup uav
 * \brief Packet tag for storing priority information
 *
 * This tag is used to mark packets with priority levels that will be
 * honored by the PriorityTxQueue. The priority values are:
 * - 0: Critical (highest priority)
 * - 1: High
 * - 2: Normal (default)
 * - 3: Low (lowest priority)
 */
class PriorityTag : public Tag {
public:
    /**
     * \brief Get the TypeId for this class
     */
    static TypeId GetTypeId();
    
    // Inherited from Tag
    virtual TypeId GetInstanceTypeId() const;
    
    /**
     * \brief Construct with default priority (2 - Normal)
     */
    PriorityTag() : m_priority(2) {}
    
    /**
     * \brief Construct with specific priority
     * \param priority The priority level (0-3)
     */
    PriorityTag(uint8_t priority) : m_priority(priority) {}
    
    // Tag serialization methods
    virtual void Serialize(TagBuffer buf) const;
    virtual void Deserialize(TagBuffer buf);
    virtual uint32_t GetSerializedSize() const;
    virtual void Print(std::ostream &os) const;
    
    /**
     * \brief Get the priority value
     * \return The priority level (0-3)
     */
    uint8_t GetPriority() const;
    
    /**
     * \brief Set the priority value
     * \param priority The priority level (0-3)
     */
    void SetPriority(uint8_t priority);

private:
    uint8_t m_priority; ///< Storage for the priority value
};

} // namespace ns3

#endif