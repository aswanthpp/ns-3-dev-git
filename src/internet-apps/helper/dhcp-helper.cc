#include "dhcp-helper.h"
#include "ns3/dhcp-server.h"
#include "ns3/dhcp-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/ipv4.h"
#include "ns3/loopback-net-device.h"
#include "ns3/traffic-control-layer.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/log.h"

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("DhcpHelper");

	DhcpHelper::DhcpHelper ()
	{
	    /*SetTypeId : Set the TypeId of the Objects to be created by this factory*/
		m_clientFactory.SetTypeId (DhcpClient::GetTypeId ());
		m_serverFactory.SetTypeId (DhcpServer::GetTypeId ());
	}

	void DhcpHelper::SetClientAttribute (std::string name, const AttributeValue &value)
	{
		/*Set : Set an attribute to be set during construction*/
		m_clientFactory.Set (name, value);
	}

	void DhcpHelper::SetServerAttribute (std::string name, const AttributeValue &value)
	{
		m_serverFactory.Set (name, value);
	}

    /*ApplicationContainer - holds a vector of ns3::Application pointers*/
	ApplicationContainer DhcpHelper::InstallDhcpClient (Ptr<NetDevice> netDevice) const
	{
		return ApplicationContainer (InstallDhcpClientPriv (netDevice));
	}

	ApplicationContainer DhcpHelper::InstallDhcpClient (NetDeviceContainer netDevices) const
	{
		ApplicationContainer apps;
		for (NetDeviceContainer::Iterator i = netDevices.Begin (); i != netDevices.End (); ++i)
		{
			/*Add : Append the contents of another ApplicationContainer to the end of this container*/
			apps.Add (InstallDhcpClientPriv (*i));
		}
		return apps;
	}

	Ptr<Application> DhcpHelper::InstallDhcpClientPriv (Ptr<NetDevice> netDevice) const
	{
		/*GetNode() : Returns the node base class which contains this network interface*/
		Ptr<Node> node = netDevice->GetNode ();

		/*At runtime, in debugging builds, if this condition is not true, the program prints the message to 
		output and halts by calling std::terminate*/
		NS_ASSERT_MSG (node != 0, "DhcpClientHelper: NetDevice is not not associated with any node -> fail");

        /*GetObject - returns a pointer to the requested Object, or zero if it could not be found*/
		Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
		NS_ASSERT_MSG (ipv4, "DhcpHelper: NetDevice is associated"
			" with a node without IPv4 stack installed -> fail "
			"(maybe need to use InternetStackHelper?)");

        /*GetInterfaceForDevice - Returns the interface number of an Ipv4 interface or -1 if not found*/
		int32_t interface = ipv4->GetInterfaceForDevice (netDevice);
		if (interface == -1)
		{
			/*AddInterface - Returns the index of the Ipv4 interface added
			  netDevice - device to add to the list of Ipv4 interfaces which can be used as output interfaces 
			  during packet forwarding*/
			interface = ipv4->AddInterface (netDevice);
		}
		NS_ASSERT_MSG (interface >= 0, "DhcpHelper: Interface index not found");

        /*interface - the interface number of an Ipv4 interface 
          1 - routing metric (cost) associated to the underlying Ipv4 interface*/
		ipv4->SetMetric (interface, 1);

		/*SetUp - Set the interface into the "up" state. In this state, it is considered valid during Ipv4 
		  forwarding*/
		ipv4->SetUp (interface);

        /*Install the default traffic control configuration if the traffic control layer has been aggregated, 
          if this is not a loopback interface, and there is no queue disc installed already*/
		Ptr<TrafficControlLayer> tc = node->GetObject<TrafficControlLayer> ();

		/*DynamicCast - Cast a Ptr; LoopbackNetDevice - The desired type to cast to; netDevice - The type of the 
		  original Ptr
		  GetRootQueueDiscOnDevice - Used to get the root queue disc installed on netDevice*/
		if (tc && DynamicCast<LoopbackNetDevice> (netDevice) == 0 && tc->GetRootQueueDiscOnDevice (netDevice) == 0)
		{
			NS_LOG_LOGIC ("DhcpHelper - Installing default traffic control configuration");

			/*TrafficControlHelper::Default () - Returns a new TrafficControlHelper with a default configuration*/
			TrafficControlHelper tcHelper = TrafficControlHelper::Default ();

			/*Install - Returns a QueueDisc container with the queue discs installed on the devices*/
			tcHelper.Install (netDevice);
		}
		
        /*Create - Create an Object instance of the configured TypeId*/
		Ptr<DhcpClient> app = DynamicCast <DhcpClient> (m_clientFactory.Create<DhcpClient> ());

		/*SetDhcpClientNetDevice - Set the NetDevice DHCP should work on*/
		app->SetDhcpClientNetDevice (netDevice);

		/*AddApplication - Associate an Application to this Node*/
		node->AddApplication (app);

		return app;
	}

	ApplicationContainer DhcpHelper::InstallDhcpServer (Ptr<NetDevice> netDevice, Ipv4Address serverAddr,
		Ipv4Address poolAddr, Ipv4Mask poolMask, Ipv4Address minAddr, Ipv4Address maxAddr, Ipv4Address gateway)
	{
		m_serverFactory.Set ("PoolAddresses", Ipv4AddressValue (poolAddr));
		m_serverFactory.Set ("PoolMask", Ipv4MaskValue (poolMask));
		m_serverFactory.Set ("FirstAddress", Ipv4AddressValue (minAddr));
		m_serverFactory.Set ("LastAddress", Ipv4AddressValue (maxAddr));
		m_serverFactory.Set ("Gateway", Ipv4AddressValue (gateway));

		Ptr<Node> node = netDevice->GetNode ();
		NS_ASSERT_MSG (node != 0, "DhcpHelper: NetDevice is not not associated with any node -> fail");

		Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
		NS_ASSERT_MSG (ipv4, "DhcpHelper: NetDevice is associated"
			" with a node without IPv4 stack installed -> fail "
			"(maybe need to use InternetStackHelper?)");

		int32_t interface = ipv4->GetInterfaceForDevice (netDevice);
		if (interface == -1)
		{
			interface = ipv4->AddInterface (netDevice);
		}
		NS_ASSERT_MSG (interface >= 0, "DhcpHelper: Interface index not found");

		Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (serverAddr, poolMask);
		ipv4->AddAddress (interface, ipv4Addr);
		ipv4->SetMetric (interface, 1);
		ipv4->SetUp (interface);

		Ptr<TrafficControlLayer> tc = node->GetObject<TrafficControlLayer> ();
		if (tc && DynamicCast<LoopbackNetDevice> (netDevice) == 0 && tc->GetRootQueueDiscOnDevice (netDevice) == 0)
		{
			NS_LOG_LOGIC ("DhcpHelper - Installing default traffic control configuration");
			TrafficControlHelper tcHelper = TrafficControlHelper::Default ();
			tcHelper.Install (netDevice);
		}

		std::list<Ipv4Address>::iterator iter;

		/*Checking that the already fixed addresses are not in conflict with the pool
		m_fixedAddresses - list of fixed addresses already allocated*/
		for (iter=m_fixedAddresses.begin (); iter!=m_fixedAddresses.end (); iter ++)
		{
			if (iter->Get () >= minAddr.Get () && iter->Get () <= maxAddr.Get ())
			{
				NS_ABORT_MSG ("DhcpHelper: Fixed address can not conflict with a pool: " << *iter << " is in [" << minAddr << ",  " << maxAddr << "]");
			}
		}
		m_addressPools.push_back (std::make_pair (minAddr, maxAddr));

		Ptr<Application> app = m_serverFactory.Create<DhcpServer> ();
		node->AddApplication (app);
		return ApplicationContainer (app);
	}

    /*InstallFixedAddress - Assign a fixed IP addresses to a net device*/
	Ipv4InterfaceContainer DhcpHelper::InstallFixedAddress (Ptr<NetDevice> netDevice, Ipv4Address addr, Ipv4Mask mask)
	{
		Ipv4InterfaceContainer retval;

		Ptr<Node> node = netDevice->GetNode ();
		NS_ASSERT_MSG (node != 0, "DhcpHelper: NetDevice is not not associated with any node -> fail");

		Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
		NS_ASSERT_MSG (ipv4, "DhcpHelper: NetDevice is associated"
			" with a node without IPv4 stack installed -> fail "
			"(maybe need to use InternetStackHelper?)");

		int32_t interface = ipv4->GetInterfaceForDevice (netDevice);
		if (interface == -1)
		{
			interface = ipv4->AddInterface (netDevice);
		}
		NS_ASSERT_MSG (interface >= 0, "DhcpHelper: Interface index not found");

		Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (addr, mask);
		ipv4->AddAddress (interface, ipv4Addr);
		ipv4->SetMetric (interface, 1);
		ipv4->SetUp (interface);
		retval.Add (ipv4, interface);

		Ptr<TrafficControlLayer> tc = node->GetObject<TrafficControlLayer> ();
		if (tc && DynamicCast<LoopbackNetDevice> (netDevice) == 0 && tc->GetRootQueueDiscOnDevice (netDevice) == 0)
		{
			NS_LOG_LOGIC ("DhcpHelper - Installing default traffic control configuration");
			TrafficControlHelper tcHelper = TrafficControlHelper::Default ();
			tcHelper.Install (netDevice);
		}

		std::list<std::pair<Ipv4Address, Ipv4Address> >::iterator iter;
		for (iter=m_addressPools.begin (); iter!=m_addressPools.end (); iter ++)
		{
			if (addr.Get () >= iter->first.Get () && addr.Get () <= iter->second.Get ())
			{
				NS_ABORT_MSG ("DhcpHelper: Fixed address can not conflict with a pool: " << addr << " is in [" << iter->first << ",  " << iter->second << "]");
			}
		}
		m_fixedAddresses.push_back (addr);
		return retval;
	}

} 
