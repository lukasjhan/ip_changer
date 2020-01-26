#pragma once

#include <windivert.h>
#include <Windows.h>

#include "interfaces/INetworkAdapter.hpp"

namespace noname_core {
	namespace network {

		class WindivertNetworkAdaper : public INetworkAdapter {

		public:
			WindivertNetworkAdaper() : handle(nullptr) {}
			~WindivertNetworkAdaper() {
				if (handle) uninstall();
			}

			bool install(std::string filter) override {
				handle = WinDivertOpen(filter.c_str(), WINDIVERT_LAYER_NETWORK, 0, 0);
			}
			bool uninstall() override {
				WinDivertClose(handle);
				handle = nullptr;
			}

			int send_packet(packet data) override {
				WindivertSend(
					handle,
					reinterpret_cast<void*>(data.data()),
					data.size(),
					nullptr,
					nullptr
					);
			}

			packet get_next_packet() override {
				byte buffer[MAXBUF];
				int packet_len = 0;

				WinDivertRecv(
					handle,
					buffer,
					sizeof buffer,
					&packet_len,
					nullptr
				);

				packet ret(packet_len);
				memcpy(ret.data(), buffer, packet_len);
				return ret;
			}

		private:

			HANDLE handle;

			bool device_ready() override {
				return handle;
			}
		};
	}
}