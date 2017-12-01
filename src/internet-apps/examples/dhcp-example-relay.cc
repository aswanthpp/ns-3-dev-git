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

NS_LOG_COMPONENT_DEFINE ("DhcpExampleRelay");

int
main (int argc, char *argv[])
{
	bool verbose = false;
	bool tracing = false;
	Time stopTime = Seconds (20);

	CommandLine cmd;
	cmd.AddValue ("verbose", "turn on the logs", verbose);
	cmd.AddValue ("tracing", "turn on the tracing", tracing);

	cmd.Parse (argc, argv);

	// GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

	if (verbose)
		{
			LogComponentEnable ("DhcpServer", LOG_LEVEL_ALL);
			LogComponentEnable ("DhcpClient", LOG_LEVEL_ALL);
		  //LogComponentEnable ("DhcpRelay", LOG_LEVEL_ALL);
		}

	NS_LOG_INFO ("Create nodes.");
	NodeContainer nodes;
	NodeContainer relay;
	nodes.Create (3);
	relay.Create (1);

	NodeContainer net (nodes, relay);

	NS_LOG_INFO ("Create channels.");
	CsmaHelper csma;
	csma.SetChannelAttribute ("DataRate", StringValue ("5Mbps"));
	csma.SetChannelAttribute ("Delay", StringValue ("2ms"));
	csma.SetDeviceAttribute ("Mtu", UintegerValue (1500));
	NetDeviceContainer devNet = csma.Install (net);

	NodeContainer p2pNodes;
	p2pNodes.Add (net.Get (3));
	p2pNodes.Create (1);

	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

	NetDeviceContainer p2pDevices;
	p2pDevices = pointToPoint.Install (p2pNodes);

	InternetStackHelper tcpip;
	tcpip.Install (nodes);
	tcpip.Install (relay);
	tcpip.Install (p2pNodes.Get (1));

	NS_LOG_INFO ("Setup the IP addresses and create DHCP applications.");
	DhcpHelper dhcpHelper;

	Ipv4InterfaceContainer relayClient = dhcpHelper.InstallFixedAddress (devNet.Get (3), 
																																			Ipv4Address ("172.30.0.17"), 
																																			Ipv4Mask ("/24"));   

	// DHCP server
	ApplicationContainer dhcpServerApp = dhcpHelper.InstallDhcpServer (p2pDevices.Get (1), Ipv4Address ("172.30.1.12"),
																																		 Ipv4Address ("172.30.1.0"), Ipv4Mask ("/24"),
																																		 Ipv4Address ("172.30.1.10"), 
																																		 Ipv4Address ("172.30.1.15"));

	//DHCP Relay Agent
	ApplicationContainer dhcpRelayApp = dhcpHelper.InstallDhcpRelay (p2pDevices.Get (0), Ipv4Address ("172.30.1.16"),
																																	Ipv4Mask ("/24"), Ipv4Address ("172.30.1.12"));

	// This is just to show how it can be done.
	DynamicCast<DhcpServer> (dhcpServerApp.Get (0))->AddStaticDhcpEntry (devNet.Get (2)->GetAddress (), Ipv4Address ("172.30.1.14"));

	dhcpServerApp.Start (Seconds (0.0));
	dhcpServerApp.Stop (stopTime);

	dhcpRelayApp.Start (Seconds (0.0));
	dhcpRelayApp.Stop (stopTime);

	// DHCP clients
	NetDeviceContainer dhcpClientNetDevs;
	dhcpClientNetDevs.Add (devNet.Get (0));
	dhcpClientNetDevs.Add (devNet.Get (1));
	dhcpClientNetDevs.Add (devNet.Get (2));

	ApplicationContainer dhcpClients = dhcpHelper.InstallDhcpClient (dhcpClientNetDevs);
	dhcpClients.Start (Seconds (1.0));
	dhcpClients.Stop (stopTime);

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
