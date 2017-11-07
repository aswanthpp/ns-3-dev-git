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
  packet = m_socket->RecvFrom (from); //Read a single packet from the socket and retrieve the sender address. 

  InetSocketAddress senderAddr = InetSocketAddress::ConvertFrom (from);  // returns corresponding inetsocket address of argument

  Ipv4PacketInfoTag interfaceInfo;
  if (!packet->RemovePacketTag (interfaceInfo))  /// true if the requested tag is found, false otherwise. 
    {
      NS_ABORT_MSG ("No incoming interface on DHCP message, aborting.");
    }
  uint32_t incomingIf = interfaceInfo.GetRecvIf ();  /// Get the tag's receiving interface. 
  Ptr<NetDevice> iDev = GetNode ()->GetDevice (incomingIf);   // pointer to the incoming interface

  if (packet->RemoveHeader (header) == 0)  /// Deserialize and remove the header from the internal buffer. 
    {
      return;
    }
  if (header.GetType () == DhcpHeader::DHCPDISCOVER)
    {
      //SendOffer (iDev, header, senderAddr); // sends offer if it is of type DHCPDISCOVER

    	// instead I need to Do SendDiscover to known server
    	header.setGiAddr(DynamicCast<Ipv4>(iDev.GetAddress ())); // how to get incoming interface??
    	sendDiscover(header,m_relayAddress); // to do the function definition
    }
  if (header.GetType () == DhcpHeader::DHCPREQ && (header.GetReq ()).Get () >= m_minAddress.Get () && (header.GetReq ()).Get () <= m_maxAddress.Get ())
    {
      //SendAck (iDev, header, senderAddr);  // sends ack if it is of type DHCPREQUEST
    	//instead sendReq to Sevrer
    	header.setGiAddr(iDev.GetAddress ());
    	SendReq(header,m_relayAddress);
    }
   /* if (packet->RemoveHeader (header) == 0)
    {
      return;
    }
  if (header.GetChaddr () != m_chaddr)
    {
      return;
    }*/

  //if (m_state == WAIT_OFFER && header.GetType () == DhcpHeader::DHCPOFFER)
    if (header.GetType () == DhcpHeader::DHCPOFFER)
    {
      OfferHandler (header,m_relayAddress);
    }
  //if (m_state == WAIT_ACK && (header.GetType () == DhcpHeader::DHCPACK || header.GetType () == DhcpHeader::DHCPNACK))  
    // realy doesnot have  state no need to check that value
    if (header.GetType () == DhcpHeader::DHCPACK || header.GetType () == DhcpHeader::DHCPNACK)
    {
      //Simulator::Remove (m_nextOfferEvent);
     // AcceptAck (header,from);
      //instead forward to client
    	SendAckClient(header);
    }
  

}
void DhcpRelay::SendDiscover(DhcpHeader header,Ipv4Address relayAddress){
	 packet = Create<Packet> ();
	  packet->AddHeader (header); // adding dhcp header to packet

}
void DhcpRelay::OfferHandler(DhcpHeader header,Ipv4Address relayAddress){
	NS_LOG_FUNCTION (this << header);
// header.getGiAddr() return the router interface.
  // broadcast forward packet to client
	// 
}
void DhcpRelay::SendAckClient(DhcpHeader header,Ipv4Address relayAddress){
	// header.getGiAddr()  return the router interface
	// broadcast this header to client  
}
void DhcpRlay::SendReq(DhcpHeader header,Ipv4Address relayAddress){
	//sent the request to server unicast
packet = Create<Packet> ();
packet->AddHeader (header);
 if ((m_socket->SendTo (packet, 0, InetSocketAddress (m_dhcps, from.GetPort ()))) >= 0)
        {
          NS_LOG_INFO ("DHCP OFFER" << " Offered Address: " << offeredAddress);
          // Send data to a specified peer. 
         // -1 in case of error or the number of bytes copied in the internal buffer and accepted for transmission. 
        }
      else
        {
          NS_LOG_INFO ("Error while sending DHCP OFFER");
        }

}
	
	

	
	


}