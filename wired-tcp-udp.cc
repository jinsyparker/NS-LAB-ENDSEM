/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contributed by:  Luis Cortes (cortes@gatech.edu)
 */

// This script exercises global routing code in a mixed point-to-point
// and csma/cd environment.  We bring up and down interfaces and observe
// the effect on global routing.  We explicitly enable the attribute
// to respond to interface events, so that routes are recomputed
// automatically.
//
// Network topology
//
//  n0
//     \ p-p
//      \          (shared csma/cd)
//       n2 -------------------------n3
//      /            |        |
//     / p-p        n4        n5 ---------- n6
//   n1                             p-p
//   |                                      |
//   ----------------------------------------
//                p-p
//
// - at time 1 CBR/UDP flow from n1 to n6's IP address on the n5/n6 link
// - at time 10, start similar flow from n1 to n6's address on the n1/n6 link
//
//  Order of events
//  At pre-simulation time, configure global routes.  Shortest path from
//  n1 to n6 is via the direct point-to-point link
//  At time 1s, start CBR traffic flow from n1 to n6
//  At time 2s, set the n1 point-to-point interface to down.  Packets
//    will be diverted to the n1-n2-n5-n6 path
//  At time 4s, re-enable the n1/n6 interface to up.  n1-n6 route restored.
//  At time 6s, set the n6-n1 point-to-point Ipv4 interface to down (note, this
//    keeps the point-to-point link "up" from n1's perspective).  Traffic will
//    flow through the path n1-n2-n5-n6
//  At time 8s, bring the interface back up.  Path n1-n6 is restored
//  At time 10s, stop the first flow.
//  At time 11s, start a new flow, but to n6's other IP address (the one
//    on the n1/n6 p2p link)
//  At time 12s, bring the n1 interface down between n1 and n6.  Packets
//    will be diverted to the alternate path
//  At time 14s, re-enable the n1/n6 interface to up.  This will change
//    routing back to n1-n6 since the interface up notification will cause
//    a new local interface route, at higher priority than global routing
//  At time 16s, stop the second flow.

// - Tracing of queues and packet receptions to file "dynamic-global-routing.tr"
#include "ns3/applications-module.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/rip-helper.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DynamicGlobalRoutingExample");

