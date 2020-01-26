#pragma once

#include <functional>
#include <tuple>

#include "../types.hpp"

namespace noname_core {
	namespace network {

		class INetworkManager {

		public:
			~INetworkManager() {}

			virtual void start() = 0;
			virtual void stop() = 0;
			virtual void loop() = 0;

			virtual void set_modifier(std::function<std::pair<bool, int>(packet&)> f) = 0;
		};
	}
}