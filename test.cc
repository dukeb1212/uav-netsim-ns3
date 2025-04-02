#include "ns3/athstats-helper.h"
#include "ns3/boolean.h"
#include "ns3/core-module.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/ssid.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"
#include <fstream>
#include <iomanip>
#include <numeric>
#include "ns3/flow-monitor-helper.h"
#include "ns3/onoff-application.h"
#include "ns3/packet-sink.h"
#include "ns3/realtime-simulator-impl.h"
#include "zmq.hpp"
#include <nlohmann/json.hpp>
#include "zmq_receiver_app.h"
#include <sstream>
#include <iostream>
#include "uav/uav-telemetry.h"
#include "uav/uav-command.h"
#include "ns3/node-list.h"

using json = nlohmann::json;
using namespace ns3;

zmq::context_t zmqContext(1);
zmq::socket_t zmqSocket(zmqContext, ZMQ_PUB);
std::string zmqAddress = "tcp://192.168.1.30:5555";

std::map<uint32_t, Time> lastActiveTime;  // Track last active time of flows
double inactivityThreshold = 5.0; // Threshold in seconds

// Enhanced event tracking structures
struct NodeState {
    WifiMode lastMode;
    double lastSnr;
    std::deque<double> snrWindow;
    uint32_t consecutiveErrors;
};

json metrics;

std::map<std::string, NodeState> nodeStates;
std::ofstream g_outputFile;

// Channel utilization tracking
std::map<uint64_t, Time> packetTxTimestamps;  // Maps packet UID -> Tx Time
Time totalTxTime = Seconds(0);
Time totalRxTime = Seconds(0);
Time totalCcaTime = Seconds(0);
Time lastUpdateTime = Seconds(0);
uint32_t maxConsecutiveErrors = 0;
double totalSnr = 0;
uint32_t snrCount = 0;

std::string Ipv4ToString(const Ipv4Address &address) {
    uint32_t ip = address.Get();

    uint8_t byte1 = (ip >> 24) & 0xFF;
    uint8_t byte2 = (ip >> 16) & 0xFF;
    uint8_t byte3 = (ip >> 8) & 0xFF;
    uint8_t byte4 = ip & 0xFF;

    std::ostringstream oss;
    oss << static_cast<int>(byte1) << "."
        << static_cast<int>(byte2) << "."
        << static_cast<int>(byte3) << "."
        << static_cast<int>(byte4);

    return oss.str();
}

void PublishZMQMessage(zmq::socket_t* socket, const std::string& topic, const json& message) {
    // // Send topic as first frame
    // zmq::message_t topicMsg(topic.begin(), topic.end());
    // socket.send(topicMsg, zmq::send_flags::sndmore);

    // // Convert JSON to string
    // std::string messageStr = message.dump();

    // // Send JSON message as second frame
    // zmq::message_t jsonMsg(messageStr.begin(), messageStr.end());
    // socket.send(jsonMsg, zmq::send_flags::none);
    std::string messageStr = message.dump();
    socket->send(zmq::buffer(topic + " " + messageStr), zmq::send_flags::none);
}


