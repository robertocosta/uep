#ifndef DECODER_HPP
#define DECODER_HPP

#include "bipartite_graph.hpp"
#include "packets.hpp"
#include "rng.hpp"

#include <set>
#include <vector>

class seqno_less {
public:
  bool operator()(const fountain_packet &lhs, const fountain_packet &rhs) {
    return less(lhs.sequence_number(), rhs.sequence_number());
  }
private:
  std::less<int> less;
};

class fountain_decoder {
public:
  explicit fountain_decoder(const degree_distribution &distr);
  explicit fountain_decoder(const fountain &f);

  void push_coded(const fountain_packet &p);
  void push_coded(fountain_packet &&p);
  
  std::vector<packet>::const_iterator decoded_begin() const;
  std::vector<packet>::const_iterator decoded_end() const;

  bool has_decoded() const;
  int K() const;
  int blockno() const;
  int block_seed() const;

  fountain the_fountain() const;
  std::set<fountain_packet>::const_iterator received_packets_begin() const;
  std::set<fountain_packet>::const_iterator received_packets_end() const;

private:
  typedef bipartite_graph<fountain_packet> bg_type;
  typedef bipartite_graph<fountain_packet>::size_type bg_size_type;
  
  fountain fount;
  int blockno_;
  int block_seed_;
  std::set<fountain_packet, seqno_less> received_pkts;
  std::vector<fountain::row_type> original_connections;
  std::vector<packet> decoded;

  bg_type bg;
  int bg_decoded_count;

  void run_message_passing();
  void init_bg();
  void decode_degree_one(std::set<bg_size_type> &ripple);
  void process_ripple(const std::set<bg_size_type> &ripple);
};

#endif
