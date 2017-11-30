# Extension ​of DHCPv4​​ implementation​ ​in​ ​ns-3​ ​to​ ​support​ ​DHCP​ ​(or​ ​BOOTP)​ ​Relay
## Course Code : CO300
## Assignment :  #14 

### Overview

The Dynamic Host Configuration Protocol (DHCP) provides configuration parameters to Internet hosts. DHCP consists of two components: a protocol for delivering host-specific configuration parameters from a DHCP server to a host and a mechanism for allocation of network addresses to hosts. However, it is not feasible to have a server on each subnet.

Using BOOTP relay agents eliminates the necessity of having a DHCP server on each physical network segment. A BOOTP relay agent or relay agent is an Internet host or router that passes DHCP messages between DHCP clients and DHCP servers. DHCP is designed to use the same relay agent behavior as specified in the BOOTP protocol specification.

### Procedure
* We have implemented the DHCP relay feature in ns-3 using the topology as shown :

![image](https://user-images.githubusercontent.com/19391965/33449818-ed16ec8e-d62f-11e7-936d-e8d46f9f97b9.png)

### References


[1]  RFC​ ​ 2131:​ ​ Dynamic​ ​ Host​ ​ Configuration​ ​ Protocol​ ​ ( https://tools.ietf.org/html/rfc2131​ )

[2]  DHCP​ ​ module​ ​ for​ ​ IPv4​ ​ in​ ​ ns-3​ ​ ( src/internet-apps/model )
