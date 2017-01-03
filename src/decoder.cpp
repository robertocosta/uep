#include "decoder.hpp"

#include <algorithm>
#include <stdexcept>

using namespace std;

fountain_decoder::fountain_decoder(uint_fast32_t K) :
  K_(K),
  fount(K),
  blockno_(0) {
}

void fountain_decoder::push_coded(const fountain_packet &p) {
  if (p.blockno() > blockno_) {
    received_pkts.clear();
    original_connections.clear();
    fount.reset();
    decoded.clear();
    bg.clear();
        
    blockno_ = p.blockno();
  }

  auto ins_res = received_pkts.insert(p);
  if (!ins_res.second) throw runtime_error("Duplicate packet");
  while (p.seqno() >= original_connections.size()) {
    fountain::row_type row = fount.next_row();
    original_connections.push_back(row);
  }

  if (received_pkts.size() >= K_ && !has_decoded()) {
    run_message_passing();
  }
}

fountain_decoder::const_decoded_iterator fountain_decoder::decoded_begin() const {
  return decoded.cbegin();
}

fountain_decoder::const_decoded_iterator fountain_decoder::decoded_end() const {
  return decoded.cend();
}

bool fountain_decoder::has_decoded() const {
  return decoded.size() == K_;
}

std::uint_fast32_t fountain_decoder::K() const {
  return K_;
}

std::uint_fast32_t fountain_decoder::blockno() const {
  return blockno_;
}

const fountain &fountain_decoder::the_fountain() const {
  return fount;
}

fountain_decoder::const_received_iterator
fountain_decoder::received_packets_begin() const {
  return received_pkts.cbegin();
}

fountain_decoder::const_received_iterator
fountain_decoder::received_packets_end() const {
    return received_pkts.cend();
}

void fountain_decoder::init_bg() {
  bg.clear();
  bg.resize_input(K_);
  bg.resize_output(received_pkts.size());
  for (auto i = received_pkts.begin(); i != received_pkts.end(); ++i) {
    if (i->seqno() >= bg.output_size()) {
      bg.resize_output(i->seqno()+1);
    }      
    bg.output_at(i->seqno()) = i->clone();
    const fountain::row_type &conns = original_connections[i->seqno()];
    bg.add_links_to_output(i->seqno(), conns.begin(), conns.end());
  }
}

void fountain_decoder::decode_degree_one(std::set<bg_size_type> &ripple) {
  ripple.clear();
  
  std::vector<std::pair<bg_size_type, bg_size_type> > delete_conns;
  for (auto i = bg.output_degree_one_begin();
       i != bg.output_degree_one_end();
       ++i) {
    bg_size_type out = *i;
    bg_size_type in = bg.find_input(out);
    auto ins_ret = ripple.insert(in);
    if (ins_ret.second) {
      bg.input_at(in) = bg.output_at(out);
    }
    delete_conns.push_back(std::pair<bg_size_type, bg_size_type>(in, out));
  }

  for (auto i = delete_conns.begin(); i != delete_conns.end(); ++i) {
    bg.remove_link(i->first, i->second);
  }
}

void fountain_decoder::run_message_passing() {
  init_bg();

  std::set<bg_size_type> ripple;
  for (;;) {
    decode_degree_one(ripple);
    if (ripple.empty()) break;
    process_ripple(ripple);
  }

  std::vector<fountain_packet> dec;
  dec.reserve(K_);
  for (auto i = bg.input_begin(); i != bg.input_end(); ++i) {
    if (!i->has_data()) return;
    dec.push_back(*i);
  }
  swap(decoded, dec);
}

void fountain_decoder::process_ripple(const std::set<bg_size_type> &ripple) {
  for (auto i = ripple.cbegin(); i != ripple.cend(); ++i) {
    bg_size_type in = *i;
    std::set<std::pair<bg_size_type, bg_size_type> > delete_conns;
    auto out_range = bg.find_all_outputs(in);
    for (; out_range.first != out_range.second; ++out_range.first) {
      bg_size_type out = *(out_range.first);
      bg.output_at(out) ^= bg.input_at(in);
      delete_conns.insert(std::pair<bg_size_type, bg_size_type>(in, out));
    }

    for (auto i = delete_conns.begin(); i != delete_conns.end(); ++i) {
      bg.remove_link(i->first, i->second);
    }
  }
}
