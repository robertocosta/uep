#include "uep_decoder.hpp"

namespace uep {

uep_decoder::uep_decoder(const parameter_set &ps) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  Ks(ps.Ks), RFs(ps.RFs), EF(ps.EF),
  tot_dec_count(0),
  tot_fail_count(0) {
  if (RFs.empty()) { // Using two-level UEP
    RFs = {ps.RFM, ps.RFL};
  }
  if (Ks.size() != RFs.size()) {
    throw std::invalid_argument("Ks, RFs sizes do not match");
  }

  std_dec = std::make_unique<lt_decoder>(block_size_in(), ps.c, ps.delta);
}

void uep_decoder::push(const fountain_packet &p) {
  std_dec->push(p);
  deduplicate_queued();
}

void uep_decoder::push(fountain_packet &&p) {
  std_dec->push(std::move(p));
  deduplicate_queued();
}

fountain_packet uep_decoder::next_decoded() {
  fountain_packet p(std::move(out_queue.front()));
  out_queue.pop();
  return p;
}

void uep_decoder::flush() {
  std_dec->flush();
  deduplicate_queued();
}

void uep_decoder::flush(std::size_t blockno_) {
  std_dec->flush(blockno_);
  deduplicate_queued();
}

void uep_decoder::flush_n_blocks(std::size_t n) {
  std_dec->flush_n_blocks(n);
  deduplicate_queued();
}

bool uep_decoder::has_decoded() const {
  return std_dec->has_decoded();
}

std::size_t uep_decoder::block_size() const {
  return std::accumulate(Ks.cbegin(), Ks.cend(), 0);
}

std::size_t uep_decoder::block_size_in() const {
  return EF * std::inner_product(Ks.cbegin(), Ks.cend(),
				 RFs.cbegin(), 0);
}

std::size_t uep_decoder::block_size_out() const {
  return block_size();
}

std::size_t uep_decoder::K() const {
  return block_size();
}

std::size_t uep_decoder::blockno() const {
  return std_dec->blockno();
}

circular_counter<std::size_t> uep_decoder::block_number_counter() const {
  return std_dec->block_number_counter();
}

 block_decoder::seed_t uep_decoder::block_seed() const {
  return std_dec->block_seed();
}

std::size_t uep_decoder::received_count() const {
  return std_dec->received_count();
}

std::size_t uep_decoder::queue_size() const {
  return out_queue.size();
}

bool uep_decoder::has_queued_packets() const {
  return !out_queue.empty();
}

std::size_t uep_decoder::total_received_count() const {
  return std_dec->total_received_count();
}

std::size_t uep_decoder::total_decoded_count() const {
  return tot_dec_count;
}

std::size_t uep_decoder::total_failed_count() const {
  return tot_fail_count;
}

double uep_decoder::average_push_time() const {
  return std_dec->average_push_time();
}

uep_decoder::operator bool() const {
  return has_queued_packets();
}

bool uep_decoder::operator!() const {
  return !has_queued_packets();
}

std::pair<std::size_t,std::size_t> uep_decoder::map_in2out(std::size_t i) {
  i %= block_size_in() / EF;
  std::size_t subblock = 0;
  std::size_t offset = 0;
  std::size_t out_offset = 0;
  while (RFs[subblock] * Ks[subblock] + offset <= i) {
    offset += RFs[subblock] * Ks[subblock];
    out_offset += Ks[subblock];
    ++subblock;
  }
  std::size_t out_i = (i - offset) % Ks[subblock] + out_offset;
  return std::make_pair(out_i, subblock);
}

void uep_decoder::deduplicate_queued() {
  while (std_dec->has_queued_packets()) {
    std::vector<fountain_packet> out_block(block_size_out());
    std::size_t decoded = 0;
    for (std::size_t i = 0; i != block_size_in(); ++i) {
      fountain_packet p(std_dec->next_decoded());
      if (p.empty()) continue;
      if (decoded == out_block.size()) continue;

      auto m_ip = map_in2out(i);
      std::size_t out_i = m_ip.first;
      std::size_t out_prio = m_ip.second;

      if (out_block[out_i].empty()) {
	p.setPriority(out_prio);
	out_block[out_i] = std::move(p);
	++decoded;
      }
    }
    tot_dec_count += decoded;
    tot_fail_count += out_block.size() - decoded;

    for (auto i = out_block.begin(); i != out_block.end(); ++i) {
      out_queue.push(std::move(*i));
    }
  }
}

}
