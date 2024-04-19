#include <iostream>
#include <fstream>
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CsmaMulticastExample");

int 
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nCsma = 1;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  cmd.Parse (argc,argv);

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  nCsma = nCsma == 0 ? 1 : nCsma;

  NodeContainer p2pNodes1;
  p2pNodes1.Create (4);

  NodeContainer p2pNodes2;
  p2pNodes2.Create (3);

  NodeContainer csmaNodes1;
  csmaNodes1.Add (p2pNodes1.Get (2));
  csmaNodes1.Create (nCsma);
  csmaNodes1.Add (p2pNodes2.Get (0));

  NodeContainer csmaNodes2;
  csmaNodes2.Add (p2pNodes2.Get (2));
  csmaNodes2.Create (2);


  PointToPointHelper ptp1;
  ptp1.SetDeviceAttribute ("DataRate", StringValue ("3Mbps"));
  ptp1.SetChannelAttribute ("Delay", StringValue ("5ms"));
  ptp1.SetQueue("ns3::DropTailQueue");

  PointToPointHelper ptp2;
  ptp2.SetDeviceAttribute ("DataRate", StringValue ("3Mbps"));
  ptp2.SetChannelAttribute ("Delay", StringValue ("5ms"));
  ptp2.SetQueue("ns3::DropTailQueue");

  PointToPointHelper ptp3;
  ptp3.SetDeviceAttribute ("DataRate", StringValue ("6Mbps"));
  ptp3.SetChannelAttribute ("Delay", StringValue ("10ms"));
  ptp3.SetQueue("ns3::DropTailQueue");

  PointToPointHelper ptp4;
  ptp4.SetDeviceAttribute ("DataRate", StringValue ("3Mbps"));
  ptp4.SetChannelAttribute ("Delay", StringValue ("5ms"));
  ptp4.SetQueue("ns3::DropTailQueue");

  PointToPointHelper ptp5;
  ptp5.SetDeviceAttribute ("DataRate", StringValue ("3Mbps"));
  ptp5.SetChannelAttribute ("Delay", StringValue ("5ms"));
  ptp5.SetQueue("ns3::DropTailQueue");

  NetDeviceContainer d1, d2, d3, d4, d5;
  d1 = ptp1.Install(p2pNodes1.Get(0),p2pNodes1.Get(2));
  d2 = ptp2.Install(p2pNodes1.Get(1),p2pNodes1.Get(2));
  d3 = ptp3.Install(p2pNodes1.Get(2),p2pNodes1.Get(3));
  d4 = ptp4.Install(p2pNodes2.Get(0),p2pNodes2.Get(1));
  d5 = ptp5.Install(p2pNodes2.Get(1),p2pNodes2.Get(2));

  CsmaHelper csma1, csma2;
  csma1.SetChannelAttribute ("DataRate", StringValue ("90Mbps"));
  csma1.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (10000000)));

  csma2.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma2.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (13000000)));

  NetDeviceContainer cd1,cd2;
  cd1 = csma1.Install (csmaNodes1);
  cd2 = csma2.Install (csmaNodes2);

  InternetStackHelper stack;
  stack.Install (p2pNodes1.Get(0));
  stack.Install (p2pNodes1.Get(1));
  stack.Install (p2pNodes1.Get(3));
  stack.Install (csmaNodes1);
  stack.Install (p2pNodes2.Get(1));
  stack.Install (csmaNodes2);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces1;
  p2pInterfaces1 = address.Assign (d1);
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces2;
  p2pInterfaces2 = address.Assign (d2);
  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces3;
  p2pInterfaces3 = address.Assign (d3);
  address.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces1;
  csmaInterfaces1 = address.Assign (cd1);
  address.SetBase ("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces4;
  p2pInterfaces2 = address.Assign (d4);
  address.SetBase ("10.1.6.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces5;
  p2pInterfaces3 = address.Assign (d5);
  address.SetBase ("10.1.7.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces2;
  csmaInterfaces2 = address.Assign (cd2);

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (p2pNodes1.Get (3));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient11 (csmaInterfaces2.GetAddress (2), 9);
  echoClient11.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient11.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient11.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps11 = echoClient11.Install (csmaNodes2.Get (2));
  clientApps11.Start (Seconds (7.0));
  clientApps11.Stop (Seconds (12.0));

  UdpEchoClientHelper echoClient21 (csmaInterfaces2.GetAddress (2), 9);
  echoClient11.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient11.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient11.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps21 = echoClient21.Install (p2pNodes2.Get (1));
  clientApps21.Start (Seconds (2.0));
  clientApps21.Stop (Seconds (7.0));

  UdpEchoClientHelper echoClient1 (p2pInterfaces1.GetAddress (0), 9);
  echoClient1.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient1.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient1.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps1 = echoClient1.Install (p2pNodes1.Get (1));
  clientApps1.Start (Seconds (12.0));
  clientApps1.Stop (Seconds (22.0));

  UdpEchoClientHelper echoClient2 (p2pInterfaces1.GetAddress (0), 9);
  echoClient1.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient1.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient1.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps2 = echoClient2.Install (csmaNodes2.Get (2));
  clientApps2.Start (Seconds (8.0));
  clientApps2.Stop (Seconds (12.0));

  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (p2pInterfaces1.GetAddress (0), 9));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer sourceApps1 = source.Install (p2pNodes1.Get (0));
  sourceApps1.Start (Seconds (2.0));
  sourceApps1.Stop (Seconds (7.0));

//
// Create a PacketSinkApplication and install it on node 1
//
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer sinkApps = sink.Install (p2pNodes1.Get (2));
  sinkApps.Start (Seconds (2.0));
  sinkApps.Stop (Seconds (7.0));

  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  AsciiTraceHelper ascii;
  csma2.EnableAsciiAll (ascii.CreateFileStream ("midterm.tr"));
  csma2.EnablePcapAll ("midterm", false);

  NS_LOG_INFO ("Run Simulation.");
  AnimationInterface anim("midterm.xml");
  anim.SetConstantPosition(csmaNodes2.Get(2), 10.0, 5.0);
  anim.SetConstantPosition(csmaNodes2.Get(1), 10.0, 10.0);
  anim.SetConstantPosition(csmaNodes2.Get(0), 10.0, 15.0);
  anim.SetConstantPosition(p2pNodes2.Get(1), 10.0, 20.0);
  anim.SetConstantPosition(csmaNodes1.Get(2), 10.0, 25.0);
  anim.SetConstantPosition(csmaNodes1.Get(1), 10.0, 30.0);
  anim.SetConstantPosition(csmaNodes1.Get(0), 10.0, 35.0);
  anim.SetConstantPosition(p2pNodes1.Get(3), 10.0, 40.0);
  anim.SetConstantPosition(p2pNodes1.Get(0), 5.0, 35.0);
  anim.SetConstantPosition(p2pNodes1.Get(1), 5.0, 40.0);
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
