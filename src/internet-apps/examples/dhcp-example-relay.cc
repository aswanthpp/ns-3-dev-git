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
  NodeContainer relay;
  
  /*Create 3 nodes and append pointers to them to the end of this NodeContainer, i.e.,nodes*/
  nodes.Create (3);
  relay.Create (1);

  /*Create a node container which is a concatenation of two input NodeContainers*/
  NodeContainer net (nodes, relay);

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
  p2pNodes.Add (net.Get (3));
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
  tcpip.Install (relay);
  tcpip.Install (p2pNodes.Get (1));

  NS_LOG_INFO ("Setup the IP addresses and create DHCP applications.");

  /*DhcpHelper - The helper class used to configure and install DHCP applications on nodes*/
  DhcpHelper dhcpHelper;

  // The router must have a fixed IP.
  /*Ipv4InterfaceContainer - holds a vector of std::pair of Ptr<Ipv4> and interface index*/
  Ipv4InterfaceContainer relayClient = dhcpHelper.InstallFixedAddress (devNet.Get (3), 
                                                                      Ipv4Address ("172.30.0.17"), 
                                                                      Ipv4Mask ("/24"));                                                                                  

  // DHCP server
  /*ApplicationContainer - holds a vector of ns3::Application pointers
  InstallDhcpServer(netDevice, serverAddr, poolAddr, poolMask, minAddr, maxAddr, gateway)*/
  ApplicationContainer dhcpServerApp = dhcpHelper.InstallDhcpServer (p2pDevices.Get (1), Ipv4Address ("172.30.1.12"),
                                                                     Ipv4Address ("172.30.0.0"), Ipv4Mask ("/24"),
                                                                     Ipv4Address ("172.30.0.10"), 
                                                                     Ipv4Address ("172.30.0.15"));

  ApplicationContainer dhcpRelayApp = dhcpHelper.InstallDhcpRelay (p2pDevices.Get (0), Ipv4Address ("172.30.1.11"),
                                                                  Ipv4Mask ("/24"), Ipv4Address ("172.30.1.12"));


  // This is just to show how it can be done.
  DynamicCast<DhcpServer> (dhcpServerApp.Get (0))->AddStaticDhcpEntry (devNet.Get (2)->GetAddress (), 
                           Ipv4Address ("172.30.0.14"));

  /*Start - This method simply iterates through the contained Applications and calls their Start() 
  methods with the provided Time*/
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

  /*UdpEchoServerHelper(port) - Create a server application which waits for input UDP packets and sends them 
  back to the original sender; port- the port the server will wait on for incoming packets*/
  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (p2pNodes.Get (1));
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (stopTime);

  /*UdpEchoClientHelper(ip, port) - Create an application which sends a UDP packet and waits for an echo of 
  this packet. 
  ip - The IP address of the remote udp echo server 
  port - The port number of the remote udp echo server */
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
