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

  /**
   * \brief Get the the NetDevice DHCP should work on
   * \return the NetDevice DHCP should work on
   */
  Ptr<NetDevice> GetDhcpRelayNetDevice (void);
  
	/**
   * \brief Set the NetDevice DHCP should work on
   * \param netDevice the NetDevice DHCP should work on
   */
  void SetDhcpRelayNetDevice (Ptr<NetDevice> netDevice); 

  /**
   * \brief Starts the DHCP Relay application
   */
  void StartApplication (void);

  /**
   * \brief Stops the DHCP Server application
   */   
  void StopApplication (void);
	  
	/**
   * \brief Get the IPv4Address of current DHCP server
   * \return Ipv4Address of current DHCP server
   */
  Ipv4Address GetDhcpServer (void);

	protected:
	  virtual void DoDispose (void);

	private:
		static const int PORT_CLIENT = 68;                 //!< relay acting as client
		static const int PORT_SERVER = 67;                 //!< relay acting as server
	
	/**
	 * \brief Handles incoming packets from the network
	 * \param socket Socket bound to port 67 of the DHCP server
	 */
	void NetHandlerClient (Ptr<Socket> socket);

	/**
	 * \brief Handles incoming packets from the network
	 * \param socket Socket bound to port 68 of the DHCP client
	 */
	void NetHandlerServer (Ptr<Socket> socket);

	/**
   * \brief Sends DHCP DISCOVER to server as a unicast message
   * \param iDev incoming NetDevice
   * \param header DHCP header of the received message
   */
	void SendDiscover(Ptr<NetDevice> iDev, DhcpHeader header);

	/**
   * \brief Sends DHCP REQUEST to server as a unicast message
   * \param header DHCP header of the received message
   */
	void SendReq(DhcpHeader header);

	/**
   * \brief Sends DHCP offer coming from server to client 
   * \param header DHCP header of the received message
   */
	void SendOffer(DhcpHeader header);

	/**
   * \brief Sends DHCP ACK coming from server to client after receiving Request
   * \param header DHCP header of the received message
   */
	void SendAckClient(DhcpHeader header);

	Ptr<Socket> m_socket_client; 					//!< socket bound to port 67
	Ptr<Socket> m_socket_server;    			//!< socket bound to port 68   
	Ptr<NetDevice> m_device;							//!< NetDevice pointer
	Ipv4Address m_relayAddress;           //!< Address assigned to the relay>!
	Ipv4Address m_dhcps;									//!< Address of the DHCP server
	Ipv4Mask m_subMask;  									//!< Mask of the Subnet
};

} // namespace ns3

#endif /* DHCP_RELAY_H */


