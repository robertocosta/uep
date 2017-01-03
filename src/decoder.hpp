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
    return less(lhs.seqno(), rhs.seqno());
  }
private:
  std::less<std::uint_fast32_t> less;
};

class fountain_decoder {
public:
  typedef typename std::vector<fountain_packet>::const_iterator const_decoded_iterator;
  typedef typename std::set<fountain_packet>::const_iterator const_received_iterator;
  
  fountain_decoder(std::uint_fast32_t K);

  void push_coded(const fountain_packet &p);
  const_decoded_iterator decoded_begin() const;
  const_decoded_iterator decoded_end() const;

  bool has_decoded() const;
  std::uint_fast32_t K() const;
  std::uint_fast32_t blockno() const;

  const fountain &the_fountain() const;
  const_received_iterator received_packets_begin() const;
  const_received_iterator received_packets_end() const;

private:
  typedef bipartite_graph<fountain_packet> bg_type;
  typedef bipartite_graph<fountain_packet>::size_type bg_size_type;
  
  const std::uint_fast32_t K_;
  fountain fount;
  std::uint_fast32_t blockno_;
  std::set<fountain_packet, seqno_less> received_pkts;
  std::vector<fountain::row_type> original_connections;
  std::vector<fountain_packet> decoded;
  bg_type bg;

  void run_message_passing();
  void init_bg();
  void decode_degree_one(std::set<bg_size_type> &ripple);
  void process_ripple(const std::set<bg_size_type> &ripple);
};

#endif
