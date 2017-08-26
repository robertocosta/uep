#ifndef UEP_NAL_READER_HPP
#define UEP_NAL_READER_HPP

#include <fstream>
#include <sstream>

#include "log.hpp"
#include "lt_param_set.hpp"
#include "packets.hpp"

namespace uep {

class nal_reader {
public:
  /** Type of the parameter set used to setup the reader. */
  typedef net_parameter_set parameter_set;

  /** Construct a reader with the given parameter set. */
  explicit nal_reader(const parameter_set &ps);

  /** Extract the next fixed-size packet, with zero padding if
   *  required, along with its assigned priority.
   */
  fountain_packet next_packet();

  /** Reference to the raw bitstream of the first set of NALs, which
   *  contain the H264 parameter sets.
   */
  const buffer_type &header() const;

  /** Return true when the reader is able to produce a packet. */
  bool has_packet() const;

  /** Return true when the reader is able to produce a packet. */
  explicit operator bool() const;
  /** Return true when the reader cannot produce a packet. */
  bool operator!() const;

private:
  log::default_logger basic_lg, perf_lg;

  std::string stream_name;

  std::ifstream file;
  std::ifstream trace;

  buffer_type hdr;

  std::size_t pkt_size = 1000;
  buffer_type last_nal;
  buffer_type::const_iterator next_chunk;
  std::size_t last_priority;

  streamTrace read_trace_line();
  void read_header();
  buffer_type read_nal(const streamTrace &st);
};

}

#endif
