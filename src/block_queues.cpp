#include "block_queues.hpp"

#include <utility>

using namespace std;

input_block_queue::input_block_queue(std::size_t block_size):
  K(block_size) {
  input_block.reserve(K);
}

void input_block_queue::push(packet &&p) {
  input_queue.push(move(p));
  check_has_block();
}

void input_block_queue::push(const packet &p) {
  packet p_copy(p);
  push(move(p_copy));
}

bool input_block_queue::has_block() const {
  return input_block.size() == K;
}

void input_block_queue::pop_block() {
  if (!has_block())
    throw std::logic_error("Cannot pop without a full block");
  input_block.clear();
  check_has_block();
}


input_block_queue::const_block_iterator input_block_queue::block_begin() const {
  if (!has_block()) throw logic_error("Does not have a block");
  return input_block.cbegin();
}

input_block_queue::const_block_iterator input_block_queue::block_end() const {
  if (!has_block()) throw logic_error("Does not have a block");
  return input_block.cend();
}

const packet &input_block_queue::block_at(std::size_t pos) const {
  if (!has_block()) throw logic_error("Does not have a block");
  return input_block.at(pos);
}

void input_block_queue::clear() {
  input_block.clear();
  typedef decltype(input_queue) queue_t;
  input_queue = queue_t();
}

std::size_t input_block_queue::block_size() const {
  return K;
}

std::size_t input_block_queue::queue_size() const {
  return input_queue.size();
}

std::size_t input_block_queue::size() const {
  return input_queue.size() + (has_block() ? K : 0);
}

void input_block_queue::check_has_block() {
  if (!has_block() && input_queue.size() >= K)
    for (size_t i = 0; i < K; ++i) {
      packet p;
      swap(p, input_queue.front());
      input_queue.pop();
      input_block.push_back(move(p));
    }
}

input_block_queue::operator bool() const {
  return has_block();
}

bool input_block_queue::operator!() const {
  return !has_block();
}

output_block_queue::output_block_queue(std::size_t block_size) :
  K(block_size) {}

const packet &output_block_queue::front() const {
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
