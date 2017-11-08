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
		
		return tid;
	}

	void DhcpRelay::StartApplication (void)
	{
		TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket (GetNode (), tid);
		InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), PORT);
		m_socket->SetAllowBroadcast (true);
		m_socket->BindToNetDevice (ipv4->GetNetDevice (ifIndex));
		m_socket->Bind (local);
		m_socket->SetRecvPktInfo (true);
		m_socket->SetRecvCallback (MakeCallback (&DhcpServer::NetHandler, this));
	}

	void DhcpRelay::StopApplication ()
	{
		//NS_LOG_FUNCTION (this);

		if (m_socket != 0)
		{
			m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
		}

		//m_leasedAddresses.clear ();
		//Simulator::Remove (m_expiredEvent);
	}

	void DhcpRelay::NetHandler (Ptr<Socket> socket)
	{
		NS_LOG_FUNCTION (this << socket);

		DhcpHeader header; 
		Ptr<Packet> packet = 0;
		Address from;

		/*Read a single packet from the socket and retrieve the sender address*/
   		packet = m_socket->RecvFrom (from);  

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
		    SendDiscover(iDev,header,m_relayAddress); 
		}
		    
		if (header.GetType () == DhcpHeader::DHCPREQ)
		{
		    header.SetGiaddr(DynamicCast<Ipv4>(iDev.GetAddress ()));
		    SendReq(header,m_relayAddress);
		}

	    if (header.GetType () == DhcpHeader::DHCPOFFER)
	    {
	    	OfferHandler (header,m_relayAddress);
	    }

	    if (header.GetType () == DhcpHeader::DHCPACK || header.GetType () == DhcpHeader::DHCPNACK)
	    {
	      SendAckClient(header);
	    }
    }

	void DhcpRelay::SendDiscover(Ptr<NetDevice> iDev, DhcpHeader header, InetSocketAddress from){

		packet = Create<Packet> ();
		header.SetGiaddr(DynamicCast<Ipv4>(iDev.GetAddress ())); 

		packet->AddHeader (header);

		if ((m_socket->SendTo (packet, 0, InetSocketAddress (m_dhcps, DHCP_PEER_PORT))) >= 0)
		{
			NS_LOG_INFO ("DHCP DISCOVER send to server");
		}
		else
		{
			NS_LOG_INFO ("Error while sending DHCP DISCOVER to server");
		}
	}

	void DhcpRelay::OfferHandler(DhcpHeader header,InetSocketAddress from){
		NS_LOG_FUNCTION (this << header);
		packet = Create<Packet> ();
		 packet->AddHeader (header);
		 if ((m_socket->SendTo (packet, 0, InetSocketAddress (Ipv4Address ("255.255.255.255"), from.GetPort ()))) >= 0)
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
	void DhcpRelay::sendAckClient(DhcpHeader header,InetSocketAddress from){
		// header.getGiAddr()  return the router interface
		// broadcast this header to client  
	}


}