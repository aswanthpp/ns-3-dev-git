#ifndef DHCP_RELAY_H
#define DHCP_RELAY_H

/*from dhcp-server.h*/
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-value.h"
#include "ns3/inet-socket-address.h"
#include "dhcp-header.h"
#include <map>
/*from dhcp-client.h*/
#include "ns3/ipv4-address.h"
#include "ns3/random-variable-stream.h"
#include <list>

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup dhcp
 *
 * \class DhcpRelay
 * \brief Implements the functionality of a DHCP server
 */
class DhcpRelay : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  DhcpRelay ();
  virtual ~DhcpRelay ();

  /*Get the IPv4Address of current DHCP server*/
  Ipv4Address GetDhcpServer (void);

protected:
  virtual void DoDispose (void);

private:
  static const int PORT = 67;                       //!< Port number of DHCP server

  /**
   * \brief Handles incoming packets from the network
   * \param socket Socket bound to port 67 of the DHCP server
   */
  void NetHandler (Ptr<Socket> socket);

   /*unicast to server from relay DHCPDISCOVER*/
  void SendDiscover(DhcpHeader header,Ipv4Address relayAddress);

  /* unicast to server from relay DHCPREQUEST*/
  void SendReq(DhcpHeader header,Ipv4Address relayAddress);

  /*broadcast offers to client */
  void OfferHandler(DhcpHeader header,Ipv4Address relayAddress);

  /* broadcast ack or nack to client*/
  void SendAckClient(DhcpHeader header,Ipv4Address relayAddress);








  /**
   * \brief Sends DHCP offer after receiving DHCP Discover
   * \param iDev incoming NetDevice
   * \param header DHCP header of the received message
   * \param from Address of the DHCP client
   */
  void SendOffer (Ptr<NetDevice> iDev, DhcpHeader header, InetSocketAddress from);



  /**
   * \brief Sends DHCP ACK (or NACK) after receiving Request
   * \param iDev incoming NetDevice
   * \param header DHCP header of the received message
   * \param from Address of the DHCP client
   */
  void SendAck (Ptr<NetDevice> iDev, DhcpHeader header, InetSocketAddress from);

  /**
   * \brief Modifies the remaining lease time of addresses
   */
  void TimerHandler (void);

  /**
   * \brief Starts the DHCP Server application
   */
  virtual void StartApplication (void);

  /**
   * \brief Stops the DHCP client application
   */
  virtual void StopApplication (void);

  Ptr<Socket> m_socket;                  //!< The socket bound to port 67
  Ipv4Address m_gateway;                 //!< The gateway address

    enum States
  {
    WAIT_OFFER = 1,             //!< State of a client that waits for the offer
    REFRESH_LEASE = 2,          //!< State of a client that needs to refresh the lease
    WAIT_ACK = 9                //!< State of a client that waits for acknowledgment
  };

  static const int DHCP_PEER_PORT = 67; //!< DHCP server port

  /**
   * \brief Handles changes in LinkState
   */
  void LinkStateHandler (void);

  /**
   * \brief Handles incoming packets from the network
   * \param socket Socket bound to port 68 of the DHCP client
   */
  void NetHandler (Ptr<Socket> socket);

  /**
   * \brief Sends DHCP DISCOVER and changes the client state to WAIT_OFFER
   */
  //void Boot (void);
  void SendDiscover()

  /**
   * \brief Stores DHCP offers in m_offerList
   * \param header DhcpHeader of the DHCP OFFER message
   */
  void OfferHandler (DhcpHeader header);

  /**
   * \brief Selects an OFFER from m_offerList
   */
  void Select (void);

  /**
   * \brief Sends the DHCP REQUEST message and changes the client state to WAIT_ACK
   */
  void Request (void);

  /**
   * \brief Receives the DHCP ACK and configures IP address of the client.
   *        It also triggers the timeout, renew and rebind events.
   * \param header DhcpHeader of the DHCP ACK message
   * \param from   Address of DHCP server that sent the DHCP ACK
   */
  void AcceptAck (DhcpHeader header, Address from);

  /**
   * \brief Remove the current DHCP information and restart the process
   */
  void RemoveAndStart ();

  uint8_t m_state;                       //!< State of the DHCP client
  Ptr<NetDevice> m_device;               //!< NetDevice pointer
  Ptr<Socket> m_socket;                  //!< Socket for remote communication
  Ipv4Address m_remoteAddress;           //!< Initially set to 255.255.255.255 to start DHCP
  Ipv4Address m_offeredAddress;          //!< Address offered to the client
  Ipv4Address m_myAddress;               //!< Address assigned to the client
  Address m_chaddr;                      //!< chaddr of the interface (stored as an Address for convenience).
  Ipv4Mask m_myMask;                     //!< Mask of the address assigned
  Ipv4Address m_server;                  //!< Address of the DHCP server
  Ipv4Address m_gateway;                 //!< Address of the gateway
  EventId m_requestEvent;                //!< Address refresh event
  EventId m_discoverEvent;               //!< Message retransmission event
  EventId m_refreshEvent;                //!< Message refresh event
  EventId m_rebindEvent;                 //!< Message rebind event
  EventId m_nextOfferEvent;              //!< Message next offer event
  EventId m_timeout;                     //!< The timeout period
  Time m_lease;                          //!< Store the lease time of address
  Time m_renew;                          //!< Store the renew time of address
  Time m_rebind;                         //!< Store the rebind time of address
  Time m_nextoffer;                      //!< Time to try the next offer (if request gets no reply)
  Ptr<RandomVariableStream> m_ran;       //!< Uniform random variable for transaction ID
  Time m_rtrs;                           //!< Defining the time for retransmission
  Time m_collect;                        //!< Time for which client should collect offers
  bool m_offered;                        //!< Specify if the client has got any offer
  std::list<DhcpHeader> m_offerList;     //!< Stores all the offers given to the client
  uint32_t m_tran;                       //!< Stores the current transaction number to be used
  TracedCallback<const Ipv4Address&> m_newLease;//!< Trace of new lease
  TracedCallback<const Ipv4Address&> m_expiry;  //!< Trace of lease expire

  Ipv4Address m_relayAddress;            /// !<Address assigned to the relay>!


};

} // namespace ns3

#endif /* DHCP_RELAY_H */


