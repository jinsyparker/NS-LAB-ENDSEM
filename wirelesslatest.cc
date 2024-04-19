#include <fstream>
#include "ns3/constant-position-mobility-model.h"
#include "ns3/mobility-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/animation-interface.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ssid.h"
#include "ns3/ipv4-routing-table-entry.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FirstScriptExample");

int main(int argc, char *argv[])
{
    CommandLine cmd(_FILE_);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    NodeContainer nodes;
    nodes.Create(13);
    NodeContainer csmaNode1, csmaNode2;
    csmaNode1.Add(nodes.Get(0));
    csmaNode1.Add(nodes.Get(1));
    csmaNode1.Add(nodes.Get(2));

    csmaNode2.Add(nodes.Get(10));
    csmaNode2.Add(nodes.Get(11));
    csmaNode2.Add(nodes.Get(12));
    NodeContainer wifiStaNodes;
    wifiStaNodes.Add(nodes.Get(4));
    wifiStaNodes.Add(nodes.Get(5));
    wifiStaNodes.Add(nodes.Get(6));
    wifiStaNodes.Add(nodes.Get(7));
    wifiStaNodes.Add(nodes.Get(8));

    NodeContainer wifiApNode = nodes.Get(3);
    wifiApNode.Add(nodes.Get(9));

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());
    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");
    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));

    NetDeviceContainer device1, device2, device3, device4, device5, device6, device7, device8;
    device1 = csma.Install(csmaNode1);
    device2 = pointToPoint.Install(nodes.Get(2), nodes.Get(3));
    device3 = wifi.Install(phy, mac, wifiStaNodes);
    device4 = pointToPoint.Install(nodes.Get(9), nodes.Get(10));
    device5 = csma.Install(csmaNode2);
    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid));
    device6 = wifi.Install(phy, mac, wifiApNode);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase(Ipv4Address("10.1.1.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer interface1 = address.Assign(device1);
    address.SetBase(Ipv4Address("10.1.2.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer interface2 = address.Assign(device2);
    address.SetBase(Ipv4Address("10.1.3.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer interface3 = address.Assign(device3);
    Ipv4InterfaceContainer interface6 = address.Assign(device6);
    address.SetBase(Ipv4Address("10.1.4.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer interface4 = address.Assign(device4);
    address.SetBase(Ipv4Address("10.1.5.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer interface5 = address.Assign(device5);

    UdpEchoServerHelper echoServer(9);

    ApplicationContainer serverApps = echoServer.Install(nodes.Get(12));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    UdpEchoClientHelper echoClient(interface5.GetAddress(2), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(10));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps =
    echoClient.Install(wifiStaNodes.Get(2));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));

    NS_LOG_INFO("Configure multicasting.");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Ptr<ConstantPositionMobilityModel> s0 = nodes.Get(0)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s1 = nodes.Get(1)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s2 = nodes.Get(2)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s3 = nodes.Get(3)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s4 = nodes.Get(4)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s5 = nodes.Get(5)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s6 = nodes.Get(6)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s7 = nodes.Get(7)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s8 = nodes.Get(8)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s9 = nodes.Get(9)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s10 = nodes.Get(10)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s11 = nodes.Get(11)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s12 = nodes.Get(12)->GetObject<ConstantPositionMobilityModel>();

    s0->SetPosition(Vector(0, 0, 0));
    s1->SetPosition(Vector(40, 0., 0));
    s2->SetPosition(Vector(0, 60, 0));
    s3->SetPosition(Vector(50, 80, 0));
    s4->SetPosition(Vector(70, 70, 0));
    s5->SetPosition(Vector(90, 60, 0));
    s6->SetPosition(Vector(90, 70, 0));
    s7->SetPosition(Vector(90, 80, 0));
    s8->SetPosition(Vector(110, 70, 0));
    s9->SetPosition(Vector(115, 80, 0));
    s10->SetPosition(Vector(200, 90, 0));
    s11->SetPosition(Vector(200, 0, 0));
    s12->SetPosition(Vector(240, 0, 0));
    AnimationInterface anim("q.xml");
    Simulator::Stop(Seconds(40.0));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
