#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "uav/uav-application.h"
#include "uav/uav-telemetry.h"
#include "uav/uav-video-client.h"
#include "uav/uav-video-server.h"
#include "priority/priority-tx-queue.h"
#include "uav/uav-qos-config.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UavPriorityQueueTest");

// Global variables to track statistics
uint32_t totalPacketsSent = 0;
uint32_t totalPacketsReceived = 0;
uint32_t highPriorityPacketsSent = 0;
uint32_t highPriorityPacketsReceived = 0;
uint32_t normalPriorityPacketsSent = 0;
uint32_t normalPriorityPacketsReceived = 0;

void ReceivePacket(Ptr<const Packet> p, const Address& addr) {
    totalPacketsReceived++;
    PriorityTag priorityTag;
    bool found = p->PeekPacketTag(priorityTag);
    uint8_t priority = found ? priorityTag.GetPriority() : 2;
    if (priority == PRIO_CRITICAL) {
        highPriorityPacketsReceived++;
        NS_LOG_INFO("Received CRITICAL priority packet at " << Simulator::Now().GetSeconds() << "s");
    } else {
        normalPriorityPacketsReceived++;
    }
}

void SendCriticalPacket(Ptr<Socket> socket) {
    Ptr<Packet> packet = Create<Packet>(100); // 100 byte critical packet
    PriorityTag priorityTag(PRIO_CRITICAL);
    packet->AddPacketTag(priorityTag);
    socket->Send(packet);
    highPriorityPacketsSent++;
    totalPacketsSent++;
    NS_LOG_INFO("Sent CRITICAL priority packet at " << Simulator::Now().GetSeconds() << "s");
    
    // Schedule next critical packet (if needed)
    Simulator::Schedule(Seconds(0.5), &SendCriticalPacket, socket);
}

