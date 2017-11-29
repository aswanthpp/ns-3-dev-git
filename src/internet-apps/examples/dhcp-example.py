
# Python Code for dhcp-example.cc


import ns.applications
import ns.core
import ns.internet
import ns.network
import ns.point_to_point


# internet-apps module 
# csma- module 

ns.core.LogComponentEnable("UdpEchoClientApplication", ns.core.LOG_LEVEL_INFO)
ns.core.LogComponentEnable("UdpEchoServerApplication", ns.core.LOG_LEVEL_INFO)


nodes = ns.network.NodeContainer()
nodes.Create(3)
router = ns.network.NodeContainer()
router.Create(2)

net=ns.network.NodeContainer()
net(nodes,router)


# csma channel creation code needed

p2pNodes=ns.network.NodeContainer()
p2pNodes.Add (net.Get (4))
p2pNodes.Create (1)

pointToPoint = ns.point_to_point.PointToPointHelper()
pointToPoint.SetDeviceAttribute("DataRate", ns.core.StringValue("5Mbps"))
pointToPoint.SetChannelAttribute("Delay", ns.core.StringValue("2ms"))

p2pDevices = pointToPoint.Install(p2pNodes)

tcpip = ns.internet.InternetStackHelper()
tcpip.Install(nodes)
tcpip.Install(router)
tcpip.Install(p2pNodes.Get(1))

address = ns.internet.Ipv4AddressHelper()
address.SetBase(ns.network.Ipv4Address("172.30.1.0"),
                ns.network.Ipv4Mask("255.255.255.0"))

p2pInterfaces = address.Assign(p2pDevices)


