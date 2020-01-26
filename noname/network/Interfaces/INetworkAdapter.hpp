#pragma once

#include <vector>
#include <string>

#include "../types.hpp"

namespace noname_core {
	namespace network {

		class INetworkAdapter
		{
		public:
			virtual ~INetworkAdapter() {}

			virtual bool install(std::string& filter) = 0;
			virtual bool uninstall() = 0;

			virtual int send_packet(packet data) = 0;
			virtual packet get_next_packet() = 0;

		protected:
			virtual bool device_ready() = 0;

		};
	}
}