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

		class tcpflowmanager : public AbstractNetworkManager {
			
		public:
			tcpflowmanager(INetworkAdapter& adapter, std::function<int(packet&)>& f) 
				: AbstractNetworkManager(adapter, f) {}

			~tcpflowmanager() {
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

					if (seq.count(key)) {
						tcp_p->tcp.set_seq_num(tcp_p->tcp.get_seq_num() + seq[key]);
						change_flag = true;
					}
					if (ack.count(key)) {
						tcp_p->tcp.set_ack_num(tcp_p->tcp.get_ack_num() + ack[key]);
						change_flag = true;
					}

					if (!tcp_p->tcp.get_flags() & (tcp_header::fin | tcp_header::ack)) // fin ack
						fin.insert(key);
					
					if (!tcp_p->tcp.get_flags() & tcp_header::ack)  // ack for finish
						if (fin.count(key) && fin.count(key.reverse())) 
							clear_flow(key);
					
					if (modifier) {
						
						auto mod = modifier(data);
						
						if (mod.first) { // packet data modified
							int mod_len = mod.second;
							
							if (!seq.count(key)) {
								seq[key] = mod_len;
								ack[key.reverse()] = -mod_len;
							}
							else {
								seq[key] += mod_len;
								ack[key.reverse()] -= mod_len;
							}
						}
					}
					if (change_flag) calchecksum(data);
					adapter.send_packet(data);
				}
			}

		protected:
			std::map<tcpflowkey, int> seq, ack;
			std::set<tcpflowkey> fin;

			void do_start() override {
				adapter.install("tcp");
			}

			void do_stop() override {
				seq.clear();
				ack.clear();
				adapter.uninstall();
			}

			void calchecksum(packet& data) {
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

			void clear_flow(tcpflowkey& key) {
				if (seq.count(key)) seq.erase(key);
				if (seq.count(key.reverse())) seq.erase(key.reverse());
				if (ack.count(key)) ack.erase(key);
				if (ack.count(key.reverse())) ack.erase(key.reverse());
				if (fin.count(key)) fin.erase(key);
				if (fin.count(key.reverse())) fin.erase(key.reverse());
			}
		};
	}
}