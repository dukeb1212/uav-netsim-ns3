#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MultiChannelWifiLogging");

void PacketReceivedCallback(Ptr<const Packet> packet)
{
    NS_LOG_INFO("Packet received: Size = " << packet->GetSize() << " Bytes");
}

void InstallWifiNetwork(NodeContainer &apNode, NodeContainer &staNode, NetDeviceContainer &devices, 
                        double frequency, std::string ssid, AcIndex ac)
{
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);

    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    auto lossModel = CreateObject<FriisPropagationLossModel>();
    spectrumChannel->AddPropagationLossModel(lossModel);
    auto delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    SpectrumWifiPhyHelper phy;
    phy.SetChannel(spectrumChannel);
    
    if (frequency == 2.4)
        phy.Set("ChannelSettings", StringValue("{1, 20, BAND_2_4GHZ, 0}"));
    else
        phy.Set("ChannelSettings", StringValue("{0, 20, BAND_5GHZ, 0}"));

    WifiMacHelper mac;
    Ssid networkSsid = Ssid(ssid);

    // Install AP
    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(networkSsid),
                "QosSupported", BooleanValue(true));
    NetDeviceContainer apDevice = wifi.Install(phy, mac, apNode);

    // Install STA
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(networkSsid),
                "ActiveProbing", BooleanValue(false),
                "QosSupported", BooleanValue(true));
    NetDeviceContainer staDevice = wifi.Install(phy, mac, staNode);

    devices.Add(apDevice);
    devices.Add(staDevice);

    // Configure EDCA parameters for both devices
    for (uint32_t i = 0; i < devices.GetN(); ++i) {
        Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(devices.Get(i));
        Ptr<QosTxop> qosTxop = wifiDev->GetMac()->GetQosTxop(ac);

        if (ac == AC_VO) {
            qosTxop->SetAifsn(2);
            qosTxop->SetMinCw(3);
            qosTxop->SetMaxCw(7);
        }
        else if (ac == AC_BK) {
            qosTxop->SetAifsn(7);
            qosTxop->SetMinCw(15);
            qosTxop->SetMaxCw(1023);
        }
    }
}

int main(int argc, char *argv[])
{
    LogComponentEnable("MultiChannelWifiLogging", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // Create nodes with clear AP/STA separation
    NodeContainer apNode24, staNode24;
    NodeContainer apNode5, staNode5;
    apNode24.Create(1);
    staNode24.Create(1);
    apNode5.Create(1);
    staNode5.Create(1);

    // Add mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(apNode24);
    mobility.Install(staNode24);
    mobility.Install(apNode5);
    mobility.Install(staNode5);

    InternetStackHelper internet;
    internet.Install(apNode24);
    internet.Install(staNode24);
    internet.Install(apNode5);
    internet.Install(staNode5);

    NetDeviceContainer devices24, devices5;

    // Install 2.4 GHz network (Control - AC_VO)
    InstallWifiNetwork(apNode24, staNode24, devices24, 2.4, "UAV_Control", AC_VO);
    
    // Install 5 GHz network (Data - AC_BK)
    InstallWifiNetwork(apNode5, staNode5, devices5, 5.0, "UAV_Data", AC_BK);

    // IP addressing
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces24 = ipv4.Assign(devices24);

    ipv4.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces5 = ipv4.Assign(devices5);

    // Applications setup
    uint16_t port = 9;

    // Control traffic (High Priority)
    UdpEchoServerHelper echoServer(port);
    ApplicationContainer serverApp = echoServer.Install(apNode24.Get(0));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    UdpEchoClientHelper echoClient(interfaces24.GetAddress(0), port);
    echoClient.SetAttribute("MaxPackets", UintegerValue(500));
    echoClient.SetAttribute("Interval", TimeValue(MilliSeconds(10)));
    echoClient.SetAttribute("PacketSize", UintegerValue(64));
    ApplicationContainer clientApp = echoClient.Install(staNode24.Get(0));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(10.0));

    // Video traffic (Low Priority)
    OnOffHelper videoStream("ns3::UdpSocketFactory", InetSocketAddress(interfaces5.GetAddress(0), port));
    videoStream.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    videoStream.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    videoStream.SetAttribute("DataRate", DataRateValue(DataRate("10Mbps")));
    videoStream.SetAttribute("PacketSize", UintegerValue(1500));
    ApplicationContainer videoApp = videoStream.Install(staNode5.Get(0));
    videoApp.Start(Seconds(3.0));
    videoApp.Stop(Seconds(10.0));

    // Packet logging
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxEnd", 
                                 MakeCallback(&PacketReceivedCallback));

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}