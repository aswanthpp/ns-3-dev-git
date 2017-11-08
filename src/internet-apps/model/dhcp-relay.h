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
    static const int PORT_CLIENT = 67;                       //!< Port number of DHCP client
    static const int PORT_SERVER = 68;                       //!< Port number of DHCP server
    /**
     * \brief Handles incoming packets from the network
     * \param socket Socket bound to port 67 of the DHCP server
     */
    void NetHandlerClient (Ptr<Socket> socket);
     void NetHandlerServer (Ptr<Socket> socket);

     /*unicast to server from relay DHCPDISCOVER*/
    void SendDiscover(Ptr<NetDevice> iDev, DhcpHeader header);

    /* unicast to server from relay DHCPREQUEST*/
    void SendReq(DhcpHeader header);

    /*broadcast offers to client */
    void SendOffer(DhcpHeader header);

    /* broadcast ack or nack to client*/
    void SendAckClient(DhcpHeader header);

     Ptr<Socket> m_socket_client; // socket communicating with client
     Ptr<Socket> m_sokcet_server;   // socket communicating with dhcp server   

    Ipv4Address m_relayAddress;            /// !<Address assigned to the relay>!
    Ipv4Address m_dhcps;

};

} 

#endif 


