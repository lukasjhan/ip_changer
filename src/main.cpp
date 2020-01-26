#include "noname/network/network.hpp"

using namespace noname_core::network;

int main(int argc, char** argv)
{
	if (argc != 3) return -1;

	char* s = argv[1];
	int s_len = strlen(s);
	char* d = argv[2];
	int d_len = strlen(d);

	WindivertNetworkAdaper adapter;
	ipflowmanager manager(adapter, ip_address("192.168.0.4"));

	manager.start();
	manager.loop();

	return 0;
}