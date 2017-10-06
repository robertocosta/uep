#include "uep_decoder.hpp"

using namespace std;

namespace uep {

uep_decoder::uep_decoder(const parameter_set &ps) :
  uep_decoder(ps.Ks.begin(), ps.Ks.end(),
	      ps.RFs.begin(), ps.RFs.end(),
	      ps.EF,
	      ps.c,
	      ps.delta) {
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
  BOOST_LOG_SEV(basic_lg, log::trace) << "UEP: extract a packet."
				      << " queue_size=" << queue_size()
				      << " next_seqno=" << next_seqno;
  auto i = std::find_if(out_queues.begin(), out_queues.end(),
			[next_seqno](const queue_type &q){
			  return !q.empty() &&
			  q.front().sequence_number() == next_seqno;
			});
  fountain_packet p;
  if (i != out_queues.end()) { // Otherwise empty fountain_packet
    uep_packet &up = i->front();
    BOOST_LOG_SEV(basic_lg, log::trace) << "Found the packet with prio "
					<< up.priority();
    p.buffer() = std::move(up.buffer());
    p.setPriority(up.priority());
    i->pop();
  }
  else {
    if (empty_queued_count == 0) {
      throw std::runtime_error("Extracting from empty UEP decoder");
    }
    --empty_queued_count;
    BOOST_LOG_SEV(basic_lg, log::trace) << "Packet was lost. "
					<< "New empty queue count = "
					<< empty_queued_count;
  }
  seqno_ctr.next();
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
  const std::size_t next_seqno = seqno_ctr.value();
  if (empty_queued_count == 0) {
    return std::any_of(out_queues.cbegin(),
		       out_queues.cend(),
		       [next_seqno](const queue_type &q){
			 return !q.empty() &&
			   q.front().sequence_number() == next_seqno;
		       });
  }
  else {
    bool any_eq = false;
    bool all_geq = std::all_of(out_queues.begin(), out_queues.end(),
			       [&any_eq,next_seqno](const queue_type &q){
				 if (q.empty()) return false;
				 auto sn = q.front().sequence_number();
				 if (sn == next_seqno) any_eq = true;
				 if (sn >= next_seqno) return true;
				 return false;
			       });
    return any_eq || all_geq;
  }
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

    BOOST_LOG(perf_lg) << "uep_decoder::deduplicate_queued "
			<< "decoded=" << decoded
			<< "failed=" << out_block.size() - decoded;

    auto j = out_block.begin();
    for (std::size_t subblock = 0; subblock < Ks.size(); ++subblock) {
      for (std::size_t i = 0; i < Ks[subblock]; ++i) {
	uep_packet up;
	swap(up, *j++); // Make *j empty
	up.priority(subblock);
	if (!up.buffer().empty()) // Do not insert empty packets: wrong seqno
	  out_queues[subblock].push(std::move(up));
	else
	  ++empty_queued_count;
      }
    }
    BOOST_LOG_SEV(basic_lg, log::trace) << "UEP: empty queued packets = "
					<< empty_queued_count;
  }
}

}
