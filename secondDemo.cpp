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


//Udp server --> (Local0, Remote0, Remote1) -->R0, L0 R1



// Default Network Topology
//
//       152.66.1.0
// R0 ---------------    (UDP Connection)
//                   |
//                  L0  
//       100.10.1.0  |
// R1 ---------------    (UDP Connection)
//    point-to-point

using namespace ns3;

// Define log component
NS_LOG_COMPONENT_DEFINE ("IITGoaNetwork");

int main (int argc, char *argv[])
{
    uint32_t nPackets = 2;
    CommandLine cmd (__FILE__);
    cmd.AddValue("nPackets", "Number of packets to echo", nPackets);
    cmd.Parse (argc, argv);


    Time::SetResolution (Time::NS);
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    NS_LOG_INFO ("Creating Topology");

    // Create nodes R0 and L0
    NodeContainer server1Nodes;
    server1Nodes.Create (2);
    
    // Establish P2P connection in R0 and L0
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer server1Devices;
    server1Devices = pointToPoint.Install(server1Nodes);

    // Import internet stack on nodes.
    InternetStackHelper stack;
    stack.Install(server1Nodes);

    // Assign IP addresses to net devices
    Ipv4AddressHelper address;
    address.SetBase ("152.66.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces1 = address.Assign (server1Devices);

    UdpEchoServerHelper echoServer1 (9);

    ApplicationContainer serverApps = echoServer1.Install (server1Nodes.Get (1));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (20.0));

    UdpEchoClientHelper echoClient1 (interfaces1.GetAddress (1), 9);
    echoClient1.SetAttribute ("MaxPackets", UintegerValue (nPackets));
    echoClient1.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient1.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps1 = echoClient1.Install (server1Nodes.Get (0));
    clientApps1.Start (Seconds (2.0));
    clientApps1.Stop (Seconds (10.0));

    
}




