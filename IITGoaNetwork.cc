/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/ipv4-global-routing-helper.h"

//Udp server --> (Local0, Remote0, Remote1) -->R0, L0 R1

// Network Topology:

// R0                                         R1
//  |                                         |
//  |  152.66.1.0                             |   100.10.1.0
//  |  point-to-point                         |   point-to-point
//  |                                         |
//  |--------------------L0-------------------|
//                        |
//                        |
//                        |
//  |-------------------------------------------|
//  |                                           |
//  |  10.1.0.0                                 |   10.10.10.0
//  |  point-to-point                           |   point-to-point
//  |                                           |
// n1  n2   n3   n4                           WiFi
// |    |    |    |                            n0*    n1*    n2*
// ================                          ApNode     10.10.30.0
//   LAN 10.1.1.0

// ------------------------------------------------------------------------------------------------------------
// Packet Flow:
//
// L0 --> R0 3 packets (from: 1s, to: 7s, delay: 1s)
// L0 --> R1 3 packets (from: 2.5s, to: 5s, delay: 0.5s)
// n4 --> L0 2 packets (from: 4s, to: 8s, delay: 1.5s)
// n2*--> L0 1 packets (from: 4s, to: 9s, delay: 1.25s)
// n3 --> R0 3 packets (from: 5s, to: 10s, delay: 1s)
// n1*--> R1 2 packets (from: 6.5s, to: 9s, delay: 0.3s)
// n2 --> n2* 1 packets (from: 6s, to: 10s, delay: 1s)
//
// ------------------------------------------------------------------------------------------------------------

using namespace ns3;

// Define log component
NS_LOG_COMPONENT_DEFINE("IITGoaNetwork");

