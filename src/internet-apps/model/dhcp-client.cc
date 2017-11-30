#include <stdlib.h>
#include <stdio.h>
#include <list>

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
#include "dhcp-client.h"
#include "dhcp-header.h"

/*Every class exported by the ns3 library is enclosed in the ns3 namespace*/
namespace ns3 { 

/*Define a Log component with the name "DhcpClient"*/
NS_LOG_COMPONENT_DEFINE ("DhcpClient"); 

/*Register an Object subclass with the TypeId system.This macro should be
  invoked once for every class which defines a new GetTypeId method.*/
NS_OBJECT_ENSURE_REGISTERED (DhcpClient); 

/*Get the type ID(a unique identifier for an interface)*/
TypeId DhcpClient::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::DhcpClient")
    .SetParent<Application> ()
    .AddConstructor<DhcpClient> ()
    .SetGroupName ("Internet-Apps")
    .AddAttribute ("RTRS", "Time for retransmission of Discover message",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&DhcpClient::m_rtrs),
                   MakeTimeChecker ())
    .AddAttribute ("Collect", "Time for which offer collection starts",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&DhcpClient::m_collect),
                   MakeTimeChecker ())
    .AddAttribute ("ReRequest", "Time after which request will be resent to next server",
                   TimeValue (Seconds (10)),
                   MakeTimeAccessor (&DhcpClient::m_nextoffer),
                   MakeTimeChecker ())
    .AddAttribute ("Transactions",
                   "The possible value of transaction numbers ",
                   StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000000.0]"),
                   MakePointerAccessor (&DhcpClient::m_ran),
                   MakePointerChecker<RandomVariableStream> ())
    .AddTraceSource ("NewLease",
                     "Get a NewLease",
                     MakeTraceSourceAccessor (&DhcpClient::m_newLease),
                     "ns3::Ipv4Address::TracedCallback")
    .AddTraceSource ("ExpireLease",
                     "A lease expires",
                     MakeTraceSourceAccessor (&DhcpClient::m_expiry),
                     "ns3::Ipv4Address::TracedCallback");
  return tid;
}

DhcpClient::DhcpClient ()
{
  NS_LOG_FUNCTION_NOARGS ();

  /*Sets address of the DHCP server as 0.0.0.0*/
  m_server = Ipv4Address::GetAny ();

  /*Socket for remote communication*/
  m_socket = 0;

  /*m_refreshEvent : Message refresh event. 
  EventId () - An identifier for simulation events. */
  m_refreshEvent = EventId ();
  m_requestEvent = EventId ();
  m_discoverEvent = EventId ();
  m_rebindEvent = EventId ();
  m_nextOfferEvent = EventId ();
  m_timeout = EventId ();
}

DhcpClient::DhcpClient (Ptr<NetDevice> netDevice)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_device = netDevice;
  m_server = Ipv4Address::GetAny ();
  m_socket = 0;
  m_refreshEvent = EventId ();
  m_requestEvent = EventId ();
  m_discoverEvent = EventId ();
  m_rebindEvent = EventId ();
  m_nextOfferEvent = EventId ();
  m_timeout = EventId ();
}

DhcpClient::~DhcpClient ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

Ptr<NetDevice> DhcpClient::GetDhcpClientNetDevice (void)
{
  return m_device;
}

void DhcpClient::SetDhcpClientNetDevice (Ptr<NetDevice> netDevice)
{
  m_device = netDevice;
}

Ipv4Address DhcpClient::GetDhcpServer (void)
{
  return m_server;
}

void
DhcpClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_device = 0;

  Application::DoDispose ();
}

int64_t
DhcpClient::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_ran->SetStream (stream);
  return 1;
}

