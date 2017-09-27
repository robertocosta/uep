#ifndef UEP_NAL_READER_HPP
#define UEP_NAL_READER_HPP

#include <fstream>
#include <queue>
#include <sstream>

#include "log.hpp"
#include "lt_param_set.hpp"
#include "packets.hpp"

#ifdef UEP_SPLIT_STREAMS
#include "uep_encoder.hpp"

bool file_exists (const std::string& name);
bool file_exists (const std::string& name, size_t *length);
std::vector<char> readByteFromFile(std::string filename, int from, int len);
std::vector<char> readByteFromFile(std::string filename, int from, int len, bool * ok);
std::vector<char> readByteFromFileOpened(std::ifstream ifs, int from, int len);
bool writeCharVecToFile(std::string filename, std::vector<char> v);
bool overwriteCharVecToFile(std::string filename, std::vector<char> v);
//typedef uep::lt_uep_parameter_set all_params;
typedef uep::all_parameter_set<uep::uep_encoder<>::parameter_set> all_params;
//all_params parset;
struct packet_source {
  typedef all_params parameter_set;
  std::vector<size_t> Ks;
  std::vector<size_t> rfs;
  std::vector<size_t> currInd;
  std::vector<uint8_t> currRep;
  size_t Ls;
  uint ef;
  std::vector<size_t> max_count;
  uint currQid;
  uint efReal;
  //parameter_set paramSet;
  std::string nomeStream;
  std::vector<std::ifstream> files;
  //boost::random::mt19937 gen;
  size_t totalLength;
  std::vector<uep::streamTrace> videoTrace;
  std::vector<char> header;
  size_t headerLength;
  explicit packet_source(const parameter_set &ps);
  fountain_packet next_packet();
  size_t totLength() const;

  explicit operator bool() const;
  bool operator!() const;
};

#endif

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

  /** Return the size of the original stream. */
  std::size_t totLength() const;

  /** Return true when the reader is able to produce a packet. */
  explicit operator bool() const;
  /** Return true when the reader cannot produce a packet. */
  bool operator!() const;

private:
  static constexpr std::size_t MAX_PACKED_SIZE = 16*1024*1024;

  log::default_logger basic_lg, perf_lg;

  std::string stream_name;
  std::size_t pkt_size;

  std::ifstream file;
  std::ifstream trace;

  buffer_type hdr;
  std::size_t totalLength;

  buffer_type last_nal;
  std::size_t last_prio;
  std::queue<fountain_packet> pkt_queue;

  /** Return the name of the underlying file with the H264 bitstream. */
  std::string filename() const;
  /** Return the name of the underlying file with the NAL trace. */
  std::string tracename() const;
  /** Read the first NALs into the `hdr` buffer. */
  void read_header();
  /** Read the next line from the NAL trace. */
  streamTrace read_trace_line();
  /** Read the NAL unit corresponding to the given trace line. */
  buffer_type read_nal(const streamTrace &st);
  /** Assign a priority to the NAL unit. */
  std::size_t classify(const streamTrace &st);
  /** Create more packets by packing together NALs with the same
   *  priority.
   */
  void pack_nals();
};

}

#endif
