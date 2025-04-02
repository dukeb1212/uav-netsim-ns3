#include "zmq_receiver_app.h"
#include "ns3/core-module.h"
#include "ns3/string.h"   
#include "ns3/uinteger.h"  
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/pointer.h"
#include "ns3/node-list.h"
#include <time.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include "uav/uav-telemetry.h"
#include "uav/uav-command.h"
#include "ns3/applications-module.h"

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

                    NS_LOG_INFO("Received Position for " << id << ": (" << x << ", " << y << ", " << z << ")");

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
            } else if (jsonData.contains("event_type"))
            {
                std::string command = jsonData["event_type"];
                std::string app_type = jsonData["app_type"];
                // AppType type = getAppTypeFromString(app_type);

                Ptr<Node> gcsNode = NodeList::GetNode(0);
                Ptr<Node> uavNode = NodeList::GetNode(1);
                uint32_t numApplication;

                if (command == "start") {
                    // Handle application start
                    std::string config = jsonData["config"];
                    int local_id = jsonData["local_id"];
                    Ipv4InterfaceAddress gcsInterfAddress = gcsNode->GetObject<Ipv4>()->GetAddress(1, 0);
                    Ipv4Address gcsAddress = gcsInterfAddress.GetAddress();

                    Ipv4InterfaceAddress uavInterfAddress = uavNode->GetObject<Ipv4>()->GetAddress(1, 0);
                    Ipv4Address uavAddress = uavInterfAddress.GetAddress();

                    if (app_type == "Telemetry") {
                        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
                        Ptr<Socket> criticalSocket = Socket::CreateSocket(uavNode, tid);
                        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 9);
                        criticalSocket->Bind(local);
                        
                        InetSocketAddress remote = InetSocketAddress(gcsAddress, 9);
                        criticalSocket->Connect(remote);

                        Ptr<UavTelemetry> telemetryApp = CreateObject<UavTelemetry>();
                        telemetryApp->SetInterval(Seconds(0.1));
                        telemetryApp->SetPacketSize(150);
                        telemetryApp->SetSocket(criticalSocket);

                        numApplication = uavNode->GetNApplications();
                        m_telemetryAppList.insert({1, numApplication});
                        uavNode->AddApplication(telemetryApp);

                        telemetryApp->SetStartTime(Seconds(0.0));
                        telemetryApp->SetStopTime(Time::Max());

                        NS_LOG_INFO("Started Telemetry app for uav1, app index" << numApplication);
                    } else if (app_type == "VideoStream") {
                        VideoStreamServerHelper videoServer (5000);
                        videoServer.SetAttribute ("MaxPacketSize", UintegerValue (1400));
                        numApplication = uavNode->GetNApplications();
                        m_videoServerAppList.insert({1, numApplication});
                        ApplicationContainer serverApp = videoServer.Install (uavNode);
                        serverApp.Start (Seconds (0.0));
                        serverApp.Stop (Time::Max());

                        NS_LOG_INFO("Started Video server app for uav1, app index" << numApplication);

                        VideoStreamClientHelper videoClient(uavAddress, 5000);
                        numApplication = gcsNode->GetNApplications();
                        m_gcsVideoClientAppList.insert({1, numApplication});
                        ApplicationContainer clientApp = videoClient.Install(gcsNode);
                        clientApp.Start (Seconds(0.0));
                        clientApp.Stop (Time::Max());

                        NS_LOG_INFO("Started Video client app for gcs, app index" << numApplication);
                    } else if (app_type == "ControlCommands") {
                        for (uint32_t i = 0; i <= gcsNode->GetNApplications() - 1; i++) {
                            Ptr<Application> app = gcsNode->GetApplication(i);
                            Ptr<UavCommand> commandApp = DynamicCast<UavCommand>(app);
                            if (commandApp) {
                                commandApp->SendCommand(PRIO_CRITICAL);
                                NS_LOG_INFO("Sent command with prio " << PRIO_CRITICAL);
                            }
                        }
                    }
                    NS_LOG_INFO("Received start event for " << app_type << " with config: " << config << " (Local ID: " << local_id << ")");
                } else if (command == "stop") {
                    NS_LOG_INFO("Received stop event for " << app_type);

                    if (app_type == "Telemetry") {
                        uint32_t appIndex;
                        for (const auto &entry : m_telemetryAppList) {{
                            if (entry.first == 1) {
                                appIndex = entry.second;
                                Ptr<Application> app = uavNode->GetApplication(appIndex);
                                Ptr<UavTelemetry> telemetryApp = DynamicCast<UavTelemetry>(app);
                                if (telemetryApp) {
                                    telemetryApp->StopApplication();
                                }

                                NS_LOG_INFO("Stop telemetry app for uav 1 at app index " << appIndex);
                            }
                        }}
                    } else if (app_type == "VideoStream") {
                        uint32_t appIndex;
                        for (const auto &entry : m_videoServerAppList) {{
                            if (entry.first == 1) {
                                appIndex = entry.second;
                                Ptr<Application> app = uavNode->GetApplication(appIndex);
                                Ptr<VideoStreamServer> videoServerApp = DynamicCast<VideoStreamServer>(app);
                                if (videoServerApp) {
                                    videoServerApp->StopApplication();
                                }

                                NS_LOG_INFO("Stop video server app for uav 1 at app index " << appIndex);
                            }
                        }}
                        for (const auto &entry : m_gcsVideoClientAppList) {{
                            if (entry.first == 1) {
                                appIndex = entry.second;
                                Ptr<Application> app = gcsNode->GetApplication(appIndex);
                                Ptr<VideoStreamClient> videoClientApp = DynamicCast<VideoStreamClient>(app);
                                if (videoClientApp) {
                                    videoClientApp->StopApplication();
                                }

                                NS_LOG_INFO("Stop video client app for gcs of uav 1 at app index " << appIndex);
                            }
                        }}
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

// AppType getAppTypeFromString(const std::string& appTypeStr) {
//     static const std::unordered_map<std::string, AppType> appTypeMap = {
//         {"Telemetry", Telemetry},
//         {"VideoStream", VideoStream},
//         {"ControlCommands", ControlCommands},
//         {"SensorData", SensorData}
//     };

//     auto it = appTypeMap.find(appTypeStr);
//     if (it != appTypeMap.end()) {
//         return it->second;
//     }

//     throw std::invalid_argument("Invalid AppType string: " + appTypeStr);
// }