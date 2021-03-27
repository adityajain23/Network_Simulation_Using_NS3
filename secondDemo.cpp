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
#include "ns3/ipv4-global-routing-helper.h"

//Udp server --> (Local0, Remote0, Remote1) -->R0, L0 R1

// Default Network Topology
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
//  |  10.1.0.0                                 |   10.150.0.0 
//  |  point-to-point                           |   
//  |                                           |
// n1  n2   n3   n4                           WiFi
// |    |    |    |                             
// ================                             
//   LAN 10.1.1.0






using namespace ns3;

// Define log component
NS_LOG_COMPONENT_DEFINE("IITGoaNetwork");

int main(int argc, char *argv[])
{
    uint32_t nPackets = 2;
    CommandLine cmd(__FILE__);
    cmd.AddValue("nPackets", "Number of packets to echo", nPackets);
    cmd.Parse(argc, argv);

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
    // Make server R0
    UdpEchoServerHelper echoServer1(9);
    ApplicationContainer serverApps1 = echoServer1.Install(server1Nodes.Get(1));
    serverApps1.Start(Seconds(1.0));
    serverApps1.Stop(Seconds(20.0));
    // Make client on L0 to R0
    UdpEchoClientHelper echoClient1(interfaces1.GetAddress(1), 9);
    echoClient1.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient1.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient1.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps1 = echoClient1.Install(server1Nodes.Get(0));
    clientApps1.Start(Seconds(1.0));
    clientApps1.Stop(Seconds(11.0));

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
    // Make server R1
    UdpEchoServerHelper echoServer2(20);
    ApplicationContainer serverApps2 = echoServer2.Install(server2Nodes.Get(0));
    serverApps2.Start(Seconds(1.0));
    serverApps2.Stop(Seconds(20.0));
    // Make client on L0 to R1
    UdpEchoClientHelper echoClient2(interfaces2.GetAddress(0), 20);
    echoClient2.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient2.SetAttribute("Interval", TimeValue(Seconds(0.5)));
    echoClient2.SetAttribute("PacketSize", UintegerValue(256));
    ApplicationContainer clientApps2 = echoClient2.Install(server1Nodes.Get(0));
    clientApps2.Start(Seconds(5.0));
    clientApps2.Stop(Seconds(20.0));

// ------------------------------------------------------------------------------------------------------------
// Create LAN Network

    uint32_t nCsma = 3;
    
    // Create node n1
    NodeContainer LANServerNodes;
    LANServerNodes.Create(1);

    // Establish P2P connection in n1 and L0
    PointToPointHelper pointToPoint3;
    pointToPoint3.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pointToPoint3.SetChannelAttribute("Delay", StringValue("1ms"));
    NetDeviceContainer LANServerDevices;
    LANServerDevices = pointToPoint3.Install(server1Nodes.Get(0),LANServerNodes.Get(0));

    // Create csma nodes
    NodeContainer csmaNodes;
    csmaNodes.Add (LANServerNodes.Get (0));
    csmaNodes.Create (nCsma);
    
    // Importing Mobility Helper on csma nodes
    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (csmaNodes);
    
    // Giving channel Attributes to csma nodes
    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
    csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

    // Make csma devices
    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install (csmaNodes);

    // Import internet stack on nodes.
    InternetStackHelper stack3;
    // stack3.Install(LANServerNodes.Get(0));
    stack3.Install (csmaNodes);

    // Assign IP addresses to net devices
    Ipv4AddressHelper address3;
    address3.SetBase("10.1.0.0", "255.255.0.0");
    Ipv4InterfaceContainer interfaces3 = address3.Assign(LANServerDevices);
    
    // Assign IP addresses to csma devices.
    Ipv4AddressHelper address4;
    address4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces;
    csmaInterfaces = address4.Assign (csmaDevices);

    // Make server L0
    UdpEchoServerHelper echoServerLocal(30);
    ApplicationContainer serverAppsLocal = echoServerLocal.Install(server1Nodes.Get(0));
    serverAppsLocal.Start(Seconds(1.0));
    serverAppsLocal.Stop(Seconds(20.0));

    // Make client on n1 to L0
    UdpEchoClientHelper echoClientLAN(interfaces3.GetAddress(0), 30);
    echoClientLAN.SetAttribute("MaxPackets", UintegerValue(2));
    echoClientLAN.SetAttribute("Interval", TimeValue(Seconds(1)));
    echoClientLAN.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer clientAppsLAN = echoClientLAN.Install(csmaNodes.Get (nCsma));
    clientAppsLAN.Start(Seconds(12.0));
    clientAppsLAN.Stop(Seconds(20.0));

// ------------------------------------------------------------------------------------------------------------

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


    AnimationInterface anim("IITGoaNetwork.xml");
    anim.SetConstantPosition(server1Nodes.Get(0), 50.0, 50.0);
    anim.SetConstantPosition(server1Nodes.Get(1), 0.0, 0.0);
    anim.SetConstantPosition(server2Nodes.Get(0), 100.0, 0.0);
    
    for (uint32_t i=0; i<=nCsma; i++)
  {
    anim.SetConstantPosition(csmaNodes.Get(i), double(25*i), double(150));
  }  

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
