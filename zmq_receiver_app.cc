#include "zmq_receiver_app.h"
#include "ns3/string.h"   
#include "ns3/uinteger.h"  
#include "ns3/pointer.h"
#include "ns3/node-list.h"
#include <time.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ZmqReceiverApp");

TypeId ZmqReceiverApp::GetTypeId()
{
    static TypeId tid = TypeId("ZmqReceiverApp")
        .SetParent<Application>()
        .SetGroupName("Zmq")
        .AddAttribute("Address", "The address of the publisher",
                      StringValue("localhost"),
                      MakeStringAccessor(&ZmqReceiverApp::m_address),
                      MakeStringChecker())
        .AddAttribute("Port", "The port of the publisher",
                      UintegerValue(5555),
                      MakeUintegerAccessor(&ZmqReceiverApp::m_port),
                      MakeUintegerChecker<uint16_t>())
        .AddAttribute("ID", "The topic indicates id of the publisher",
                      StringValue("ns3"),
                      MakeStringAccessor(&ZmqReceiverApp::m_id),
                      MakeStringChecker())
        .AddConstructor<ZmqReceiverApp>();
    return tid;
}

ZmqReceiverApp::ZmqReceiverApp()
    : m_running(false),
      m_thread(nullptr),
      m_address("localhost"),
      m_port(5555),
      m_context(1),
      m_socket(m_context, ZMQ_SUB),
      m_id("ns3"),
      m_heartBeatTopic("heartbeat")
{
    
}

void ZmqReceiverApp::StartApplication()
{
    m_running = true;
    m_socket.set(zmq::sockopt::subscribe, m_id);
    //m_socket.set(zmq::sockopt::subscribe, m_heartBeatTopic);
    m_socket.connect("tcp://" + m_address + ":" + std::to_string(m_port));
    m_thread = std::make_unique<std::thread>(&ZmqReceiverApp::Run, this);
}

void ZmqReceiverApp::StopApplication()
{
    m_running = false;
    if (m_thread && m_thread->joinable())
    {
        m_thread->join();
    }
}

void ZmqReceiverApp::Run()
{
    while (m_running)
    {
        zmq::recv_result_t result = m_socket.recv(m_message, zmq::recv_flags::none);
        if (!result)
        {
            NS_LOG_ERROR("Failed to receive message: " << zmq_strerror(zmq_errno()));
            break;
        }

        std::string message = std::string(static_cast<char*>(m_message.data()), m_message.size());

        // Parse JSON message
        try
        {
            json jsonData = json::parse(message.substr(m_id.size() + 1));
    
            if (jsonData.contains("actors"))
            {
                for (auto& actor : jsonData["actors"])
                {
                    std::string id = actor["id"];
                    double x = actor["x"];
                    double y = actor["y"];
                    double z = actor["z"];

                    //NS_LOG_INFO("Received Position for " << id << ": (" << x << ", " << y << ", " << z << ")");

                    Vector position(x, y, z);
                    uint32_t nodeId;

                    // Handle different actor types
                    if (id.find("uav") == 0) {
                        // Extract UAV number from ID (e.g., "uav1" -> 1)
                        try {
                            nodeId = static_cast<uint32_t>(std::stoi(id.substr(3)));
                        } catch (const std::exception& e) {
                            NS_LOG_WARN("Invalid UAV ID format: " << id);
                            continue;
                        }
                    } else if (id == "gcs") {
                        // Assign ground control station to node 0
                        nodeId = 0;
                    } else {
                        NS_LOG_WARN("Unknown actor type: " << id);
                        continue;
                    }

                    // Find and update the node position
                    Ptr<Node> node = NodeList::GetNode(nodeId);
                    if (node) {
                        SetNodePosition(node, position);
                    } else {
                        NS_LOG_WARN("Node not found for ID: " << nodeId);
                    }
                }
            }
        }
        catch (json::parse_error& e)
        {
            NS_LOG_ERROR("JSON Parsing Error: " << e.what());
        }
    }
}

Vector ZmqReceiverApp::PositionConverter(std::string message)
{
    std::istringstream iss(message);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, ' '))
    {
        tokens.push_back(token);
    }

    if (tokens.size() < 4)
    {
        NS_LOG_ERROR("Invalid message format: " << message);
        return Vector();
    }

    std::string id = tokens[0];

    // Extract numeric values by removing the prefix (X=, Y=, Z=)
    double x = std::stod(tokens[1].substr(2));  // Remove "X="
    double y = std::stod(tokens[2].substr(2));  // Remove "Y="
    double z = std::stod(tokens[3].substr(2));  // Remove "Z="

    NS_LOG_INFO("Position of " << id << ": (" << x << ", " << y << ", " << z << ")");

    Vector position(x, y, z);
    return position;
}


void ZmqReceiverApp::SetNodePosition(Ptr<Node> node, Vector position)
{
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    if (mobility != nullptr)
    {
        mobility->SetPosition(position);
        //NS_LOG_INFO(mobility->GetPosition().x << " " << mobility->GetPosition().y << " " << mobility->GetPosition().z);
    }
    else
    {
        NS_LOG_ERROR("MobilityModel is null for node: " << node->GetId());
    }
}