void
DhcpClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  m_remoteAddress = Ipv4Address ("255.255.255.255");
  m_myAddress = Ipv4Address ("0.0.0.0");
  m_gateway = Ipv4Address ("0.0.0.0");
  Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4> (); //GetNode () : Returns the Node to which this 
                                                   //Application object is attached. 
                                                   //GetObject<Ipv4> () : Get a pointer to the requested 
                                                   //aggregated Object. 
  
  uint32_t ifIndex = ipv4->GetInterfaceForDevice (m_device); //Returns the interface number of an Ipv4 interface 
                                                             //or -1 if not found

  // We need to cleanup the type from the stored chaddr, or later we'll fail to compare it.
  // Moreover, the length is always 16, because chaddr is 16 bytes.
  Address myAddress = m_device->GetAddress ();
  NS_LOG_INFO ("My address is " << myAddress);
  uint8_t addr[Address::MAX_SIZE]; //Address::MAX_SIZE - The maximum size of a byte buffer which can be stored 
                                   //in an Address instance (20)
  
  //void * memset ( void * ptr, int value, size_t num );
  //Sets the first num bytes of the block of memory pointed by ptr to the specified value.
  std::memset (addr, 0, Address::MAX_SIZE); 

  /*Copy myAddress to addr. Returns the number of bytes copied */
  uint32_t len = myAddress.CopyTo (addr);
  NS_ASSERT_MSG (len <= 16, "DHCP client can not handle a chaddr larger than 16 bytes");
  m_chaddr.CopyFrom (addr, 16);
  NS_LOG_INFO ("My m_chaddr is " << m_chaddr);

  bool found = false;
  /*GetNAddresses returns the number of Ipv4InterfaceAddresses stored on this interface*/
  for (uint32_t i = 0; i < ipv4->GetNAddresses (ifIndex); i++)
    {
      if (ipv4->GetAddress (ifIndex, i).GetLocal () == m_myAddress) //m_myAddress = 0.0.0.0
        {
          found = true;
        }
    }

  if (!found)
    {
      ipv4->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("0.0.0.0"),Ipv4Mask ("/0")));
    }

  //m_socket - Socket for remote communication  
  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory"); //Get a TypeId by the name "ns3::UdpSocketFactory"
      m_socket = Socket::CreateSocket (GetNode (), tid);
      
      /*an Inet address class : this class holds an Ipv4Address and 
      a port number to form an ipv4 transport endpoint. */
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 68);
      m_socket->SetAllowBroadcast (true);

      /*Bind a socket to specific device. If set on a socket, this option will force packets to leave the bound 
      device regardless of the device that IP routing would naturally choose. In the receive direction,
      only packets received from the bound interface will be delivered.*/
      m_socket->BindToNetDevice (m_device);
      m_socket->Bind (local);
    }

  /*SetRecvCallback is intended to notify a socket, that would have been blocked in a blocking socket model, 
  that data is available to be read. 
  DhcpClient::NetHandler - Handles incoming packets from the network. */  
  m_socket->SetRecvCallback (MakeCallback (&DhcpClient::NetHandler, this));

  /*DhcpClient::LinkStateHandler - Handles changes in LinkState.*/
  m_device->AddLinkChangeCallback (MakeCallback (&DhcpClient::LinkStateHandler, this));

  /*Sends DHCP DISCOVER and changes the client state to WAIT_OFFER. */
  Boot ();

}

void
DhcpClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  /*Simulator::Remove - Remove an event from the scheduled events list*/
  Simulator::Remove (m_discoverEvent);
  Simulator::Remove (m_requestEvent);
  Simulator::Remove (m_rebindEvent);
  Simulator::Remove (m_refreshEvent);
  Simulator::Remove (m_timeout);
  Simulator::Remove (m_nextOfferEvent);
  Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4> ();

  int32_t ifIndex = ipv4->GetInterfaceForDevice (m_device);
  for (uint32_t i = 0; i < ipv4->GetNAddresses (ifIndex); i++)
    {
      if (ipv4->GetAddress (ifIndex,i).GetLocal () == m_myAddress)
        {
          ipv4->RemoveAddress (ifIndex, i);
          break;
        }
    }

  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  m_socket->Close ();
}