int main(int argc, char *argv[]) {
    // Enable logging
    LogComponentEnable("UavPriorityQueueTest", LOG_LEVEL_INFO);
    LogComponentEnable("PriorityTxQueue", LOG_LEVEL_ALL);
    LogComponentEnable("UavTelemetry", LOG_LEVEL_INFO);
    LogComponentEnable("UavVideoClient", LOG_LEVEL_ALL);
    LogComponentEnable("UavVideoServer", LOG_LEVEL_ALL);
    LogComponentEnable("VideoStreamClientApplication", LOG_LEVEL_ALL);
    LogComponentEnable("VideoStreamServerApplication", LOG_LEVEL_ALL);

    // Create nodes
    NodeContainer nodes;
    nodes.Create(2); // UAV (node 0) and Ground Station (node 1)

    // Set up mobility (simple constant position)
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Create point-to-point link with limited bandwidth to create congestion
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    // Install internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Create and configure QoS
    Ptr<UavQosConfig> qosConfig = CreateObject<UavQosConfig>();
    qosConfig->SetOperationMode(UavQosConfig::HIGH_QUALITY_VIDEO);

    // Ptr<QosConfig> qosConfig = CreateObject<QosConfig>();
    // qosConfig->SetQueueWeights({50, 30, 15, 5});

    // Create and configure priority queue
    Ptr<PriorityTxQueue> priorityQueue = CreateObject<PriorityTxQueue>();
    priorityQueue->SetQosConfig(qosConfig);
    priorityQueue->SetCycleBudget(12500000);
    
    // Replace the default queue with our priority queue
    Ptr<PointToPointNetDevice> p2pDevice = DynamicCast<PointToPointNetDevice>(devices.Get(1));
    p2pDevice->SetQueue(priorityQueue);

    // // Create UDP sockets for critical communication test
    // TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    // Ptr<Socket> criticalSocket = Socket::CreateSocket(nodes.Get(0), tid);
    // InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 9);
    // criticalSocket->Bind(local);
    //InetSocketAddress remote = InetSocketAddress(interfaces.GetAddress(1), 10);
    // criticalSocket->Connect(remote);

    // // Setup packet sink on ground station to receive critical packets
    // PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 10));
    // ApplicationContainer sinkApp = sink.Install(nodes.Get(1));
    // sinkApp.Start(Seconds(0.0));
    // sinkApp.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&ReceivePacket));

    // // Install UAV telemetry application
    // Ptr<UavTelemetry> telemetryApp = CreateObject<UavTelemetry>();
    // telemetryApp->SetInterval(Seconds(0.1)); // Fast telemetry updates to create congestion
    // telemetryApp->SetPacketSize(200); // Larger packets to create congestion faster
    // telemetryApp->SetSocket(criticalSocket);
    // nodes.Get(0)->AddApplication(telemetryApp);
    // telemetryApp->SetStartTime(Seconds(1.0));
    // telemetryApp->SetStopTime(Seconds(35.0));

    // Install video streaming server on UAV
    // Ptr<UavVideoServer> videoServer = CreateObject<UavVideoServer>();
    // videoServer->SetAttribute("Interval", TimeValue(Seconds(0.04)));
    // videoServer->SetAttribute("Port", UintegerValue (5000));
    // videoServer->SetAttribute("VideoLength", UintegerValue (30));
    // videoServer->SetAttribute ("MaxPacketSize", UintegerValue (1400));
    // nodes.Get(0)->AddApplication(videoServer);
    // videoServer->SetStartTime(Seconds(0.0));
    // videoServer->SetStopTime(Seconds(35.0));

    // // Install video streaming client on ground station
    // Ptr<UavVideoClient> videoClient = CreateObject<UavVideoClient>();
    // videoClient->SetRemote(interfaces.GetAddress(0), 5000);
    // nodes.Get(1)->AddApplication(videoClient);
    // videoClient->SetStartTime(Seconds(0.5));
    // videoClient->SetStopTime(Seconds(35.0));

    // // Schedule critical packets to be sent during congestion
    // //Simulator::Schedule(Seconds(3.0), &SendCriticalPacket, criticalSocket);
    // pointToPoint.EnablePcap ("new", devices.Get (1), false);

    VideoStreamClientHelper videoClient (interfaces.GetAddress (0), 5000);
    ApplicationContainer clientApp = videoClient.Install (nodes.Get (1));
    clientApp.Start (Seconds (0.5));
    clientApp.Stop (Seconds (30.0));

    VideoStreamServerHelper videoServer (5000);
    videoServer.SetAttribute ("MaxPacketSize", UintegerValue (1400));
    //videoServer.SetAttribute ("FrameFile", StringValue ("./scratch/videoStreamer/frameList.txt"));
    // videoServer.SetAttribute ("FrameSize", UintegerValue (4096));

    ApplicationContainer serverApp = videoServer.Install (nodes.Get (0));
    serverApp.Start (Seconds (0.0));
    serverApp.Stop (Seconds (30.0));

    pointToPoint.EnablePcap ("videoStream", devices.Get (1), false);

    // Run simulation
    Simulator::Stop(Seconds(40.0));
    Simulator::Run();

    // Print statistics
    // std::cout << "\nSimulation Results:\n";
    // std::cout << "Total packets sent: " << totalPacketsSent << "\n";
    // std::cout << "Total packets received: " << totalPacketsReceived << "\n";
    // std::cout << "High priority packets sent: " << highPriorityPacketsSent << "\n";
    // std::cout << "High priority packets received: " << highPriorityPacketsReceived << "\n";
    // std::cout << "Normal priority packets sent: " << normalPriorityPacketsSent << "\n";
    // std::cout << "Normal priority packets received: " << normalPriorityPacketsReceived << "\n";
    // std::cout << "High priority delivery ratio: " 
    //           << (highPriorityPacketsReceived * 100.0 / highPriorityPacketsSent) << "%\n";
    // std::cout << "Normal priority delivery ratio: " 
    //           << (normalPriorityPacketsReceived * 100.0 / normalPriorityPacketsSent) << "%\n";

    Simulator::Destroy();
    return 0;
}