#pragma once

namespace noname_core {
	namespace network {

		using byte = uint8_t;
		using word = uint16_t;
		using dword = uint32_t;
		using qword = uint64_t;

		using packet = std::vector<byte>;

		enum class PacketType : int {
			Ethernet,
			ARP,
			RARP,
			IP,
			TCP,
			UDP,
			HTTP,
			UNKNOWN
		};
	}
}