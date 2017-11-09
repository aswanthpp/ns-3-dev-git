#include <bits/stdc++.h>
/*from dhcp-client.cc*/
#include "ns3/ipv4.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/uinteger.h"
#include "ns3/random-variable-stream.h"
#include "ns3/pointer.h"
#include "ns3/string.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "dhcp-relay.h"
#include "dhcp-header.h"
/*from dhcp-server.cc*/
#include "ns3/assert.h"
#include "ns3/ipv4-packet-info-tag.h"
#include "ns3/config.h"

/*Every class exported by the ns3 library is enclosed in the ns3 namespace*/
namespace ns3 { 

	/*Define a Log component with the name "DhcpRelay"*/
	NS_LOG_COMPONENT_DEFINE ("DhcpRelay"); 

	/*Register an Object subclass with the TypeId system.This macro should be
    invoked once for every class which defines a new GetTypeId method*/
	NS_OBJECT_ENSURE_REGISTERED (DhcpRelay); 

	/*Get the type ID(a unique identifier for an interface)*/
	TypeId DhcpRelay::GetTypeId (void) 
	{
		static TypeId tid = TypeId ("ns3::DhcpRelay")
		.SetParent<Application> ()
		.AddConstructor<DhcpRelay> ()
		.SetGroupName ("Internet-Apps")
		.AddAttribute ("RelayAddress",
                   "Pool of addresses to provide on request.",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&DhcpRelay::m_relayAddress),
                   MakeIpv4AddressChecker ())
		.AddAttribute ("DhcpServerAddress",
                   "Pool of addresses to provide on request.",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&DhcpRelay::m_dhcps),
                   MakeIpv4AddressChecker ())
		;
		return tid;
	}

	Ptr<NetDevice> DhcpClient::GetDhcpRelayNetDevice (void)
	{
	  return m_device;
	}

	void DhcpClient::SetDhcpRelayNetDevice (Ptr<NetDevice> netDevice)
	{
	  m_device = netDevice;
	}


	void DhcpRelay::StartApplication (void)	
	{	// m_socket_client  can only contact  dhcp server
		TypeId tid_client = TypeId::LookupByName ("ns3::UdpSocketFactory");
		m_socket_client = Socket::CreateSocket (GetNode (), tid_client);
		InetSocketAddress local_client = InetSocketAddress (Ipv4Address::GetAny (), PORT_SERVER);
		m_socket_client->SetAllowBroadcast (true);
		//m_socket_client->BindToNetDevice (ipv4->GetNetDevice (ifIndex));
		m_socket_client->Bind (local_client);
		m_socket_client->SetRecvPktInfo (true);
		m_socket_client->SetRecvCallback (MakeCallback (&DhcpRelay::NetHandlerClient, this));


		// m_socket_server -> can only contact dhcp client
		TypeId tid_server = TypeId::LookupByName ("ns3::UdpSocketFactory");
		m_socket_server = Socket::CreateSocket (GetNode (), tid_server);
		InetSocketAddress local_server = InetSocketAddress (m_relayAddress, PORT_CLIENT);
		m_socket_server->Bind (local_server);
		m_socket_server->SetRecvPktInfo (true);
		m_socket_server->SetRecvCallback (MakeCallback (&DhcpRelay::NetHandlerServer, this));
	}

	void DhcpRelay::StopApplication ()
	{
		//NS_LOG_FUNCTION (this);

		if (m_socket_client != 0)
			{
				m_socket_client->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
			}
		if (m_socket_client != 0)
		{
			m_socket_server->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
		}

		//m_leasedAddresses.clear ();
		//Simulator::Remove (m_expiredEvent);
	}

	void DhcpRelay::NetHandlerClient (Ptr<Socket> socket)
	{
		NS_LOG_FUNCTION (this << socket);

		DhcpHeader header; 
		Ptr<Packet> packet = 0;
		Address from;

		/*Read a single packet from the socket and retrieve the sender address*/
		packet = m_socket_client->RecvFrom (from);  

        /*Returns corresponding inetsocket address of argument*/
		InetSocketAddress senderAddr = InetSocketAddress::ConvertFrom (from);  

		Ipv4PacketInfoTag interfaceInfo;
    	/*True if the requested tag is found, false otherwise*/
		if (!packet->RemovePacketTag (interfaceInfo))   
			{
				NS_ABORT_MSG ("No incoming interface on DHCP message, aborting.");
			}

        /*Get the tag's receiving interface*/
		uint32_t incomingIf = interfaceInfo.GetRecvIf ();   

  		/*Pointer to the incoming interface*/
		Ptr<NetDevice> iDev = GetNode ()->GetDevice (incomingIf);   

        /*Deserialize and remove the header from the internal buffer*/
		if (packet->RemoveHeader (header) == 0)   
			{
				return;
			}

		if (header.GetType () == DhcpHeader::DHCPDISCOVER)
			{
				SendDiscover(iDev,header); 
			}

		if (header.GetType () == DhcpHeader::DHCPREQ)
			{
				
				SendReq(header,m_relayAddress);
			}

	}

	void DhcpRelay::NetHandlerServer(Ptr<Socket> socket)
	{
		NS_LOG_FUNCTION (this << socket);

		DhcpHeader header; 
		Ptr<Packet> packet = 0;
		Address from;

		/*Read a single packet from the socket and retrieve the sender address*/
		packet = m_socket_client->RecvFrom (from); 
		InetSocketAddress senderAddr = InetSocketAddress::ConvertFrom (from);  
		Ipv4PacketInfoTag interfaceInfo;
    	/*True if the requested tag is found, false otherwise*/
		if (!packet->RemovePacketTag (interfaceInfo))   
			{
				NS_ABORT_MSG ("No incoming interface on DHCP message, aborting.");
			}

        /*Get the tag's receiving interface*/
		uint32_t incomingIf = interfaceInfo.GetRecvIf ();   

  		/*Pointer to the incoming interface*/
		Ptr<NetDevice> iDev = GetNode ()->GetDevice (incomingIf);   

        /*Deserialize and remove the header from the internal buffer*/
		if (packet->RemoveHeader (header) == 0)   
			{
				return;
			}

		if (header.GetType () == DhcpHeader::DHCPOFFER)
			{
				SendOffer(iDev,header);
			}

		if (header.GetType () == DhcpHeader::DHCPACK || header.GetType () == DhcpHeader::DHCPNACK)
			{
				SendAckClient(header);
			}

	}

	void DhcpRelay::SendDiscover(Ptr<NetDevice> iDev,DhcpHeader header)
	{
		Ptr<Packet> packet = Create<Packet> ();
		header.SetGiaddr(DynamicCast<Ipv4>(iDev.GetAddress ())); 
		header.SetMask(24);  //  assumed that every subnetworks has /24 poolMask

		packet->AddHeader (header);

		if ((m_socket_server->SendTo (packet, 0, InetSocketAddress (m_dhcps, PORT_SERVER))) >= 0)
			{
				NS_LOG_INFO ("DHCP DISCOVER send to server");
			}
		else
			{
				NS_LOG_INFO ("Error while sending DHCP DISCOVER to server");
			}
	}

	void DhcpRelay::SendOffer(DhcpHeader header)
	{
		NS_LOG_FUNCTION (this << header);
		
		packet = Create<Packet> ();
		packet->AddHeader (header);
		
		if ((m_socket_client->SendTo (packet, 0, InetSocketAddress (Ipv4Address ("255.255.255.255"), from.GetPort ()))) >= 0)
			{
				NS_LOG_INFO ("DHCP OFFER Sent to Client");
		          // Send data to a specified peer. 
		         // -1 in case of error or the number of bytes copied in the internal buffer and accepted for transmission. 
			}
		else
			{
				NS_LOG_INFO ("Error while sending DHCP OFFER");
			}
	    // header.getGiAddr() return the router interface.
	    // broadcast forward packet to client

	}

	void DhcpRelay::SendReq(DhcpHeader header)
	{		

		// header.getGiAddr()  return the router interface
		// broadcast this header to client 

		NS_LOG_FUNCTION (this);

		DhcpHeader header;
		Ptr<Packet> packet;

		packet->AddHeader(header);
		if(m_socket_client->SendTo (packet, 0, InetSocketAddress (m_dhcps, DHCP_PEER_PORT)) >= 0);
			{
				NS_LOG_INFO ("DHCP REQUEST sent from Relay agent to Client");
			}
		else
			{
				NS_LOG_INFO("Error while sending DHCPREQ to" << m_dhcps);
			}	    
	}

	void DhcpRelay::SendAckClient(DhcpHeader header)
	{
		Ptr<Packet> packet = Create<Packet> ();

		packet->AddHeader (header);

		if ((m_socket_server->SendTo (packet, 0, InetSocketAddress (Ipv4Address ("255.255.255.255"), PORT_CLIENT))) >= 0)
			{
				NS_LOG_INFO ("DHCP ACK send to client");
			}
		else
			{
				NS_LOG_INFO ("Error while sending DHCP ACK to client");
			}
	}

}