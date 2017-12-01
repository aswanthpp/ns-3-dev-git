# Extension ​of DHCPv4​​ implementation​ ​in​ ​ns-3​ ​to​ ​support​ ​DHCP​ ​(or​ ​BOOTP)​ ​Relay
## Course Code : CO300
## Assignment :  #14 

### Overview

The Dynamic Host Configuration Protocol (DHCP) provides configuration parameters to Internet hosts. DHCP consists of two components: a protocol for delivering host-specific configuration parameters from a DHCP server to a host and a mechanism for allocation of network addresses to hosts. However, it is not feasible to have a server on each subnet.

Using BOOTP relay agents eliminates the necessity of having a DHCP server on each physical network segment. A BOOTP relay agent or relay agent is an Internet host or router that passes DHCP messages between DHCP clients and DHCP servers. DHCP is designed to use the same relay agent behavior as specified in the BOOTP protocol specification.

### Procedure
* We have implemented the DHCP relay feature in ns-3 using the topology as shown :


![dhcp_relay](https://user-images.githubusercontent.com/19391965/33467526-0cf53c04-d67b-11e7-9d75-a60b47a0768c.png)


### To run the code

Clone the repository to your local machine.

`git clone https://github.com/aswanthpp/dhcp_relay_in_ns3.git`

Then build ns-3.

`./waf configure`

An example program for DHCP Relay has been provided in :

`src/internet-apps/examples/dhcp-example-relay.cc`

Once ns-3 has been built, we can run the example file as :

`./waf --run "src/internet-apps/examples/dhcp-example-relay --verbose=true"`

### References


[1]  RFC​ ​ 2131:​ ​ Dynamic​ ​ Host​ ​ Configuration​ ​ Protocol​ ​ ( https://tools.ietf.org/html/rfc2131​ )

[2]  DHCP​ ​ module​ ​ for​ ​ IPv4​ ​ in​ ​ ns-3​ ​ ( src/internet-apps/model )
