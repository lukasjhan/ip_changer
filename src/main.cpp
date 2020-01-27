#include "noname/network/network.hpp"

using namespace noname_core::network;

int main(int argc, char** argv)
{
	if (argc != 2) return -1;

	std::string ip = argv[1];

	WindivertNetworkAdaper adapter;
	ipflowmanager manager(adapter, ip_address(ip));

	manager.start();
	manager.loop();

	return 0;
}