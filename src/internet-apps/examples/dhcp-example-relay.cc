/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 UPB
 * Copyright (c) 2017 NITK Surathkal
 *
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
 * Author: Radu Lupu <rlupu@elcom.pub.ro>
 *         Ankit Deepak <adadeepak8@gmail.com>
 *         Deepti Rajagopal <deeptir96@gmail.com>
 *
 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

/*Define a Log component with the name DhcpExample*/
NS_LOG_COMPONENT_DEFINE ("DhcpExample");

int main (int argc, char *argv[])
{
  CommandLine cmd;

  bool verbose = false;
  bool tracing = false;
  cmd.AddValue ("verbose", "turn on the logs", verbose);
  cmd.AddValue ("tracing", "turn on the tracing", tracing);

  /*Parse the program arguments*/
  cmd.Parse (argc, argv);

  // GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  if (verbose)
    {
      LogComponentEnable ("DhcpServer", LOG_LEVEL_ALL);
      LogComponentEnable ("DhcpClient", LOG_LEVEL_ALL);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    }

  Time stopTime = Seconds (20);

  NS_LOG_INFO ("Create nodes.");

  /*NodeContainer - Keeps track of a set of node pointers. Typically ns-3 helpers operate on more than one node at 
  a time. For example, a device helper may want to install devices on a large number of similar nodes. The helper 
  Install methods usually take a NodeContainer as a parameter. NodeContainers hold the multiple Ptr<Node> which 
  are used to refer to the nodes*/
  NodeContainer nodes;
  NodeContainer router;
  

  /*Create 3 nodes and append pointers to them to the end of this NodeContainer, i.e.,nodes*/
  nodes.Create (3);
  router.Create (2);

  /*Create a node container which is a concatenation of two input NodeContainers*/
  NodeContainer net (nodes, router);

  NS_LOG_INFO ("Create channels.");

  /*Build a set of CsmaNetDevice objects*/
  CsmaHelper csma;

  /*SetChannelAttribute - Set these attributes on each ns3::CsmaChannel created by CsmaHelper::Install*/
  csma.SetChannelAttribute ("DataRate", StringValue ("5Mbps"));
  csma.SetChannelAttribute ("Delay", StringValue ("2ms"));

  /*SetDeviceAttribute - Set these attributes on each ns3::CsmaNetDevice created by CsmaHelper::Install*/
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1500));

  /*Install - This method creates an ns3::CsmaChannel with the attributes configured by 
  CsmaHelper::SetChannelAttribute, an ns3::CsmaNetDevice with the attributes configured by 
  CsmaHelper::SetDeviceAttribute and then adds the device to the node, i.e., net and attaches the 
  channel to the device*/
  NetDeviceContainer devNet = csma.Install (net);

  NodeContainer p2pNodes;

  /*Add - Append the contents of another NodeContainer to the end of this container*/
  p2pNodes.Add (net.Get (4));
  p2pNodes.Create (1);

  /*PointToPointHelper - Build a set of PointToPointNetDevice objects*/
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  /*InternetStackHelper - Aggregate IP/TCP/UDP functionality to existing Nodes*/
  InternetStackHelper tcpip;

  /*Install - Aggregate implementations of the ns3::Ipv4, ns3::Ipv6, ns3::Udp, and ns3::Tcp classes onto the 
  provided node*/
  tcpip.Install (nodes);
  tcpip.Install (router);
  tcpip.Install (p2pNodes.Get (1));

  /*Ipv4AddressHelper - This class is a very simple IPv4 address generator*/
  Ipv4AddressHelper address;

  /*Set the base network number, network mask and base address. The address helper allocates IP addresses 
  based on a given network number and mask combination along with an initial IP address.
  For example, if you want to use a /24 prefix with an initial network number of 192.168.1 (corresponding 
  to a mask of 255.255.255.0) and you want to start allocating IP addresses out of that network beginning 
  at 192.168.1.3, you would call
  SetBase ("192.168.1.0", "255.255.255.0", "0.0.0.3");
  If you don't care about the initial address it defaults to "0.0.0.1" in which case you can simply use,
  SetBase ("192.168.1.0", "255.255.255.0");
  and the first address generated will be 192.168.1.1*/
  address.SetBase ("172.30.1.0", "255.255.255.0");

  /*Ipv4InterfaceContainer - Holds a vector of std::pair of Ptr<Ipv4> and interface index. Typically ns-3 
  Ipv4Interfaces are installed on devices using an Ipv4 address helper. The helper's Assign() method takes 
  a NetDeviceContainer which holds some number of Ptr<NetDevice>. For each of the NetDevices in the 
  NetDeviceContainer the helper will find the associated Ptr<Node> and Ptr<Ipv4>. It makes sure that an 
  interface exists on the node for the device and then adds an Ipv4Address according to the address helper 
  settings (incrementing the Ipv4Address somehow as it goes). The helper then converts the Ptr<Ipv4> and the 
  interface index to a std::pair and adds them to a container – a container of this type*/
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  // manually add a routing entry because we don't want to add a dynamic routing
  Ipv4StaticRoutingHelper ipv4RoutingHelper;

  /*Get - Get the Ptr<Node> stored in this container at a given index
  GetObject - Get a pointer to the requested aggregated Object*/
  Ptr<Ipv4> ipv4Ptr = p2pNodes.Get (1)->GetObject<Ipv4> ();

  /*GetStaticRouting - Try and find the static routing protocol as either the main routing protocol or in the 
  list of routing protocols associated with the Ipv4 provided*/
  Ptr<Ipv4StaticRouting> staticRoutingA = ipv4RoutingHelper.GetStaticRouting (ipv4Ptr);

  /*AddNetworkRouteTo(network, networkMask, interface, metric) - Add a network route to the static routing table*/
  staticRoutingA->AddNetworkRouteTo (Ipv4Address ("172.30.0.0"), Ipv4Mask ("/24"),
                                     Ipv4Address ("172.30.1.1"), 1);

  NS_LOG_INFO ("Setup the IP addresses and create DHCP applications.");

  /*DhcpHelper - The helper class used to configure and install DHCP applications on nodes*/
  DhcpHelper dhcpHelper;

  // The router must have a fixed IP.
  Ipv4InterfaceContainer fixedNodes = dhcpHelper.InstallFixedAddress (devNet.Get (4), 
                                                                      Ipv4Address ("172.30.0.17"), 
                                                                      Ipv4Mask ("/24"));
                                                                                  
  // Not really necessary, IP forwarding is enabled by default in IPv4.
  fixedNodes.Get (0).first->SetAttribute ("IpForward", BooleanValue (true));

  // DHCP server
  ApplicationContainer dhcpServerApp = dhcpHelper.InstallDhcpServer (devNet.Get (3), Ipv4Address ("172.30.0.12"),
                                                                     Ipv4Address ("172.30.0.0"), Ipv4Mask ("/24"),
                                                                     Ipv4Address ("172.30.0.10"), Ipv4Address ("172.30.0.15"),
                                                                     Ipv4Address ("172.30.0.17"));

  // This is just to show how it can be done.
  DynamicCast<DhcpServer> (dhcpServerApp.Get (0))->AddStaticDhcpEntry (devNet.Get (2)->GetAddress (), Ipv4Address ("172.30.0.14"));

  dhcpServerApp.Start (Seconds (0.0));
  dhcpServerApp.Stop (stopTime);

  // DHCP clients
  NetDeviceContainer dhcpClientNetDevs;
  dhcpClientNetDevs.Add (devNet.Get (0));
  dhcpClientNetDevs.Add (devNet.Get (1));
  dhcpClientNetDevs.Add (devNet.Get (2));

  ApplicationContainer dhcpClients = dhcpHelper.InstallDhcpClient (dhcpClientNetDevs);
  dhcpClients.Start (Seconds (1.0));
  dhcpClients.Stop (stopTime);

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (p2pNodes.Get (1));
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (stopTime);

  UdpEchoClientHelper echoClient (p2pInterfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (100));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (1));
  clientApps.Start (Seconds (10.0));
  clientApps.Stop (stopTime);

  Simulator::Stop (stopTime + Seconds (10.0));

  if (tracing)
    {
      csma.EnablePcapAll ("dhcp-csma");
      pointToPoint.EnablePcapAll ("dhcp-p2p");
    }

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