void ReportRealTimeMetrics(Ptr<FlowMonitor> monitor, Ptr<Ipv4FlowClassifier> classifier, double interval) {
    double now = Simulator::Now().GetSeconds();
    metrics["conditions"]["time"] = now;

    monitor->CheckForLostPackets();
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    json flowStats;
    for (auto const& flow : stats) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        std::string flowDirection = std::to_string(flow.first);
        if (flow.second.rxPackets <= 1) {
            flowStats[flowDirection] = {
                {"src", Ipv4ToString(t.sourceAddress)},
                {"dst", Ipv4ToString(t.destinationAddress)},
                {"txBytes", flow.second.txBytes},
                {"rxBytes", flow.second.rxBytes},
                {"txPackets", flow.second.txPackets},
                {"rxPackets", flow.second.rxPackets},
                {"meanDelay", flow.second.delaySum.GetMicroSeconds() / 1},
                {"meanJitter", flow.second.jitterSum.GetMicroSeconds() / 1},
                {"packetLossL3", flow.second.lostPackets}
            };

            g_outputFile << "FLOW_STATS, "
                     << "id=" << flow.first << ", "
                     << "time=" << now << ", "
                     << "src=" << t.sourceAddress << ":" << t.sourcePort << ", "
                     << "dst=" << t.destinationAddress << ":" << t.destinationPort << "," 
                     << "txBytes=" << flow.second.txBytes << ", "
                     << "rxBytes=" << flow.second.rxBytes << ", "
                     << "txPackets=" << flow.second.txPackets << ", "
                     << "rxPackets=" << flow.second.rxPackets << ", "
                     << "meanDelay=" << flow.second.delaySum.GetMicroSeconds() / 1 << "us, "
                     << "meanJitter=" << flow.second.jitterSum.GetMicroSeconds() / 1 << "us, "
                     << "packetLoss=" << flow.second.lostPackets << ", ";
        } else {
            flowStats[flowDirection] = {
                {"src", Ipv4ToString(t.sourceAddress)},
                {"dst", Ipv4ToString(t.destinationAddress)},
                {"txBytes", flow.second.txBytes},
                {"rxBytes", flow.second.rxBytes},
                {"txPackets", flow.second.txPackets},
                {"rxPackets", flow.second.rxPackets},
                {"meanDelay", flow.second.delaySum.GetMicroSeconds() / flow.second.rxPackets},
                {"meanJitter", flow.second.jitterSum.GetMicroSeconds() / (flow.second.rxPackets - 1)},
                {"packetLossL3", flow.second.lostPackets}
            };

            g_outputFile << "FLOW_STATS, "
                     << "id=" << flow.first << ", "
                     << "time=" << now << ", "
                     << "src=" << t.sourceAddress << ":" << t.sourcePort << ", "
                     << "dst=" << t.destinationAddress << ":" << t.destinationPort << "," 
                     << "txBytes=" << flow.second.txBytes << ", "
                     << "rxBytes=" << flow.second.rxBytes << ", "
                     << "txPackets=" << flow.second.txPackets << ", "
                     << "rxPackets=" << flow.second.rxPackets << ", "
                     << "meanDelay=" << flow.second.delaySum.GetMicroSeconds() / flow.second.rxPackets << "us, "
                     << "meanJitter=" << flow.second.jitterSum.GetMicroSeconds() / (flow.second.rxPackets - 1) << "us, "
                     << "packetLoss=" << flow.second.lostPackets << ", ";
        }
    }
    
    metrics["conditions"]["flows"] = flowStats;

    // Publish metrics
    PublishZMQMessage(&zmqSocket, "network", metrics);

    // Schedule next report
    Simulator::Schedule(Seconds(interval), &ReportRealTimeMetrics, monitor, classifier, interval);
}

uint32_t ExtractNodeId(const std::string& context) {
    size_t start = context.find("/NodeList/") + 10;  // Move past "/NodeList/"
    size_t end = context.find("/", start);
    uint32_t nodeId = std::stoi(context.substr(start, end - start));
    return nodeId;
}

void LogChannelUtilization() {
    Time now = Simulator::Now();
    Time interval = now - lastUpdateTime;
    double utilization = ((totalTxTime + totalRxTime + totalCcaTime) / interval).GetDouble();
    
    g_outputFile << now.GetSeconds() << ",CHANNEL_UTIL"
                 << ",util=" << utilization * 100 << "%"
                 << ",tx_ratio=" << (totalTxTime/interval)*100 << "%"
                 << ",rx_ratio=" << (totalRxTime/interval)*100 << "%\n";
    
    // Reset counters
    totalTxTime = totalRxTime = totalCcaTime = Seconds(0);
    lastUpdateTime = now;
    Simulator::Schedule(Seconds(1), &LogChannelUtilization);
}

void TrackModeChange(std::string context, WifiMode currentMode) {
    NodeState& state = nodeStates[context];
    if (state.lastMode != currentMode && !state.lastMode.IsMandatory()) {
        // metrics["events"].push_back({
        //     {"time", Simulator::Now().GetSeconds()},
        //     {"type", "MODE_CHANGE"},
        //     {"node", ExtractNodeId(context)},
        //     {"from", state.lastMode.GetUniqueName()},
        //     {"to", currentMode.GetUniqueName()}
        // });
        // metrics["conditions"].clear();
        // PublishZMQMessage(&zmqSocket, "network", metrics);
        // metrics["events"].clear();
        g_outputFile << Simulator::Now().GetSeconds() << ",MODE_CHANGE"
                     << ",from=" << state.lastMode.GetUniqueName()
                     << ",to=" << currentMode.GetUniqueName() << "\n";
    }
    state.lastMode = currentMode;
}


void TrackSnrVariation(std::string context, double snr) {
    NodeState& state = nodeStates[context];
    state.snrWindow.push_back(snr);
    if (state.snrWindow.size() > 5) state.snrWindow.pop_front();

    // Store total SNR and count
    totalSnr += snr;
    snrCount++;

    // Check SNR stability
    if (state.snrWindow.size() == 5) {
        double avg = std::accumulate(state.snrWindow.begin(), state.snrWindow.end(), 0.0) / 5;
        if (fabs(snr - avg) > 5.0) {
            g_outputFile << Simulator::Now().GetSeconds() << ",SNR_CHANGE"
                         << ",current=" << snr
                         << ",avg=" << avg
                         << ",delta=" << (snr - avg) << "\n";
        }
    }
    state.lastSnr = snr;
}


