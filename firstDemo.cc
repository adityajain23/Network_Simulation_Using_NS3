
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
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/netanim-module.h"
//   Wifi 10.1.3.0
//
//    *     *     *
//    |     |     |
//    n2    n1    n0 ------------------------ Rs (Remote Server)
//                AP     point-to-point

using namespace ns3;

//Define Log Component
NS_LOG_COMPONENT_DEFINE("HomeNetwork");

int main(int argc, char *argv[])
{
    bool verbose = true;
    //uint32_t nCsma = 3;
    uint32_t nWifi = 2;
    bool tracing = true;

    CommandLine cmd(__FILE__);

    cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);

    cmd.Parse(argc, argv);

    // The underlying restriction of 18 is due to the grid position
    // allocator's configuration; the grid layout will exceed the
    // bounding box if more than 18 nodes are provided.
    if (nWifi > 18)
    {
        std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box" << std::endl;
        return 1;
    }

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
    //-------------------------------------------------------------------------------------------------------------------
    //Create nodes Rs(Remote Server) and n0
    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    //Establishes a Point to Point (P2P) connection between Rs and n0

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    InternetStackHelper stack;
    stack.Install(p2pNodes.Get(1));

    //-----------------------------------------------------------------------------------------------------------------------

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWifi);
    NodeContainer wifiApNode = p2pNodes.Get(0);

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

    MobilityHelper mobility;

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(5.0),
                                  "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(3),
                                  "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.Install(wifiStaNodes);

    //Access Point AP is stationary and does not move
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(p2pNodes.Get(1));

    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    //Assign IP addresses to net devices
    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0"); //10.1.1.2 remote server
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(p2pDevices);

    address.SetBase("10.1.3.0", "255.255.255.0"); //10.1.3.1 laptop , 10.1.3.2 mobile
    Ipv4InterfaceContainer wifiNodesInterfaces;
    Ipv4InterfaceContainer apNodeInterface;

    wifiNodesInterfaces = address.Assign(staDevices);

    apNodeInterface = address.Assign(apDevices);

    //--------------------------------------------------------------------------------------
    // Setting  applications
    //--------------------------------------------------------------------------------------

    //Laptop to Rs and Mobile to Rs
    UdpEchoServerHelper echoServer(9);
    //Make server Rs
    ApplicationContainer serverApps = echoServer.Install(p2pNodes.Get(1));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    UdpEchoClientHelper echoClient(p2pInterfaces.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));
    //Make client on Laptop and Mobile
    ApplicationContainer clientApps = echoClient.Install(wifiStaNodes.Get(nWifi - 1));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));

    ApplicationContainer client1Apps = echoClient.Install(wifiStaNodes.Get(nWifi - 2));
    client1Apps.Start(Seconds(3.0));
    client1Apps.Stop(Seconds(10.0));

    //Laptop to Mobile communcation
    UdpEchoServerHelper echo1Server(13);
    //Make server on Laptop
    ApplicationContainer server1Apps = echo1Server.Install(wifiStaNodes.Get(0));
    server1Apps.Start(Seconds(11.0));
    server1Apps.Stop(Seconds(20.0));
    //Make client on Mobile
    UdpEchoClientHelper echo1Client(wifiNodesInterfaces.GetAddress(0), 13);
    echo1Client.SetAttribute("MaxPackets", UintegerValue(1));
    echo1Client.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echo1Client.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer client3Apps = echo1Client.Install(wifiStaNodes.Get(1));
    client3Apps.Start(Seconds(12.0));
    client3Apps.Stop(Seconds(20.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(20.0));

    if (tracing == true)
    {
        pointToPoint.EnablePcapAll("Home_Network", p2pDevices.Get(0));
        phy.EnablePcap("Home_Network", apDevices.Get(0));
        phy.EnablePcap("Home_Network", staDevices.Get(0));
        phy.EnablePcap("Home_Network", staDevices.Get(1));
    }
    AnimationInterface anim("HomeNetwork.xml");
    anim.SetConstantPosition(p2pNodes.Get(1), 10.0, 20.0);
    anim.SetConstantPosition(p2pNodes.Get(0), 3.0, 3.0);
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