void DhcpClient::LinkStateHandler (void)
{
  NS_LOG_FUNCTION (this);

  if (m_device->IsLinkUp ())
    {
      NS_LOG_INFO ("Link up at " << Simulator::Now ().As (Time::S));
      m_socket->SetRecvCallback (MakeCallback (&DhcpClient::NetHandler, this));
      StartApplication ();
    }
  else
    {
      NS_LOG_INFO ("Link down at " << Simulator::Now ().As (Time::S)); //reinitialization
      Simulator::Remove (m_refreshEvent); //stop refresh timer!!!!
      Simulator::Remove (m_rebindEvent);
      Simulator::Remove (m_timeout);
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());  //stop receiving on this socket !!!

      Ptr<Ipv4> ipv4MN = GetNode ()->GetObject<Ipv4> ();
      int32_t ifIndex = ipv4MN->GetInterfaceForDevice (m_device);

      for (uint32_t i = 0; i < ipv4MN->GetNAddresses (ifIndex); i++)
        {
          if (ipv4MN->GetAddress (ifIndex,i).GetLocal () == m_myAddress)
            {
              ipv4MN->RemoveAddress (ifIndex, i);
              break;
            }
        }

      Ipv4StaticRoutingHelper ipv4RoutingHelper;

      /*GetStaticRouting() - Try and find the static routing protocol as either the main routing protocol or in the
      list of routing protocols associated with the Ipv4 provided*/
      Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4MN);

      uint32_t i;

      /*GetNRoutes () - Get the number of individual unicast routes that have been added to the routing table.*/
      for (i = 0; i < staticRouting->GetNRoutes (); i++)
        {
          if (staticRouting->GetRoute (i).GetGateway () == m_gateway)
            {
              staticRouting->RemoveRoute (i);
              break;
            }
        }
    }
}

/*NetHandler - Handles incoming packets from the network*/
void DhcpClient::NetHandler (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Address from;					

  /*RecvFrom() - Read a single packet from the socket and retrieve the sender address*/
  Ptr<Packet> packet = m_socket->RecvFrom (from);
  DhcpHeader header;

  /*Deserialize and remove the header from the internal buffer. Returns the no. of bytes removed from the packet*/
  if (packet->RemoveHeader (header) == 0)
    {
      return;
    }

  /*GetChaddr () - Get the Address(16-bytes long) of the client. 
    m_chaddr - chaddr of the interface (stored as an Address for convenience). */  
  if (header.GetChaddr () != m_chaddr)
    {
      return;
    }
   // NS_LOG_INFO("-----------before offerhandler-------------------");


  if (m_state == WAIT_OFFER && header.GetType () == DhcpHeader::DHCPOFFER)
    {
  //    NS_LOG_INFO("-----------inside--------------");
      OfferHandler (header);
    //  NS_LOG_INFO("-----------inside----Afer offer--------------");

    }

  if (m_state == WAIT_ACK && header.GetType () == DhcpHeader::DHCPACK)
    {
      Simulator::Remove (m_nextOfferEvent);
      AcceptAck (header,from);
    }

  if (m_state == WAIT_ACK && header.GetType () == DhcpHeader::DHCPNACK)
    {
      Simulator::Remove (m_nextOfferEvent);
      Boot ();
    }
}