void PhyTxTrace(std::string context, Ptr<const Packet> packet, WifiMode mode, WifiPreamble preamble, uint8_t txPower) {
    TrackModeChange(context, mode);
    uint64_t packetId = packet->GetUid();

    packetTxTimestamps[packetId] = Simulator::Now(); // Store expected reception time

    g_outputFile << Simulator::Now().GetSeconds() << ",PHY_TX"
                 << ",node=" << ExtractNodeId(context)
                 << ",mode=" << mode.GetUniqueName()
                 << ",size=" << packet->GetSize()
                 << ",packetId=" << packetId << "\n";
}


void PhyRxOkTrace(std::string context, Ptr<const Packet> packet, double snr, WifiMode mode, WifiPreamble preamble) 
{
    uint64_t packetId = packet->GetUid();

    if (packetTxTimestamps.find(packetId) != packetTxTimestamps.end()) {
        Time txTime = packetTxTimestamps[packetId];
        Time rxTime = Simulator::Now();
        Time latency = rxTime - txTime;

        g_outputFile << rxTime.GetSeconds() << ",DATA_RX"
                     << ",node=" << ExtractNodeId(context)
                     << ",latency=" << latency.GetMicroSeconds() << "us"
                     << ",snr=" << snr
                     << ",packetId=" << packetId << "\n";
    } else {
        g_outputFile << Simulator::Now().GetSeconds() << ",DATA_RX"
                     << ",latency=UNKNOWN,packetId=" << packetId << "\n";
    }
    TrackSnrVariation(context, snr);
}




void PhyRxErrorTrace(std::string context, Ptr<const Packet> packet, double snr) {
    NodeState& state = nodeStates[context];
    if (packet->GetSize() > 100) { // Data packets only
        state.consecutiveErrors++;
        maxConsecutiveErrors = std::max(maxConsecutiveErrors, state.consecutiveErrors);

        g_outputFile << Simulator::Now().GetSeconds() << ",DATA_ERROR"
                     << ",consecutive=" << state.consecutiveErrors
                     << ",snr=" << snr << "\n";
    }
    TrackSnrVariation(context, snr);
}


void PhyStateTrace(std::string context, Time start, Time duration, WifiPhyState state) {
    Time now = Simulator::Now();
    if (now > lastUpdateTime) {
        Time activeTime = std::min(now, start + duration) - std::max(lastUpdateTime, start);
        switch(state) {
            case WifiPhyState::TX: totalTxTime += activeTime; break;
            case WifiPhyState::RX: totalRxTime += activeTime; break;
            case WifiPhyState::CCA_BUSY: totalCcaTime += activeTime; break;
            default: break;
        }
    }
}

void CourseChangeCallback(std::string context, Ptr<const MobilityModel> model) {
    Vector pos = model->GetPosition();
    g_outputFile << Simulator::Now().GetSeconds() << ",POSITION"
                 << ",x=" << pos.x << ",y=" << pos.y << ",z=" << pos.z << "\n";
}


static void AdvanceNodePosition(Ptr<Node> node) {
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    Vector pos = mobility->GetPosition();
    pos.x += 5.0;
    mobility->SetPosition(pos);
    Simulator::Schedule(Seconds(1), &AdvanceNodePosition, node);
}

