#include "block_encoder.hpp"

using namespace std;

namespace uep {

block_encoder::block_encoder(const lt_row_generator &rg) :
  block_encoder(std::make_unique<lt_row_generator>(rg)) {
}

block_encoder::block_encoder(std::unique_ptr<base_row_generator> &&rg) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  rowgen(std::move(rg)), out_count(0) {
  block.reserve(rowgen->K());
}



void block_encoder::set_seed(seed_t seed) {
  rowgen->reset(seed);
}

void block_encoder::reset() {
  rowgen->reset();
  block.clear();
  out_count = 0;
}

bool block_encoder::can_encode() const {
  return block.size() == rowgen->K();
}

block_encoder::seed_t block_encoder::seed() const {
  return rowgen->seed();
}

block_encoder::const_block_iterator block_encoder::block_begin() const {
  return block.cbegin();
}

block_encoder::const_block_iterator block_encoder::block_end() const {
  return block.cend();
}

std::size_t block_encoder::block_size() const {
  return rowgen->K();
}

lt_row_generator block_encoder::row_generator() const {
  return dynamic_cast<const lt_row_generator&>(*rowgen);
}

std::size_t block_encoder::output_count() const {
  return out_count;
}

packet block_encoder::next_coded() {
  if (!can_encode())
    throw std::logic_error("Does not have a block");
  base_row_generator::row_type row = rowgen->next_row();
  auto i = row.cbegin();
  packet first(block[*i++]);
  for (; i != row.cend(); ++i) {
    first ^= block[*i];
  }
  ++out_count;
  return first;
}

block_encoder::operator bool() const {
  return can_encode();
}

bool block_encoder::operator!() const {
  return !can_encode();
}

}