void DhcpClient::Boot (void)
{
  NS_LOG_FUNCTION (this);

  DhcpHeader header;
  Ptr<Packet> packet;
  packet = Create<Packet> ();

  /*Reset the BOOTP options*/
  header.ResetOpt ();

  /*m_tran stores the current transaction number to be used. */
  m_tran = (uint32_t) (m_ran->GetValue ());
  header.SetTran (m_tran);
  header.SetType (DhcpHeader::DHCPDISCOVER);

  /*Set the time when message is sent*/
  header.SetTime ();
  header.SetChaddr (m_chaddr);
  packet->AddHeader (header);

  /*SendTo(packet,flag,toAddress)
    InetSocketAddress(ipv4 address,port number)*/
  if ((m_socket->SendTo (packet, 0, InetSocketAddress (Ipv4Address ("255.255.255.255"), DHCP_PEER_PORT))) >= 0)
    {
      NS_LOG_INFO ("DHCP DISCOVER sent" );
    }
  else
    {
      /*m_remoteAddress is initially set to 255.255.255.255 to start DHCP. */
      NS_LOG_INFO ("Error while sending DHCP DISCOVER to " << m_remoteAddress);
    }

  m_state = WAIT_OFFER;
  m_offered = false;

  /*Simulator::Schedule (delay, mem_ptr, obj) - Schedule an event to expire after delay.
    m_rtrs defines the time for retransmission */
  m_discoverEvent = Simulator::Schedule (m_rtrs, &DhcpClient::Boot, this);
}

/*Stores DHCP offers in m_offerList*/
void DhcpClient::OfferHandler (DhcpHeader header)
{
  NS_LOG_FUNCTION (this << header);
  NS_LOG_INFO("--------------------------");

  m_offerList.push_back (header);
  if (m_offered == false)
    {
      Simulator::Remove (m_discoverEvent);
      m_offered = true;

      /*m_collect - Time for which client should collect offers*/
      Simulator::Schedule (m_collect, &DhcpClient::Select, this);
    }
}

void DhcpClient::Select (void)
{
  NS_LOG_FUNCTION (this);

  if (m_offerList.empty ())
    {
      Boot ();
      return;
    }

  DhcpHeader header = m_offerList.front ();
  m_offerList.pop_front ();
  m_lease = Time (Seconds (header.GetLease ()));
  m_renew = Time (Seconds (header.GetRenew ()));
  m_rebind = Time (Seconds (header.GetRebind ()));
  m_offeredAddress = header.GetYiaddr ();
  m_myMask = Ipv4Mask (header.GetMask ());
  m_server = header.GetDhcps ();
  m_gateway = header.GetRouter ();
  m_offerList.clear ();
  m_offered = false;
  Request ();
}

void DhcpClient::Request (void)
{
  NS_LOG_FUNCTION (this);

  DhcpHeader header;
  Ptr<Packet> packet;
  if (m_state != REFRESH_LEASE)
    {
      packet = Create<Packet> ();
      header.ResetOpt ();
      header.SetType (DhcpHeader::DHCPREQ);
      header.SetTime ();
      header.SetTran (m_tran);
      header.SetReq (m_offeredAddress);
      header.SetChaddr (m_chaddr);
      packet->AddHeader (header);
      m_socket->SendTo (packet, 0, InetSocketAddress (Ipv4Address ("255.255.255.255"), DHCP_PEER_PORT));
      m_state = WAIT_ACK;

      /*m_nextoffer - Time to try the next offer (if request gets no reply)*/
      m_nextOfferEvent = Simulator::Schedule (m_nextoffer, &DhcpClient::Select, this);
    }
  else
    {
      /*m_myAddress - Address assigned to the client*/
      uint32_t addr = m_myAddress.Get ();
      packet = Create<Packet> ((uint8_t*)&addr, sizeof(addr));
      header.ResetOpt ();
      m_tran = (uint32_t) (m_ran->GetValue ());
      header.SetTran (m_tran);
      header.SetTime ();
      header.SetType (DhcpHeader::DHCPREQ);

      /*Set the Ipv4Address requested by the client*/
      header.SetReq (m_myAddress);
      m_offeredAddress = m_myAddress;
      header.SetChaddr (m_chaddr);
      packet->AddHeader (header);

      /*m_remoteAddress has the address of the current DHCP server*/
      if ((m_socket->SendTo (packet, 0, InetSocketAddress (m_remoteAddress, DHCP_PEER_PORT))) >= 0)
        {
          NS_LOG_INFO ("DHCP REQUEST sent");
        }
      else
        {
          NS_LOG_INFO ("Error while sending DHCP REQ to " << m_remoteAddress);
        }
        
      m_state = WAIT_ACK;
    }
}