int main(int argc, char *argv[]) {
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // Initialize ZMQ publisher
    
    zmqSocket.bind(zmqAddress);

    g_outputFile.open("network_events.csv");
    g_outputFile << "timestamp,event,details\n";

    GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::RealtimeSimulatorImpl"));

    LogComponentEnable("ZmqReceiverApp", LOG_LEVEL_INFO);
    // LogComponentEnable("UavTelemetry", LOG_LEVEL_INFO);
    // LogComponentEnable ("VideoStreamClientApplication", LOG_LEVEL_INFO);
    // LogComponentEnable ("VideoStreamServerApplication", LOG_LEVEL_INFO);

    // Create network nodes
    NodeContainer nodes;
    nodes.Create(2); // Node 1,2: Mobile STA, Node 0: AP

    // Configure wireless channel
    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::RandomPropagationDelayModel",
                                "Variable", StringValue("ns3::UniformRandomVariable[Min=0.000001|Max=0.000005]"));
    channel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                                "Exponent", DoubleValue(3.0),
                                "ReferenceLoss", DoubleValue(46.6777),
                                "ReferenceDistance", DoubleValue(1.0));

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());
    phy.Set("TxPowerStart", DoubleValue(20.0));
    phy.Set("TxPowerEnd", DoubleValue(20.0));
    phy.Set("RxNoiseFigure", DoubleValue(7.0));

    // Configure WiFi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    WifiMacHelper mac;
    Ssid ssid = Ssid("indoor-uav");

    // Setup AP
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevice = wifi.Install(phy, mac, nodes.Get(0));

    // Setup STA
    mac.SetType("ns3::StaWifiMac",
               "Ssid", SsidValue(ssid),
               "ActiveProbing", BooleanValue(true));
    NetDeviceContainer staDevice = wifi.Install(phy, mac, nodes.Get(1));

    // Configure mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);
    
    // mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    // mobility.Install(nodes.Get(0)); // Mobile STA
    
    Ptr<ZmqReceiverApp> app = CreateObject<ZmqReceiverApp>();
    nodes.Get(0)->AddApplication(app);
    app->SetAttribute("Address", StringValue("192.168.1.12"));
    app->SetAttribute("Port", UintegerValue(5555));
    app->SetAttribute("ID", StringValue("network_events"));
    app->SetStartTime(Seconds(1));
    app->SetStopTime(Seconds(300.0));

    InternetStackHelper internet;
    internet.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer gcsInterface = ipv4.Assign(apDevice);
    Ipv4InterfaceContainer uavInterfaces = ipv4.Assign(staDevice);

    // ==================== Flow Monitor Setup ====================
    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> flowMonitor = flowmonHelper.InstallAll();

    // ==================== Application Setup ====================
    // UDP traffic configuration
    uint16_t gcsPort = 9;
    Address gcsSinkAddress(InetSocketAddress(gcsInterface.GetAddress(0), gcsPort));
    PacketSinkHelper gcsPacketSinkHelper("ns3::UdpSocketFactory", gcsSinkAddress);

    uint16_t uavPort = 99;
    Address uavSinkAddress(InetSocketAddress(uavInterfaces.GetAddress(0), uavPort));
    PacketSinkHelper uavPacketSinkHelper("ns3::UdpSocketFactory", uavSinkAddress);
    
    // Install sink on AP
    ApplicationContainer gcsSinkApp = gcsPacketSinkHelper.Install(nodes.Get(0));
    gcsSinkApp.Start(Seconds(0.0));
    gcsSinkApp.Stop(Seconds(300.0));

    ApplicationContainer uavSinkApp = uavPacketSinkHelper.Install(nodes.Get(1));
    uavSinkApp.Start(Seconds(0.0));
    uavSinkApp.Stop(Seconds(300.0));

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> gcsSocket = Socket::CreateSocket(nodes.Get(0), tid);
    InetSocketAddress remote = InetSocketAddress(uavInterfaces.GetAddress(0), uavPort);
    gcsSocket->Connect(remote);

    Ptr<UavCommand> commandApp = CreateObject<UavCommand>();
    commandApp->SetSocket(gcsSocket);
    nodes.Get(0)->AddApplication(commandApp);

    commandApp->SetStartTime(Seconds(0.5));
    commandApp->SetStopTime(Seconds(300.0));


    // Ptr<OnOffApplication> onoffApp = CreateObject<OnOffApplication>();
    // nodes.Get(1)->AddApplication(onoffApp);

    // // Set the remote address (where to send packets)
    // onoffApp->SetAttribute("Remote", AddressValue(sinkAddress));
    // onoffApp->SetAttribute("DataRate", DataRateValue(DataRate("500kbps")));
    // onoffApp->SetAttribute("PacketSize", UintegerValue(512));

    // // Set start and stop times
    // onoffApp->SetStartTime(Seconds(1.0));
    // onoffApp->SetStopTime(Seconds(299.0));
    
    // Periodic channel utilization logging
    //Simulator::Schedule(Seconds(1.0), &LogChannelUtilization);

    // ==================== Event Tracing ====================
    // Config::Connect("/NodeList/*/DeviceList/*/Phy/State/State", MakeCallback(&PhyStateTrace));
    // Config::Connect("/NodeList/*/DeviceList/*/Phy/State/Tx", MakeCallback(&PhyTxTrace));
    // Config::Connect("/NodeList/*/DeviceList/*/Phy/State/RxOk", MakeCallback(&PhyRxOkTrace));
    // Config::Connect("/NodeList/*/DeviceList/*/Phy/State/RxError", MakeCallback(&PhyRxErrorTrace));
    // Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeCallback(&CourseChangeCallback));

    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());

    Simulator::Schedule(Seconds(1.0), &ReportRealTimeMetrics, flowMonitor, classifier, 1.0);
    
    Simulator::Stop(Seconds(300.01));
    Simulator::Run();

    g_outputFile.close();

    Simulator::Destroy();
    return 0;
}