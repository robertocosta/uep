#include "packets.hpp"
#include "rw_utils.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include <boost/numeric/conversion/cast.hpp>

using namespace std;
using namespace uep;

namespace uep {

buffer_type &operator^=(buffer_type &lhs, const buffer_type &rhs) {
  inplace_xor(lhs,rhs);
  return lhs;
}

buffer_type operator^(const buffer_type &lhs, const buffer_type &rhs) {
  buffer_type res(lhs);
  return res ^= rhs;
}

}

packet::packet() :
  shared_data(new vector<char>()) {}

packet::packet(const buffer_type &b) : shared_data(new buffer_type(b)) {
}

packet::packet(buffer_type &&b) : shared_data(new buffer_type(std::move(b))) {
}

packet::packet(size_t size, char value) :
  shared_data(new vector<char>(size, value)) {}

packet::packet(const packet &p) :
  shared_data(new vector<char>(*p.shared_data)) {}

packet &packet::operator=(const packet &p) {
  shared_data = make_shared< vector<char> >(*p.shared_data);
  return *this;
}

void packet::assign(size_type count, char value) {
  shared_data->assign(count, value);
}

char &packet::at(size_type pos) {
  return shared_data->at(pos);
}

const char &packet::at(size_type pos) const {
  return shared_data->at(pos);
}

char &packet::operator[](size_type pos) {
  return (*shared_data)[pos];
}

const char &packet::operator[](size_type pos) const {
  return (*shared_data)[pos];
}

char &packet::front() {
  return shared_data->front();
}

const char &packet::front() const {
  return shared_data->front();
}

char &packet::back() {
  return shared_data->back();
}

const char &packet::back() const {
  return shared_data->back();
}

char *packet::data() {
  return shared_data->data();
}

const char *packet::data() const {
  return shared_data->data();
}

packet::iterator packet::begin() {
  return shared_data->begin();
}

packet::iterator packet::end() {
  return shared_data->end();
}

packet::const_iterator packet::begin() const {
  return shared_data->cbegin();
}

packet::const_iterator packet::end() const {
  return shared_data->cend();
}

packet::const_iterator packet::cbegin() const {
  return shared_data->cbegin();
}

packet::const_iterator packet::cend() const {
  return shared_data->cend();
}

packet::reverse_iterator packet::rbegin() {
  return shared_data->rbegin();
}

packet::reverse_iterator packet::rend() {
  return shared_data->rend();
}

packet::const_reverse_iterator packet::rbegin() const {
  return shared_data->crbegin();
}

packet::const_reverse_iterator packet::rend() const {
  return shared_data->crend();
}

packet::const_reverse_iterator packet::crbegin() const {
  return shared_data->crbegin();
}

packet::const_reverse_iterator packet::crend() const {
  return shared_data->crend();
}

packet::size_type packet::size() const {
  return shared_data->size();
}

bool packet::empty() const {
  return shared_data->empty();
}

packet::size_type packet::max_size() const {
  return shared_data->max_size();
}

void packet::reserve(size_type new_cap) {
  shared_data->reserve(new_cap);
}

packet::size_type packet::capacity() const {
  return shared_data->capacity();
}

void packet::clear() {
  shared_data->clear();
}

packet::iterator packet::insert(const_iterator pos, char value) {
  return shared_data->insert(pos, value);
}

packet::iterator packet::insert(const_iterator pos, size_type count, char value) {
  return shared_data->insert(pos, count, value);
}

packet::iterator packet::erase(const_iterator pos) {
  return shared_data->erase(pos);
}

packet::iterator packet::erase(const_iterator first, const_iterator last) {
  return shared_data->erase(first, last);
}

void packet::push_back(char value) {
  shared_data->push_back(value);
}

void packet::pop_back() {
  shared_data->pop_back();
}

void packet::resize(size_type size) {
  shared_data->resize(size);
}

void packet::resize(size_type size, char value) {
  shared_data->resize(size, value);
}

void packet::swap(packet &other) {
  shared_data.swap(other.shared_data);
}

