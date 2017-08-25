#include "uep_decoder.hpp"

#include <limits>

using namespace std;

namespace uep {

uep_decoder::uep_decoder(const parameter_set &ps) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  Ks(ps.Ks), RFs(ps.RFs), EF(ps.EF),
  empty_queued_count(0),
  seqno_ctr(numeric_limits<uep_packet::seqno_type>::max()),
  tot_dec_count(0),
  tot_fail_count(0) {
  if (RFs.empty()) { // Using two-level UEP
    RFs = {ps.RFM, ps.RFL};
  }
  if (Ks.size() != RFs.size()) {
    throw std::invalid_argument("Ks, RFs sizes do not match");
  }

  out_queues.resize(Ks.size());

  std_dec = std::make_unique<lt_decoder>(block_size_in(), ps.c, ps.delta);

  seqno_ctr.set(0);
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
  std::size_t next_seqno = seqno_ctr.value();
  seqno_ctr.next();
  auto i = std::find_if(out_queues.begin(), out_queues.end(),
			[next_seqno](const queue_type &q){
			  return q.front().sequence_number() == next_seqno;
			});
  fountain_packet p;
  if (i != out_queues.end()) { // Otherwise empty fountain_packet
    uep_packet up = std::move(i->front());
    i->pop();
    p.buffer() = std::move(up.buffer());
    p.setPriority(up.priority());
  }
  else {
    --empty_queued_count;
  }
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

int uep_decoder::block_seed() const {
  return std_dec->block_seed();
}

std::size_t uep_decoder::received_count() const {
  return std_dec->received_count();
}

std::size_t uep_decoder::queue_size() const {
  return std::accumulate(out_queues.cbegin(),
			 out_queues.cend(),
			 0,
			 [](std::size_t sum,
			    const queue_type &iq) -> std::size_t {
			   return sum + iq.size();
			 }) + empty_queued_count;
}

bool uep_decoder::has_queued_packets() const {
  return empty_queued_count > 0 ||
    std::any_of(out_queues.cbegin(),
		out_queues.cend(),
		[](const queue_type &q){
		  return !q.empty();
		});
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

std::size_t uep_decoder::map_in2out(std::size_t i) {
  i %= block_size_in() / EF;
  std::size_t subblock = 0;
  std::size_t offset = 0;
  std::size_t out_offset = 0;
  while (RFs[subblock] * Ks[subblock] + offset <= i) {
    offset += RFs[subblock] * Ks[subblock];
    out_offset += Ks[subblock];
    ++subblock;
  }
  return (i - offset) % Ks[subblock] + out_offset;
}

void uep_decoder::deduplicate_queued() {
  using std::swap;

  std::vector<uep_packet> out_block(block_size_out());
  while (std_dec->has_queued_packets()) {
    std::size_t decoded = 0;
    for (std::size_t i = 0; i != block_size_in(); ++i) {
      packet p = std_dec->next_decoded();
      if (p.empty()) continue;
      if (decoded == out_block.size()) continue;

      std::size_t out_i = map_in2out(i);

      if (out_block[out_i].buffer().empty()) {
	uep_packet up = uep_packet::from_packet(p);
	out_block[out_i] = std::move(up);
	++decoded;
      }
    }
    tot_dec_count += decoded;
    tot_fail_count += out_block.size() - decoded;

    auto j = out_block.begin();
    for (std::size_t subblock = 0; subblock < Ks.size(); ++subblock) {
      for (std::size_t i = 0; i < Ks[subblock]; ++i) {
	uep_packet up;
	swap(up, *j++); // Make *i empty
	up.priority(subblock); // Set correct priority even on empty pacekts
	if (!up.buffer().empty()) // Do not insert empty packets: wrong seqno
	  out_queues[subblock].push(std::move(up));
	else
	  ++empty_queued_count;
      }
    }
  }
}

}