int main(int argc, char *argv[])
{
  // Defining Constants Used
  uint32_t nCsma = 3;
  uint32_t nWifi = 2;

  Time::SetResolution(Time::NS);
  LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
  NS_LOG_INFO("Creating Topology");

  // ------------------------------------------------------------------------------------------------------------

  // Create nodes R0 and L0
  NodeContainer server1Nodes;
  server1Nodes.Create(2);

  // Establish P2P connection in R0 and L0
  PointToPointHelper pointToPoint1;
  pointToPoint1.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  pointToPoint1.SetChannelAttribute("Delay", StringValue("2ms"));
  NetDeviceContainer server1Devices;
  server1Devices = pointToPoint1.Install(server1Nodes);

  // Import internet stack on nodes.
  InternetStackHelper stack1;
  stack1.Install(server1Nodes);

  // Assign IP addresses to net devices
  Ipv4AddressHelper address1;
  address1.SetBase("152.66.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces1 = address1.Assign(server1Devices);

  // Make server on R0
  UdpEchoServerHelper echoServer1(9);
  ApplicationContainer serverApps1 = echoServer1.Install(server1Nodes.Get(1));
  serverApps1.Start(Seconds(0.0));
  serverApps1.Stop(Seconds(11.0));

  // Make client on for R0 on L0
  UdpEchoClientHelper echoClient1(interfaces1.GetAddress(1), 9);
  echoClient1.SetAttribute("MaxPackets", UintegerValue(4));
  echoClient1.SetAttribute("Interval", TimeValue(Seconds(1.0)));
  echoClient1.SetAttribute("PacketSize", UintegerValue(1024));
  ApplicationContainer clientApps1 = echoClient1.Install(server1Nodes.Get(0));
  clientApps1.Start(Seconds(1.0));
  clientApps1.Stop(Seconds(7.0));

  // ------------------------------------------------------------------------------------------------------------

  // Create node R1
  NodeContainer server2Nodes;
  server2Nodes.Create(1);

  // Establish P2P connection in R1 and L0
  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
  pointToPoint2.SetChannelAttribute("Delay", StringValue("10ms"));
  NetDeviceContainer server2Devices;
  server2Devices = pointToPoint2.Install(server2Nodes.Get(0), server1Nodes.Get(0));

  // Import internet stack on nodes.
  InternetStackHelper stack2;
  stack2.Install(server2Nodes);

  // Assign IP addresses to net devices
  Ipv4AddressHelper address2;
  address2.SetBase("100.10.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces2 = address2.Assign(server2Devices);

  // Make server on R1
  UdpEchoServerHelper echoServer2(20);
  ApplicationContainer serverApps2 = echoServer2.Install(server2Nodes.Get(0));
  serverApps2.Start(Seconds(0.0));
  serverApps2.Stop(Seconds(11.0));

  // Make client for R1 on L0
  UdpEchoClientHelper echoClient2(interfaces2.GetAddress(0), 20);
  echoClient2.SetAttribute("MaxPackets", UintegerValue(4));
  echoClient2.SetAttribute("Interval", TimeValue(Seconds(0.5)));
  echoClient2.SetAttribute("PacketSize", UintegerValue(256));
  ApplicationContainer clientApps2 = echoClient2.Install(server1Nodes.Get(0));
  clientApps2.Start(Seconds(2.5));
  clientApps2.Stop(Seconds(5.0));

  // ------------------------------------------------------------------------------------------------------------
  // Create LAN Network

  // Create node n1
  NodeContainer LANServerNodes;
  LANServerNodes.Create(1);

  // Establish P2P connection in n1 and L0
  PointToPointHelper pointToPoint3;
  pointToPoint3.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  pointToPoint3.SetChannelAttribute("Delay", StringValue("1ms"));
  NetDeviceContainer LANServerDevices;
  LANServerDevices = pointToPoint3.Install(server1Nodes.Get(0), LANServerNodes.Get(0));

  // Create csma nodes and add node n1 to the container.
  NodeContainer csmaNodes;
  csmaNodes.Add(LANServerNodes.Get(0));
  csmaNodes.Create(nCsma);

  // Making Mobility Model for CSMA nodes.
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(csmaNodes);

  // Giving channel Attributes to csma nodes
  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
  csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

  // Make csma devices
  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install(csmaNodes);

  // Import internet stack on nodes.
  InternetStackHelper stack3;
  stack3.Install(csmaNodes);

  // Assign IP addresses to net devices
  Ipv4AddressHelper address3;
  address3.SetBase("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer interfaces3 = address3.Assign(LANServerDevices);

  // Assign IP addresses to csma devices.
  Ipv4AddressHelper address4;
  address4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address4.Assign(csmaDevices);

  // Make server on node L0
  UdpEchoServerHelper echoServerLocal(30);
  ApplicationContainer serverAppsLocal = echoServerLocal.Install(server1Nodes.Get(0));
  serverAppsLocal.Start(Seconds(0.0));
  serverAppsLocal.Stop(Seconds(11.0));

  // Make client for L0 on node n4
  UdpEchoClientHelper echoClientLAN(interfaces3.GetAddress(0), 30);
  echoClientLAN.SetAttribute("MaxPackets", UintegerValue(3));
  echoClientLAN.SetAttribute("Interval", TimeValue(Seconds(1.5)));
  echoClientLAN.SetAttribute("PacketSize", UintegerValue(512));
  ApplicationContainer clientAppsLAN = echoClientLAN.Install(csmaNodes.Get(nCsma));
  clientAppsLAN.Start(Seconds(4.0));
  clientAppsLAN.Stop(Seconds(8.0));

  // ------------------------------------------------------------------------------------------------------------
  // Create WIFI Network

  // Create node n0*
  NodeContainer WifiServerNode;
  WifiServerNode.Create(1);

  // Establish P2P connection in n0* and L0
  PointToPointHelper pointToPoint4;
  pointToPoint4.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  pointToPoint4.SetChannelAttribute("Delay", StringValue("2ms"));
  NetDeviceContainer WifiServerDevices;
  WifiServerDevices = pointToPoint4.Install(WifiServerNode.Get(0), server1Nodes.Get(0));

  // Create wifi nodes
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create(nWifi);
  NodeContainer wifiApNode = WifiServerNode.Get(0);

  // Giving channel Attributes to wifi nodes
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

  NetDeviceContainer staDevices;
  staDevices = wifi.Install(phy, mac, wifiStaNodes);

  mac.SetType("ns3::ApWifiMac",
              "Ssid", SsidValue(ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install(phy, mac, wifiApNode);

  // Setting mobility model for wifi nodes
  MobilityHelper mobility1;

  mobility1.SetPositionAllocator("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue(100.0),
                                 "MinY", DoubleValue(100.0),
                                 "DeltaX", DoubleValue(5.0),
                                 "DeltaY", DoubleValue(10.0),
                                 "GridWidth", UintegerValue(3),
                                 "LayoutType", StringValue("RowFirst"));

  mobility1.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue(Rectangle(-50, 150, -50, 150)));
  mobility1.Install(wifiStaNodes);

  mobility1.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility1.Install(wifiApNode);

  // Import internet stack on nodes
  InternetStackHelper stack4;
  stack4.Install(wifiApNode);
  stack4.Install(wifiStaNodes);

  // Assign IP addresses to Devices
  Ipv4AddressHelper address5;
  address5.SetBase("10.10.10.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces4;
  interfaces4 = address5.Assign(WifiServerDevices);

  address5.SetBase("10.10.30.0", "255.255.255.0");
  Ipv4InterfaceContainer wifiNodesInterfaces;
  Ipv4InterfaceContainer apNodeInterface;

  wifiNodesInterfaces = address5.Assign(staDevices);
  apNodeInterface = address5.Assign(apDevices);

  // Make server on L0
  UdpEchoServerHelper echoServer3(100);

  ApplicationContainer serverApps3 = echoServer3.Install(server1Nodes.Get(0));
  serverApps3.Start(Seconds(0.0));
  serverApps3.Stop(Seconds(11.0));

  // Make client on n2* to L0
  UdpEchoClientHelper echoClient3(interfaces3.GetAddress(0), 100);
  echoClient3.SetAttribute("MaxPackets", UintegerValue(3));
  echoClient3.SetAttribute("Interval", TimeValue(Seconds(1.25)));
  echoClient3.SetAttribute("PacketSize", UintegerValue(1024));

  ApplicationContainer clientApps3 = echoClient3.Install(wifiStaNodes.Get(nWifi - 1));
  clientApps3.Start(Seconds(4.0));
  clientApps3.Stop(Seconds(9.0));

  // ------------------------------------------------------------------------------------------------------------
  // Data transfer between n3 and R0

  // Make client on n3 to R0
  echoClient1.SetAttribute("MaxPackets", UintegerValue(3));
  echoClient1.SetAttribute("Interval", TimeValue(Seconds(1.0)));
  echoClient1.SetAttribute("PacketSize", UintegerValue(512));

  ApplicationContainer clientApps4 = echoClient1.Install(csmaNodes.Get(nCsma - 1));
  clientApps4.Start(Seconds(5.0));
  clientApps4.Stop(Seconds(10.0));

  // ------------------------------------------------------------------------------------------------------------
  // Data transfer between n1* and R1

  // Make client on n1* to R1
  echoClient2.SetAttribute("MaxPackets", UintegerValue(3));
  echoClient2.SetAttribute("Interval", TimeValue(Seconds(0.3)));
  echoClient2.SetAttribute("PacketSize", UintegerValue(1024));

  ApplicationContainer clientApps5 = echoClient2.Install(wifiStaNodes.Get(nWifi - 2));
  clientApps5.Start(Seconds(6.5));
  clientApps5.Stop(Seconds(9.0));

  // ------------------------------------------------------------------------------------------------------------
  // Data transfer between n2 and n2*

  // Make server on n2*
  UdpEchoServerHelper echoServerWifi(122);
  ApplicationContainer serverAppsWifi = echoServerWifi.Install(wifiStaNodes.Get(nWifi - 1));
  serverAppsWifi.Start(Seconds(0.0));
  serverAppsWifi.Stop(Seconds(11.0));

  // Make client on n2 to n2*
  UdpEchoClientHelper echoClient6(wifiNodesInterfaces.GetAddress(1), 122);
  echoClient6.SetAttribute("MaxPackets", UintegerValue(3));
  echoClient6.SetAttribute("Interval", TimeValue(Seconds(1)));
  echoClient6.SetAttribute("PacketSize", UintegerValue(1024));
  ApplicationContainer clientApps6 = echoClient6.Install(csmaNodes.Get(nCsma - 2));
  clientApps6.Start(Seconds(6.0));
  clientApps6.Stop(Seconds(10.0));

  // ------------------------------------------------------------------------------------------------------------

  mobility1.Install(server1Nodes);
  mobility1.Install(server2Nodes);

  // ------------------------------------------------------------------------------------------------------------

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  Simulator::Stop(Seconds(11.0));

  // -------------------------------------------
  pointToPoint1.EnablePcapAll("IITGoa_Network", server1Nodes.Get(0));
  pointToPoint3.EnablePcapAll("IITGoa_Network", LANServerNodes.Get(0));
  csma.EnablePcapAll("IITGoa_Network", csmaNodes.Get(nCsma));
  phy.EnablePcapAll("IITGoa_Network", WifiServerNode.Get(0));
  phy.EnablePcapAll("IITGoa_Network", wifiStaNodes.Get(nWifi - 1));
  // -------------------------------------------
  // Setting the position of nodes on NetAnim
  AnimationInterface anim("IITGoaNetwork.xml");

  anim.UpdateNodeDescription(0, "Local Server");
  anim.UpdateNodeDescription(1, "Remote Server 1");
  anim.UpdateNodeDescription(2, "Remote Server 2");
  anim.UpdateNodeDescription(3, "LAN Node 1");
  anim.UpdateNodeDescription(4, "LAN Node 2");
  anim.UpdateNodeDescription(5, "LAN Node 3");
  anim.UpdateNodeDescription(6, "LAN Node 4");
  anim.UpdateNodeDescription(7, "WiFi AP Node");
  anim.UpdateNodeDescription(8, "WiFi Device 1");
  anim.UpdateNodeDescription(9, "WiFi Deivce 2");

  std::string path = "/home/percy/ns3/ns-allinone-3.33/ns-3.33/assets/";
  uint32_t mobileImg = anim.AddResource(path + "mobile.png");
  uint32_t laptopImg = anim.AddResource(path + "laptop.png");
  uint32_t routerImg = anim.AddResource(path + "router.png");
  uint32_t serverImg = anim.AddResource(path + "server.png");

  anim.UpdateNodeImage(0, serverImg);
  anim.UpdateNodeSize(0, 20.0, 20.0);
  anim.UpdateNodeImage(1, serverImg);
  anim.UpdateNodeSize(1, 20.0, 20.0);
  anim.UpdateNodeImage(2, serverImg);
  anim.UpdateNodeSize(2, 20.0, 20.0);
  anim.UpdateNodeImage(3, laptopImg);
  anim.UpdateNodeSize(3, 20.0, 20.0);
  anim.UpdateNodeImage(4, laptopImg);
  anim.UpdateNodeSize(4, 20.0, 20.0);
  anim.UpdateNodeImage(5, laptopImg);
  anim.UpdateNodeSize(5, 20.0, 20.0);
  anim.UpdateNodeImage(6, laptopImg);
  anim.UpdateNodeSize(6, 20.0, 20.0);
  anim.UpdateNodeImage(7, routerImg);
  anim.UpdateNodeSize(7, 20.0, 20.0);
  anim.UpdateNodeImage(8, mobileImg);
  anim.UpdateNodeSize(8, 20.0, 20.0);
  anim.UpdateNodeImage(9, mobileImg);
  anim.UpdateNodeSize(9, 20.0, 20.0);

  anim.SetConstantPosition(server1Nodes.Get(0), 50.0, 50.0);
  anim.SetConstantPosition(server1Nodes.Get(1), 0.0, 0.0);
  anim.SetConstantPosition(server2Nodes.Get(0), 100.0, 0.0);

  for (uint32_t i = 0; i <= nCsma; i++)
  {
    anim.SetConstantPosition(csmaNodes.Get(i), double(25 * i), double(150));
  }

  anim.SetConstantPosition(WifiServerNode.Get(0), 100.0, 75.0);
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
