#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mobility-module.h"
#include "ns3/realtime-simulator-impl.h"
#include "zmq_receiver_app.h"

using namespace ns3;

// Modified CheckMetrics with full metrics
void CheckMetrics(Ptr<FlowMonitor> monitor, Ptr<Ipv4FlowClassifier> classifier, double interval) {
    static std::map<FlowId, uint64_t> prevTxPackets, prevRxPackets, prevRxBytes, prevLostPackets;
    static std::map<FlowId, Time> prevDelaySum, prevJitterSum;

    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    
    for (auto& flow : stats) {
        FlowId flowId = flow.first;
        FlowMonitor::FlowStats& stat = flow.second;

        // Get flow classification details
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flowId);

        // Ensure previous values exist (initialize if needed)
        if (prevTxPackets.find(flowId) == prevTxPackets.end()) {
            prevTxPackets[flowId] = stat.txPackets;
            prevRxPackets[flowId] = stat.rxPackets;
            prevRxBytes[flowId] = stat.rxBytes;
            prevLostPackets[flowId] = stat.lostPackets;
            prevDelaySum[flowId] = stat.delaySum;
            prevJitterSum[flowId] = stat.jitterSum;
            continue;  // Skip first iteration to avoid inaccurate metrics
        }

        // Calculate deltas since last interval
        uint64_t tx = stat.txPackets - prevTxPackets[flowId];
        uint64_t rx = stat.rxPackets - prevRxPackets[flowId];
        uint64_t lost = stat.lostPackets - prevLostPackets[flowId];

        // Handle potential negative values
        Time delay = (stat.delaySum >= prevDelaySum[flowId]) ? (stat.delaySum - prevDelaySum[flowId]) : Time(0);
        Time jitter = (stat.jitterSum >= prevJitterSum[flowId]) ? (stat.jitterSum - prevJitterSum[flowId]) : Time(0);

        // Calculate metrics
        double lossRatio = (tx > 0) ? (lost * 100.0) / tx : 0.0;
        double throughput = (stat.rxBytes - prevRxBytes[flowId]) * 8.0 / interval / 1e6;
        double avgDelay = (rx > 0) ? delay.GetSeconds() / rx : 0.0;
        double avgJitter = (rx > 1) ? jitter.GetSeconds() / std::max(rx - 1, uint64_t(1)) : 0.0;

        // Update previous values
        prevTxPackets[flowId] = stat.txPackets;
        prevRxPackets[flowId] = stat.rxPackets;
        prevRxBytes[flowId] = stat.rxBytes;
        prevLostPackets[flowId] = stat.lostPackets;
        prevDelaySum[flowId] = stat.delaySum;
        prevJitterSum[flowId] = stat.jitterSum;

        // Output metrics with flow details
        std::cout << "At " << Simulator::Now().GetSeconds() << "s: "
                  << "Flow " << flowId << " (" 
                  << t.sourceAddress << ":" << t.sourcePort << " -> "
                  << t.destinationAddress << ":" << t.destinationPort << ") | "
                  << "Throughput: " << throughput << " Mbps | "
                  << "Avg Delay: " << avgDelay << "s | "
                  << "Avg Jitter: " << avgJitter << "s | "
                  << "Loss: " << lossRatio << "%" << std::endl;
    }

    // Schedule next check
    Simulator::Schedule(Seconds(interval), &CheckMetrics, monitor, classifier, interval);
}


void PrintNodePositions(NodeContainer nodes, double interval) {
    for (uint32_t i = 0; i < nodes.GetN(); i++) {
        Ptr<Node> node = nodes.Get(i);
        Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
        Vector position = mobility->GetPosition();

        std::cout << "At " << Simulator::Now().GetSeconds() << "s: "
                  << "Node " << i << " is at (" 
                  << position.x << ", " 
                  << position.y << ", " 
                  << position.z << ")" << std::endl;
    }

    Simulator::Schedule(Seconds(interval), &PrintNodePositions, nodes, interval);
}

int main(int argc, char* argv[]) {
    GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::RealtimeSimulatorImpl"));
    LogComponentEnable("ZmqReceiverApp", LOG_LEVEL_INFO);

    // Create network nodes
    NodeContainer nodes;
    nodes.Create(2);

    // Configure WiFi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode", StringValue("HtMcs0"),
                               "ControlMode", StringValue("HtMcs0"));

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel");

    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

    // Install internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Configure applications
    OnOffHelper onoff("ns3::UdpSocketFactory", Address(InetSocketAddress(interfaces.GetAddress(0), 10)));
    onoff.SetConstantRate(DataRate("1Mbps"));
    onoff.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer apps = onoff.Install(nodes.Get(1));
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(300.0));  // Reduced from 1 year to 5 minutes for testing

    PacketSinkHelper sink("ns3::UdpSocketFactory", Address(InetSocketAddress(interfaces.GetAddress(0), 10)));
    apps = sink.Install(nodes.Get(0));
    apps.Start(Seconds(0.0));

    // ZMQ receiver configuration
    Ptr<ZmqReceiverApp> app = CreateObject<ZmqReceiverApp>();
    nodes.Get(0)->AddApplication(app);
    app->SetAttribute("Address", StringValue("192.168.1.4"));
    app->SetAttribute("Port", UintegerValue(5555));
    app->SetAttribute("ID", StringValue("ns3"));
    app->SetStartTime(Seconds(1));
    app->SetStopTime(Seconds(300.0));

    // Configure mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Configure Flow Monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());

    // Schedule metrics collection
    double interval = 0.1;
    Simulator::Schedule(Seconds(interval), &CheckMetrics, monitor, classifier, interval);
    //Simulator::Schedule(Seconds(interval), &PrintNodePositions, nodes, interval);

    // Set simulation duration and run
    Simulator::Stop(Seconds(300.0));
    Simulator::Run();

    // Generate final XML report
    monitor->SerializeToXmlFile("flow-metrics.xml", true, true);
    Simulator::Destroy();

    return 0;
}