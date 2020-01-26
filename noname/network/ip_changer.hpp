#pragma once
#include <cstring>
#include <map>
#include <functional>
#include <set>

#include "AbstractNetworkManager.hpp"
#include "ethernet.hpp"
#include "ipv4.hpp"
#include "tcp.hpp"
#include "types.hpp"

namespace noname_core {
	namespace network {
		using port_address = word;

		namespace {
			struct tcpflowkey
			{
				ip_address sip;
				port_address sport;

				ip_address dip;
				port_address dport;

				tcpflowkey() {}
				tcpflowkey(ip_address sip, port_address sport, ip_address dip, port_address dport)
					: sip(sip)
					, sport(sport)
					, dip(dip)
					, dport(dport) {}

				bool operator< (const tcpflowkey& k) const {
					return memcmp(this, &k, sizeof(tcpflowkey)) < 0;
				}

				tcpflowkey reverse() { return tcpflowkey(dip, dport, sip, sport); }
			};

			struct tcp_packet {
				ethernet_header ether;
				ip_header ip;
				tcp_header tcp;
			};

			struct pseudo_header {
				ip_address sip;
				ip_address dip;
				byte reserved;
				byte proto;
				word tcp_len;

				pseudo_header(ip_address sip, ip_address dip, byte proto, word tcp_len)
					: sip(sip)
					, dip(dip)
					, proto(proto)
					, tcp_len(tcp_len) {}
			};
		}

		class ipflowmanager : public AbstractNetworkManager {

		public:
			ipflowmanager(INetworkAdapter& adapter, ip_address& proxy)
				: AbstractNetworkManager(adapter, nullptr)
				, proxy(proxy) {}

			~ipflowmanager() {
				if (run) adapter.uninstall();
			}

			void loop() override {
				if (!run) return;

				while (run) {

					packet data = adapter.get_next_packet();
					tcp_packet* tcp_p = reinterpret_cast<tcp_packet*>(data.data());
					bool change_flag = false;

					tcpflowkey key(
						tcp_p->ip.get_src_ip(),
						tcp_p->tcp.get_src_port(),
						tcp_p->ip.get_des_ip(),
						tcp_p->tcp.get_des_port()
					);

					if (tcp_p->tcp.get_des_port() == 80 || tcp_p->tcp.get_des_port() == 443) {
						if (!ip_table.count(tcp_p->tcp.set_src_port())) {
							ip_table[tcp_p->tcp.get_src_port()] = tcp_p->ip.get_des_ip();
						}
						tcp_p->ip.set_des_ip(proxy);
						change_flag = true;
					}
					else if (tcp_p->tcp.get_src_port() == 80 || tcp_p->tcp.get_src_port() == 443) {
						if (!ip_table.count(tcp_p->tcp.get_des_port())) {
							tcp_p->ip.set_src_ip(ip_table[tcp_p->tcp.get_des_port()]);
							change_flag = true;
						}
					}

					if (change_flag) {
						// change checksum, ip, tcp
						cal_ip_checksum(tcp_p->ip);
						cal_tcp_checksum(data);
					}

					adapter.send_packet(data);
				}
			}

		protected:
			ip_address proxy;
			std::map<port_address, ip_address> ip_table;

			void do_start() override {
				adapter.install("tcp");
			}

			void do_stop() override {
				adapter.uninstall();
			}

			void cal_ip_checksum(ip_header& data) {
				data.set_checksum(0);
				word ret = calculate((word*)&data, sizeof data);
				data.set_checksum(~ret);
			}

			void cal_tcp_checksum(packet& data) {
				tcp_packet* tcp_p = reinterpret_cast<tcp_packet*>(data.data());

				pseudo_header ph(
					tcp_p->ip.get_src_ip(),
					tcp_p->ip.get_des_ip(),
					tcp_p->ip.get_proto(), data.size() - sizeof(ethernet_header) - sizeof(ip_header)
				);

				word checksum = 0;
				word checksum_ph = calculate(reinterpret_cast<word*>(&ph), sizeof ph);

				tcp_p->tcp.set_check_sum(0);
				word checksum_tcp_seg = calculate(
					reinterpret_cast<word*>(data.data()[sizeof(ethernet_header) + sizeof(ip_header)]),
					data.size() - (sizeof(ethernet_header) + sizeof(ip_header))
				);

				word temp = checksum_ph + checksum_tcp_seg;
				if (temp < checksum_ph)
					checksum = temp + 1;
				else
					checksum = temp;

				tcp_p->tcp.set_check_sum(~checksum);
			}

			word calculate(word* p, int len)
			{
				register long sum;
				word oddbyte;
				word answer;

				sum = 0;
				while (len > 1) {
					sum += *p++;
					len -= 2;
				}
				if (len == 1) {
					oddbyte = 0;
					*((byte*)&oddbyte) = *(byte*)p;
					sum += oddbyte;
				}

				sum = (sum >> 16) + (sum & 0xffff);
				answer = sum + (sum >> 16);

				return answer;
			}
		};
	}
}