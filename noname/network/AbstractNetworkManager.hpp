#pragma once

#include <functional>

#include "Interfaces/INetworkManager.hpp"
#include "Interfaces/INetworkAdapter.hpp"

namespace noname_core {
	namespace network {
		class AbstractNetworkManager : public INetworkManager {

		public:
			AbstractNetworkManager(INetworkAdapter& adapter, std::function<int(packet&)> f) : adapter(adapter), modifier(f) {}
			virtual ~AbstractNetworkManager() {}

			void start() override {
				if (!run) {
					run = true;
					do_start();
				}
			}
			void stop() override {
				if (run) {
					run = false;
					do_stop();
				}
			}

			void set_modifier(std::function<std::pair<bool, int>(packet&)> f) override {
				modifier = f;
			}

			virtual void loop() = 0;

		protected:
			std::function<std::pair<bool, int>(packet&)> modifier;
			INetworkAdapter& adapter;
			bool run = false;

			virtual void do_start() = 0;
			virtual void do_stop() = 0;
		};
	}
}