packet packet::shallow_copy() const {
  packet p;
  p.shared_data = shared_data;
  return p;
}

std::size_t packet::shared_count() const {
  return shared_data.use_count();
}

void packet::xor_data(const packet &other) {
  uep::inplace_xor(*shared_data, *(other.shared_data));
}

packet::operator bool() const {
  return !empty();
}

bool packet::operator!() const {
  return empty();
}

bool operator==(const packet &lhs, const packet &rhs) {
  return (lhs.shared_data == rhs.shared_data) ||
    (*lhs.shared_data == *rhs.shared_data);
}

bool operator!=(const packet &lhs, const packet &rhs) {
  return !(lhs == rhs);
}

void swap(packet &lhs, packet &rhs) {
  lhs.swap(rhs);
}

packet &operator^=(packet &a, const packet &b) {
  a.xor_data(b);
  return a;
}

packet operator^(const packet &a, const packet &b) {
  packet c(a);
  return c ^= b;
}

fountain_packet::fountain_packet(int blockno_, int seqno_, int seed_,
				 size_type count, char value) :
  packet(count, value),
  blockno(blockno_), seqno(seqno_), seed(seed_), priorita(0) {}

fountain_packet::fountain_packet(int blockno_, int seqno_, int seed_,
				 size_type count, char value, uint8_t p_) :
  packet(count, value),
  blockno(blockno_), seqno(seqno_), seed(seed_), priorita(p_) {}

fountain_packet::fountain_packet() :
  fountain_packet(0,0,0,0,0,0) {}

fountain_packet::fountain_packet(int blockno_, int seqno_, int seed_) :
  fountain_packet(blockno_, seqno_, seed_, 0, 0, 0) {}

fountain_packet::fountain_packet(size_type count, char value) :
  fountain_packet(0,0,0,count, value, 0) {}

fountain_packet::fountain_packet(const packet &p) :
  packet(p), blockno(0), seqno(0), seed(0), priorita(0) {}

fountain_packet::fountain_packet(packet &&p) :
  packet(move(p)), blockno(0), seqno(0), seed(0) {}

fountain_packet &fountain_packet::operator=(const packet &other) {
  packet::operator=(other);
  blockno = 0;
  seqno = 0;
  seed = 0;
  priorita = 0;
  return *this;
}

fountain_packet &fountain_packet::operator=(packet &&other) {
  packet::operator=(move(other));
  blockno = 0;
  seqno = 0;
  seed = 0;
  priorita = 0;
  return *this;
}

uint8_t fountain_packet::getPriority() const {
  return priorita;
}

int fountain_packet::block_number() const {
  return blockno;
}

int fountain_packet::block_seed() const {
  return seed;
}

int fountain_packet::sequence_number() const {
  return seqno;
}

void fountain_packet::setPriority(uint8_t p_) {
	priorita = p_;
}

void fountain_packet::block_number(int blockno_) {
  blockno = blockno_;
}

void fountain_packet::block_seed(int seed_) {
  seed = seed_;
}

void fountain_packet::sequence_number(int seqno_) {
  seqno = seqno_;
}

fountain_packet fountain_packet::shallow_copy() const {
  fountain_packet copy(packet::shallow_copy());
  copy.blockno = blockno;
  copy.seqno = seqno;
  copy.seed = seed;
  copy.priorita = priorita;
  return copy;
}

fountain_packet &operator^=(fountain_packet &a, const packet &b) {
  a.xor_data(b);
  return a;
}

std::ostream &operator<<(std::ostream &out, const fountain_packet &p) {
  out << "fountain_packet(" <<
    "blockno=" << p.block_number() <<
    ", seqno=" << p.sequence_number() <<
    ", seed=" << p.block_seed() <<
    ", priority=" << p.getPriority() <<
	", size=" << p.size() <<
    ")";
  return out;
}

