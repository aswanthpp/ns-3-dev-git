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
                   "relay address",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&DhcpRelay::m_relayAddress),
                   MakeIpv4AddressChecker ())
		.AddAttribute ("DhcpServerAddress",
                   "dhcpServer Address",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&DhcpRelay::m_dhcps),
                   MakeIpv4AddressChecker ())
		.AddAttribute ("SubnetMask",
                   "Mask of the subnet",
                   Ipv4MaskValue (),
                   MakeIpv4MaskAccessor (&DhcpRelay::m_subMask),
                   MakeIpv4MaskChecker ())
		;
		return tid;
	}

DhcpRelay::DhcpRelay ()
{
  NS_LOG_FUNCTION (this);
}

DhcpRelay::~DhcpRelay ()
{
  NS_LOG_FUNCTION (this);
}
void
DhcpRelay::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

	Ptr<NetDevice> DhcpRelay::GetDhcpRelayNetDevice (void)
	{
	  return m_device;
	}

	void DhcpRelay::SetDhcpRelayNetDevice (Ptr<NetDevice> netDevice)
	{
	  m_device = netDevice;
	}


	void DhcpRelay::StartApplication (void)	
	{
		NS_LOG_FUNCTION (this);
		// m_socket_client  can only contact  dhcp server
		TypeId tid_client = TypeId::LookupByName ("ns3::UdpSocketFactory");
		m_socket_client = Socket::CreateSocket (GetNode (), tid_client);
		InetSocketAddress local_client = InetSocketAddress (Ipv4Address::GetAny (), PORT_SERVER);
		m_socket_client->SetAllowBroadcast (true);
		//m_socket_client->BindToNetDevice (ipv4->GetNetDevice (ifIndex));
		m_socket_client->Bind (local_client);
		m_socket_client->SetRecvPktInfo (true);
		m_socket_client->SetRecvCallback (MakeCallback (&DhcpRelay::NetHandlerServer, this));
       

		// m_socket_server -> can only contact dhcp client
		TypeId tid_server = TypeId::LookupByName ("ns3::UdpSocketFactory");
		m_socket_server = Socket::CreateSocket (GetNode (), tid_server);
		InetSocketAddress local_server = InetSocketAddress (m_relayAddress, PORT_CLIENT);
		m_socket_server->SetAllowBroadcast (true);

		m_socket_server->Bind (local_server);
		m_socket_server->SetRecvPktInfo (true);
		m_socket_server->SetRecvCallback (MakeCallback (&DhcpRelay::NetHandlerClient, this));
	}

	void DhcpRelay::StopApplication ()
	{
		NS_LOG_FUNCTION (this);

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

	void DhcpRelay::NetHandlerServer (Ptr<Socket> socket)
	{
		NS_LOG_FUNCTION (this << socket);
// NS_LOG_INFO("-----------------------------------------------");
		DhcpHeader header; 
		Ptr<Packet> packet = 0;
		Address from;
//  NS_LOG_INFO("----------111111111111111-------------------------------------");
		/*Read a single packet from the socket and retrieve the sender address*/
		packet = m_socket_client->RecvFrom (from);  

        /*Returns corresponding inetsocket address of argument*/
		//InetSocketAddress senderAddr = InetSocketAddress::ConvertFrom (from);  
// NS_LOG_INFO("------------------222222222222222222----------------------------");
		Ipv4PacketInfoTag interfaceInfo;
    	/*True if the requested tag is found, false otherwise*/
		if (!packet->RemovePacketTag (interfaceInfo))   
			{
				NS_ABORT_MSG ("No incoming interface on DHCP message, aborting.");
			}
 //NS_LOG_INFO("----------3333333333333333333333-------------------------------------");
        /*Get the tag's receiving interface*/
		uint32_t incomingIf = interfaceInfo.GetRecvIf ();   

  		/*Pointer to the incoming interface*/
		Ptr<NetDevice> iDev = GetNode ()->GetDevice (incomingIf); 
		//  NS_LOG_INFO("-------------444444444444444444444----------------------------------");  

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
				
				SendReq(header);
			}
		//	  NS_LOG_INFO("------------endendend-----------------------------------");

	}

	void DhcpRelay::NetHandlerClient(Ptr<Socket> socket)
	{
		NS_LOG_FUNCTION (this << socket);
        //NS_LOG_INFO("-----------------------------------------------");
		DhcpHeader header; 
		Ptr<Packet> packet = 0;
		Address from;

        //NS_LOG_INFO("----------111111111111111-------------------------------------");
 
		/*Read a single packet from the socket and retrieve the sender address*/
		packet = m_socket_server->RecvFrom (from); 

        //NS_LOG_INFO("------------------222222222222222222----------------------------");

		//InetSocketAddress senderAddr = InetSocketAddress::ConvertFrom (from);  
		Ipv4PacketInfoTag interfaceInfo;
    	/*True if the requested tag is found, false otherwise*/
		if (!packet->RemovePacketTag (interfaceInfo))   
			{
				NS_ABORT_MSG ("No incoming interface on DHCP message, aborting.");
			}
      //  NS_LOG_INFO("----------3333333333333333333333-------------------------------------");

        /*Get the tag's receiving interface*/
		uint32_t incomingIf = interfaceInfo.GetRecvIf ();   
       // NS_LOG_INFO("-------------444444444444444444444----------------------------------");

  		/*Pointer to the incoming interface*/
		Ptr<NetDevice> iDev = GetNode ()->GetDevice (incomingIf); 

     //   NS_LOG_INFO("---------------------555555555555555555555--------------------------");


        /*Deserialize and remove the header from the internal buffer*/
		if (packet->RemoveHeader (header) == 0)   
			{
				return;
			}

		if (header.GetType () == DhcpHeader::DHCPOFFER)
			{
				SendOffer(header);
			}

		if (header.GetType () == DhcpHeader::DHCPACK || header.GetType () == DhcpHeader::DHCPNACK)
			{
				SendAckClient(header);
			}
       // NS_LOG_INFO("------------endendend-----------------------------------");


	}

	void DhcpRelay::SendDiscover(Ptr<NetDevice> iDev,DhcpHeader header)
	{
		NS_LOG_FUNCTION (this<<header);
		// NS_LOG_INFO("-----------------------------------------------");
		Ptr<Packet> packet = 0;
		packet = Create<Packet> ();
		DhcpHeader newDhcpHeader;
		uint32_t tran=header.GetTran();
		Address sourceChaddr = header.GetChaddr ();
		uint32_t mask=header.GetMask();
		//Ipv4Address giAddr=header.GetGiaddr(); 

		newDhcpHeader.ResetOpt ();
		newDhcpHeader.SetType (DhcpHeader::DHCPDISCOVER);
		newDhcpHeader.SetTran (tran);
  		newDhcpHeader.SetChaddr (sourceChaddr);
  		newDhcpHeader.SetTime ();
  		newDhcpHeader.SetGiaddr("172.30.0.17"); /// need to add programmitically
  		
  		//packet->AddHeader (newHeader);
		//newDhcpHeader.SetGiaddr(DynamicCast<Ipv4>(iDev.GetAddress ())); 
		newDhcpHeader.SetMask(mask);  //  assumed that every subnetworks has /24 poolMask
		// NS_LOG_INFO("-------------------------111111111----------------------");
		packet->AddHeader (newDhcpHeader);
		//packet->AddHeader (header);
		 //NS_LOG_INFO("-----------------22222222222------------------------------");

		if ((m_socket_server->SendTo (packet, 0, InetSocketAddress (m_dhcps, PORT_SERVER))) >= 0)
			{
				NS_LOG_INFO ("DHCP DISCOVER send to server");
			}
		else
			{
				NS_LOG_INFO ("Error while sending DHCP DISCOVER to server");
			}
			// NS_LOG_INFO("----------------endendend-------------------------------");
	}

	void DhcpRelay::SendOffer(DhcpHeader header)
	{
		NS_LOG_FUNCTION (this << header);
		Ptr<Packet> packet = 0;
		packet = Create<Packet> ();
		/*DhcpHeader newDhcpHeader;
		uint32_t tran=header.GetTran();
		Address sourceChaddr = header.GetChaddr ();
		uint32_t mask=header.GetMask();
		Ipv4Address offeredAddress=header.GetYiaddr();
		Ipv4Address dhcpServerAddress=header.GetDhcps();
*/
		//newDhcpHeader.ResetOpt ();
		//newDhcpHeader.SetType (DhcpHeader::DHCPOFFER);
		//newDhcpHeader.SetTran (tran);
  		//newDhcpHeader.SetChaddr (sourceChaddr);
  		//newDhcpHeader.SetTime ();
  		//newDhcpHeader.SetMask(mask);


		//header.SetMask(24);

		//packet->AddHeader (newDhcpHeader);
		packet->AddHeader (header);
		
		if ((m_socket_client->SendTo (packet, 0, InetSocketAddress (Ipv4Address ("255.255.255.255"), PORT_CLIENT))) >= 0)
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
		NS_LOG_FUNCTION (this << header);
		// header.getGiAddr()  return the router interface
		// broadcast this header to client 

		NS_LOG_FUNCTION (this);
		Ptr<Packet> packet = 0;
		packet = Create<Packet> ();

		header.SetMask(24);

		packet->AddHeader(header);

		if(m_socket_server->SendTo (packet, 0, InetSocketAddress (m_dhcps, PORT_SERVER)) >= 0)
			{
				NS_LOG_INFO ("DHCP REQUEST sent from Relay agent to Server");
			}
		else
			{
				NS_LOG_INFO("Error while sending DHCPREQ to" << m_dhcps);
			}	    
	}

	void DhcpRelay::SendAckClient(DhcpHeader header)
	{
		NS_LOG_FUNCTION (this<<header);
		Ptr<Packet> packet = 0;
		packet = Create<Packet> ();

		header.SetMask(24);

		packet->AddHeader (header);

		if ((m_socket_client->SendTo (packet, 0, InetSocketAddress (Ipv4Address ("255.255.255.255"), PORT_CLIENT))) >= 0)
			{
				NS_LOG_INFO ("DHCP ACK send to client");
			}
		else
			{
				NS_LOG_INFO ("Error while sending DHCP ACK to client");
			}
	}

}