int
main(int argc, char* argv[])
{
    // The below value configures the default behavior of global routing.
    // By default, it is disabled.  To respond to interface events, set to true
    bool verbose = false;
    bool printRoutingTables = false;
    bool showPings = false;
    std::string SplitHorizon("PoisonReverse");

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "turn on log components", verbose);
    cmd.AddValue("printRoutingTables",
                 "Print routing tables at 30, 60 and 90 seconds",
                 printRoutingTables);
    cmd.AddValue("showPings", "Show Ping6 reception", showPings);
    cmd.AddValue("splitHorizonStrategy",
                 "Split Horizon strategy to use (NoSplitHorizon, SplitHorizon, PoisonReverse)",
                 SplitHorizon);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_NODE));
        LogComponentEnable("RipSimpleRouting", LOG_LEVEL_INFO);
        LogComponentEnable("Rip", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv4Interface", LOG_LEVEL_ALL);
        LogComponentEnable("Icmpv4L4Protocol", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv4L3Protocol", LOG_LEVEL_ALL);
        LogComponentEnable("ArpCache", LOG_LEVEL_ALL);
        LogComponentEnable("V4Ping", LOG_LEVEL_ALL);
    }

    if (SplitHorizon == "NoSplitHorizon")
    {
        Config::SetDefault("ns3::Rip::SplitHorizon", EnumValue(RipNg::NO_SPLIT_HORIZON));
    }
    else if (SplitHorizon == "SplitHorizon")
    {
        Config::SetDefault("ns3::Rip::SplitHorizon", EnumValue(RipNg::SPLIT_HORIZON));
    }
    else
    {
        Config::SetDefault("ns3::Rip::SplitHorizon", EnumValue(RipNg::POISON_REVERSE));
    }

    NS_LOG_INFO("Create nodes.");
    NodeContainer c;
    c.Create(11);
    NodeContainer n1n3 = NodeContainer(c.Get(0), c.Get(2));
    NodeContainer n1n2 = NodeContainer(c.Get(0), c.Get(1));
    NodeContainer n2n6 = NodeContainer(c.Get(1), c.Get(5));
    NodeContainer n245 = NodeContainer(c.Get(1), c.Get(3), c.Get(4));
    NodeContainer n5n7 = NodeContainer(c.Get(4), c.Get(6));
    NodeContainer n7n9 = NodeContainer(c.Get(6), c.Get(8));
    NodeContainer n9n10 = NodeContainer(c.Get(8), c.Get(9));
    NodeContainer n10n11 = NodeContainer(c.Get(9), c.Get(10));
    NodeContainer n7n8 = NodeContainer(c.Get(6), c.Get(7));
    NodeContainer n8n10 = NodeContainer(c.Get(7), c.Get(9));

    InternetStackHelper internet;
    internet.Install(c);

    // We create the channels first without any IP addressing information
    NS_LOG_INFO("Create channels.");
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer d1d2 = p2p.Install(n1n2);
    NetDeviceContainer d1d3 = p2p.Install(n1n3);

    // p2p.SetDeviceAttribute("DataRate", StringValue("1500kbps"));
    // p2p.SetChannelAttribute("Delay", StringValue("10ms"));
    NetDeviceContainer d2d6 = p2p.Install(n2n6);
    NetDeviceContainer d5d7 = p2p.Install(n5n7);
    NetDeviceContainer d7d9 = p2p.Install(n7n9);
    NetDeviceContainer d9d10 = p2p.Install(n9n10);
    // NetDeviceContainer d7d8 = p2p.Install(n7n8);
    //  // NetDeviceContainer d10d11 = p2p.Install(n10n11);
    NetDeviceContainer d8d10 = p2p.Install(n8n10);

    // We create the channels first without any IP addressing information
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("5Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer d245 = csma.Install(n245);
    NetDeviceContainer d10d11 = csma.Install(n10n11);
    NetDeviceContainer d7d8 = csma.Install(n7n8);

    // Later, we add IP addresses.
    NS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i1i2 = ipv4.Assign(d1d2);

    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i1i3 = ipv4.Assign(d1d3);

    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i2i6 = ipv4.Assign(d2d6);

    ipv4.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer i5i7 = ipv4.Assign(d5d7);

    ipv4.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer i7i9 = ipv4.Assign(d7d9);

    // ipv4.SetBase("10.1.6.0", "255.255.255.0");
    // Ipv4InterfaceContainer i7i8 = ipv4.Assign(d7d8);

    // ipv4.SetBase("10.1.11.0", "255.255.255.0");
    // Ipv4InterfaceContainer i7i8 = ipv4.Assign(d7d8);
    // ipv4.SetBase("10.1.6.0", "255.255.255.0");
    // Ipv4InterfaceContainer i7i8 = ipv4.Assign(d7d8);

    ipv4.SetBase("10.1.7.0", "255.255.255.0");
    Ipv4InterfaceContainer i8i10 = ipv4.Assign(d8d10);

    ipv4.SetBase("10.1.8.0", "255.255.255.0");
    Ipv4InterfaceContainer i9i10 = ipv4.Assign(d9d10);

    ipv4.SetBase("10.250.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i245 = ipv4.Assign(d245);

    ipv4.SetBase("10.250.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i10i11 = ipv4.Assign(d10d11);

    // ipv4.SetBase("172.16.1.0", "255.255.255.0");
    // Ipv4InterfaceContainer i1i6 = ipv4.Assign(d1d6);

    // Create router nodes, initialize routing database and set up the routing
    // tables in the nodes.
    // RipHelper ripHelper;
    // ripHelper.EnablePoissonReverse(true); // Enable poison reverse
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    // Create the OnOff application to send UDP datagrams of size
    // 210 bytes at a rate of 448 Kb/s
    NS_LOG_INFO("Create Applications.");
    uint16_t port = 9; // Discard port (RFC 863)
    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(i1i3.GetAddress(1), port));
    onoff.SetConstantRate(DataRate("2kbps"));
    onoff.SetAttribute("PacketSize", UintegerValue(50));

    ApplicationContainer apps = onoff.Install(c.Get(9));
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(3.0));

    // Create a second OnOff application to send UDP datagrams of size
    // 210 bytes at a rate of 448 Kb/s
    // OnOffHelper onoff2("ns3::UdpSocketFactory", InetSocketAddress(i1i6.GetAddress(1), port));
    // onoff2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    // onoff2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    // onoff2.SetAttribute("DataRate", StringValue("2kbps"));
    // onoff2.SetAttribute("PacketSize", UintegerValue(50));

    // ApplicationContainer apps2 = onoff2.Install(c.Get(1));
    // apps2.Start(Seconds(11.0));
    // apps2.Stop(Seconds(16.0));

    // Create an optional packet sink to receive these packets
    PacketSinkHelper sink("ns3::UdpSocketFactory",
                          Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
    apps = sink.Install(c.Get(9));
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(3.0));

    PacketSinkHelper sinkHelper(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(),
                          port)); // Listen on any address at the specified port
    ApplicationContainer serverApps =
        sinkHelper.Install(c.Get(5)); // Install the server application on node 19
    serverApps.Start(Seconds(5.0));   // Start the server immediately
    serverApps.Stop(Seconds(10.0));   // Stop the server at time 10

    // Create a TCP client (sender) on node 1
    BulkSendHelper clientHelper(
        "ns3::TcpSocketFactory",
        InetSocketAddress(i2i6.GetAddress(1),
                          port)); // Use the IP address of node 19 as the destination
    clientHelper.SetAttribute("MaxBytes", UintegerValue(0)); // Send unlimited bytes
    ApplicationContainer clientApps =
        clientHelper.Install(c.Get(2)); // Install the client application on node 1
    clientApps.Start(Seconds(5.0));     // Start the client at time 1
    clientApps.Stop(Seconds(10.0));     // Stop the client at time 10

    // PacketSinkHelper sink2("ns3::UdpSocketFactory",
    //                        Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
    // apps2 = sink2.Install(c.Get(6));
    // apps2.Start(Seconds(11.0));
    // apps2.Stop(Seconds(16.0));

    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream("dynamic-global-routing.tr");
    p2p.EnableAsciiAll(stream);
    csma.EnableAsciiAll(stream);
    internet.EnableAsciiIpv4All(stream);

    p2p.EnablePcapAll("dynamic-global-routing");
    csma.EnablePcapAll("dynamic-global-routing", false);

    // Ptr<Node> n1 = c.Get(1);
    // Ptr<Ipv4> ipv41 = n1->GetObject<Ipv4>();
    // // The first ifIndex is 0 for loopback, then the first p2p is numbered 1,
    // // then the next p2p is numbered 2
    // uint32_t ipv4ifIndex1 = 2;

    // Simulator::Schedule(Seconds(2), &Ipv4::SetDown, ipv41, ipv4ifIndex1);
    // Simulator::Schedule(Seconds(4), &Ipv4::SetUp, ipv41, ipv4ifIndex1);

    // Ptr<Node> n6 = c.Get(6);
    // Ptr<Ipv4> ipv46 = n6->GetObject<Ipv4>();
    // The first ifIndex is 0 for loopback, then the first p2p is numbered 1,
    // then the next p2p is numbered 2
    // uint32_t ipv4ifIndex6 = 2;
    // Simulator::Schedule(Seconds(6), &Ipv4::SetDown, ipv46, ipv4ifIndex6);
    // Simulator::Schedule(Seconds(8), &Ipv4::SetUp, ipv46, ipv4ifIndex6);

    // Simulator::Schedule(Seconds(12), &Ipv4::SetDown, ipv41, ipv4ifIndex1);
    // Simulator::Schedule(Seconds(14), &Ipv4::SetUp, ipv41, ipv4ifIndex1);

    // Trace routing tables
    Ptr<OutputStreamWrapper> routingStream =
        Create<OutputStreamWrapper>("dynamic-global-routing.routes", std::ios::out);
    Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(12), routingStream);

    AnimationInterface anim("l2q1_midsem_dem.xml");
    anim.SetConstantPosition(c.Get(0), 0.0, 0.0);
    anim.SetConstantPosition(c.Get(1), 20.0, 0.0);
    anim.SetConstantPosition(c.Get(2), 0.0, 20.0);
    anim.SetConstantPosition(c.Get(5), 20.0, 20.0);
    anim.SetConstantPosition(c.Get(3), 40.0, 5.0);
    anim.SetConstantPosition(c.Get(4), 60.0, 0.0);
    anim.SetConstantPosition(c.Get(6), 80.0, 0.0);
    anim.SetConstantPosition(c.Get(7), 100.0, 20.0);
    anim.SetConstantPosition(c.Get(8), 100.0, -20.0);
    anim.SetConstantPosition(c.Get(9), 120.0, 0.0);
    anim.SetConstantPosition(c.Get(10), 140.0, 0.0);

    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}

// run -> cmd command -> ./ns3 run scratch/first -- --splitHorizonStrategy=SplitHorizon