bool operator==(const fountain_packet &lhs, const fountain_packet &rhs ) {
  return lhs.block_number() == rhs.block_number() &&
    lhs.sequence_number() == rhs.sequence_number() &&
    lhs.block_seed() == rhs.block_seed() &&
	lhs.getPriority() == rhs.getPriority() &&
    static_cast<const packet &>(lhs) == static_cast<const packet &>(rhs);
}

bool operator!=(const fountain_packet &lhs, const fountain_packet &rhs ) {
  return !(lhs == rhs);
}

namespace uep {

uep_packet uep_packet::from_packet(const packet &p) {
  uep_packet up;
  buffer_type &upb = up.buffer();
  const buffer_type &pb = p.buffer();

  upb.resize(pb.size() - sizeof(seqno_type));
  std::copy(pb.cbegin() + sizeof(seqno_type), pb.cend(),
	    upb.begin());

  seqno_type sn;
  rw_utils::read_ntoh<seqno_type>(sn, pb.cbegin(),
				  pb.cbegin() + sizeof(seqno_type));
  up.seqno = boost::numeric_cast<std::size_t>(sn);
  return up;
}

uep_packet uep_packet::from_fountain_packet(const fountain_packet &fp) {
  uep_packet up = from_packet(fp);
  up.priority_lvl = fp.getPriority();
  return up;
}

uep_packet::uep_packet() : shared_buf(new buffer_type()),
			   priority_lvl(0),
			   seqno(0) {
}

uep_packet::uep_packet(buffer_type &&b) : uep_packet() {
  *shared_buf = std::move(b);
}

uep_packet::uep_packet(const buffer_type &b) : uep_packet() {
  *shared_buf = b;
}

packet uep_packet::to_packet() const {
  packet p;
  const buffer_type &upb = buffer();
  buffer_type &pb = p.buffer();

  pb.resize(upb.size() + sizeof(seqno_type));
  std::copy(upb.cbegin(), upb.cend(), pb.begin() + sizeof(seqno_type));

  seqno_type sn = boost::numeric_cast<seqno_type>(seqno);
  rw_utils::write_hton<seqno_type>(sn, pb.begin(),
				   pb.begin() + sizeof(seqno_type));

  return p;
}

fountain_packet uep_packet::to_fountain_packet() const {
  packet p = to_packet();
  fountain_packet fp(std::move(p));
  fp.setPriority(priority_lvl);
  return fp;
}


buffer_type &uep_packet::buffer() {
  return *shared_buf;
}

const buffer_type &uep_packet::buffer() const {
  return *shared_buf;
}

std::shared_ptr<buffer_type> uep_packet::shared_buffer() {
  return shared_buf;
}

std::shared_ptr<const buffer_type> uep_packet::shared_buffer() const {
  return shared_buf;
}

std::size_t uep_packet::priority() const {
  return priority_lvl;
}

void uep_packet::priority(std::size_t p) {
  priority_lvl = p;
}

uep_packet::seqno_type uep_packet::sequence_number() const {
  return seqno & 0x7fffffff;
}

void uep_packet::sequence_number(seqno_type sn) {
 if (sn > MAX_SEQNO) {
   throw std::invalid_argument("seqno is too large");
 }

 seqno_type padding_bit = seqno & 0x80000000;
 seqno = sn;
 seqno |= padding_bit;
}

uep_packet uep_packet::make_padding(std::size_t size, seqno_type seqno) {
  uep_packet p{};
  p.buffer().resize(size);
  p.sequence_number(seqno);
  p.padding(true);
  for (byte *i = reinterpret_cast<byte*>(p.buffer().data());
       i != reinterpret_cast<byte*>(p.buffer().data()) + size;
       ++i) {
    *i = padding_rng();
  }
  return p;
}

void uep_packet::padding(bool enable) {
  if (enable) {
    seqno |= 0x80000000; // Set the MSB
  }
  else {
    seqno &= 0x7fffffff;
  }
}

bool uep_packet::padding() const {
  return seqno > MAX_SEQNO;
}

std::independent_bits_engine<std::default_random_engine,
			     8,
			     byte> uep_packet::padding_rng;


}