void DhcpClient::AcceptAck (DhcpHeader header, Address from)
{
  NS_LOG_FUNCTION (this << header << from);

  Simulator::Remove (m_rebindEvent);
  Simulator::Remove (m_refreshEvent);
  Simulator::Remove (m_timeout);
  NS_LOG_INFO ("DHCP ACK received");
  Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4> ();
  int32_t ifIndex = ipv4->GetInterfaceForDevice (m_device);

  for (uint32_t i = 0; i < ipv4->GetNAddresses (ifIndex); i++)
    {
      if (ipv4->GetAddress (ifIndex,i).GetLocal () == m_myAddress)
        {
          NS_LOG_LOGIC ("Got a new address, removing old one: " << m_myAddress);
          ipv4->RemoveAddress (ifIndex, i);
          break;
        }
    }

  ipv4->AddAddress (ifIndex, Ipv4InterfaceAddress (m_offeredAddress, m_myMask));
  ipv4->SetUp (ifIndex);

  /*InetSocketAddress::ConvertFrom() - Returns an InetSocketAddress which corresponds to the input Address
    DHCP_PEER_PORT : DHCP server port*/
  InetSocketAddress remote = InetSocketAddress (InetSocketAddress::ConvertFrom (from).GetIpv4 (), DHCP_PEER_PORT);

  m_socket->Connect (remote);
  if (m_myAddress != m_offeredAddress)
    {
      m_newLease (m_offeredAddress);
      if (m_myAddress != Ipv4Address ("0.0.0.0"))
        {
          m_expiry (m_myAddress);
        }
    }
    
  m_myAddress = m_offeredAddress;
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4);
  if (m_gateway == Ipv4Address ("0.0.0.0"))
    {
      m_gateway = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
    }

  staticRouting->SetDefaultRoute (m_gateway, ifIndex, 0);

  m_remoteAddress = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
  NS_LOG_INFO ("Current DHCP Server is " << m_remoteAddress);

  m_offerList.clear ();
  m_refreshEvent = Simulator::Schedule (m_renew, &DhcpClient::Request, this);
  m_rebindEvent = Simulator::Schedule (m_rebind, &DhcpClient::Request, this);
  m_timeout =  Simulator::Schedule (m_lease, &DhcpClient::RemoveAndStart, this);
  m_state = REFRESH_LEASE;
}

void DhcpClient::RemoveAndStart ()
{
  NS_LOG_FUNCTION (this);

  Simulator::Remove (m_nextOfferEvent);
  Simulator::Remove (m_refreshEvent);
  Simulator::Remove (m_rebindEvent);
  Simulator::Remove (m_timeout);

  Ptr<Ipv4> ipv4MN = GetNode ()->GetObject<Ipv4> ();
  int32_t ifIndex = ipv4MN->GetInterfaceForDevice (m_device);

  for (uint32_t i = 0; i < ipv4MN->GetNAddresses (ifIndex); i++)
    {
      if (ipv4MN->GetAddress (ifIndex,i).GetLocal () == m_myAddress)
        {
          ipv4MN->RemoveAddress (ifIndex, i);
          break;
        }
    }
  m_expiry (m_myAddress);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4MN);
  uint32_t i;
  for (i = 0; i < staticRouting->GetNRoutes (); i++)
    {
      if (staticRouting->GetRoute (i).GetGateway () == m_gateway)
        {
          staticRouting->RemoveRoute (i);
          break;
        }
    }
  StartApplication ();
}

} // Namespace ns3
