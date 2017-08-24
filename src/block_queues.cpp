#include "block_queues.hpp"

#include <utility>

using namespace std;

output_block_queue::output_block_queue(std::size_t block_size) :
  K(block_size) {}

const packet &output_block_queue::front() const {
  return output_queue.front();
}

packet &output_block_queue::front() {
  return output_queue.front();
}

void output_block_queue::pop() {
  output_queue.pop(); // does throw if empty?
}

void output_block_queue::clear() {
  typedef decltype(output_queue) queue_t;
  output_queue = queue_t();
}

bool output_block_queue::empty() const {
  return output_queue.empty();
}

std::size_t output_block_queue::size() const {
  return output_queue.size();
}

output_block_queue::operator bool() const {
  return !empty();
}

bool output_block_queue::operator!() const {
  return empty();
}
