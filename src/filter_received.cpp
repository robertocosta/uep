#include <fstream>

#include "log.hpp"
#include "lt_param_set.hpp"
#include "utils.hpp"

namespace uep {

class nal_filter {
public:
  explicit nal_filter(std::istream &in_file_,
		      std::istream &in_trace_,
		      std::ostream &out_file_,
		      std::ostream &out_trace_) :
    basic_lg{boost::log::keywords::channel = log::basic},
    perf_lg{boost::log::keywords::channel = log::performance},
    in_file{in_file_},
    in_trace{in_trace_},
    out_file{out_file_},
    out_trace{out_trace_} {
  }

  void run() {
    out_trace << trace_headline;
    BOOST_LOG_SEV(basic_lg, log::debug) << "Written the trace header";

    std::size_t last_layer;
    std::size_t nal_offset = 0;
    while(in_trace) {
      streamTrace st = read_trace_line();
      unique_buffer buf(st.len);
      read_nal(st, buf);

      BOOST_LOG_SEV(basic_lg, log::trace) << "Read a NAL with qid="
					  << st.qid;

      if (st.qid == 0) {
	last_layer = 0;
      }
      else if (static_cast<std::size_t>(st.qid) == last_layer + 1) {
	++last_layer;
      }
      else {
	// Lost a layer: drop all the following ones until a qid==0
	BOOST_LOG_SEV(basic_lg, log::debug) << "Remove a packet with qid = "
					    << st.qid;
	continue;
      }

      BOOST_LOG_SEV(basic_lg, log::trace) << "Write a NAL with offset="
					  << nal_offset
					  << " length=" << st.len;
      st.startPos = nal_offset;
      out_file.write(buf.data<char>(), st.len);
      nal_offset += st.len;
      write_trace_line(st);
    }
  }

private:
  static const std::string trace_headline;

  log::default_logger basic_lg, perf_lg;

  std::istream &in_file;
  std::istream &in_trace;
  std::ostream &out_file;
  std::ostream &out_trace;

  streamTrace read_trace_line() {
    std::string line;
    std::getline(in_trace,line);
    utils::skip_empty(in_trace);
    if (line.empty()) {
      throw std::runtime_error("Empty trace line");
    }

    BOOST_LOG_SEV(basic_lg, log::trace) << "Reading trace line"
					<< ". Raw line: "
					<< line;

    std::istringstream iss{line};
    streamTrace elem;

    iss >> std::hex >> elem.startPos;
    if (iss.fail()) { // Was not a packet line, ignore and take next one
      BOOST_LOG_SEV(basic_lg, log::trace) << "Skip non-packet line";
      //trace.clear();
      return read_trace_line();
    }

  iss >> std::dec;
  iss >> elem.len;
  iss >> elem.lid;
  iss >> elem.tid;
  iss >> elem.qid;

  std::string pkt_type;
  iss >> std::ws; // Skip whitespace
  std::getline(iss, pkt_type, ' ');
  BOOST_LOG_SEV(basic_lg, log::trace) << "Raw pkt_type=" << pkt_type;
  if (pkt_type == "StreamHeader") {
    elem.packetType = streamTrace::stream_header;
  } else if (pkt_type == "ParameterSet") {
    elem.packetType = streamTrace::parameter_set;
  } else if (pkt_type == "SliceData") {
    elem.packetType = streamTrace::slice_data;
  }
  else throw std::runtime_error("Unknown packet type");

  std::string disc;
  iss >> std::ws; // Skip whitespace
  std::getline(iss, disc, ' ');
  BOOST_LOG_SEV(basic_lg, log::trace) << "Raw disc=" << disc;
  if (disc == "Yes") {
    elem.discardable = true;
  } else if (disc == "No") {
    elem.discardable = false;
  }
  else throw std::runtime_error("Unknown discardable field");

  std::string trunc;
  iss >> std::ws; // Skip whitespace
  std::getline(iss, trunc, ' ');
  BOOST_LOG_SEV(basic_lg, log::trace) << "Raw trunc=" << trunc;
  if (trunc == "Yes") {
    elem.truncatable = true;
  } else if (trunc == "No") {
    elem.truncatable = false;
  }
  else throw std::runtime_error("Unknown truncatable field");

  BOOST_LOG_SEV(basic_lg, log::trace) << std::hex
				      << "Read streamTrace: offset="
				      << elem.startPos
				      << " length="
				      << std::dec << elem.len
				      << " Lid=" << elem.lid
				      << " Tid=" << elem.tid
				      << " Qid=" << elem.qid
				      << std::boolalpha
				      << " PktType=" << elem.packetType
				      << " Disc=" << elem.discardable
				      << " Trunc=" << elem.truncatable;

  return elem;
  }

  void write_trace_line(const streamTrace &st) {
    out_trace << "0x"
	      << std::hex << std::noshowbase
	      << std::setfill('0') << std::internal
	      << std::setw(8)
	      << st.startPos;
    out_trace << "  ";
    out_trace << std::dec << std::noshowbase
	      << std::setfill(' ') << std::right
	      << std::setw(6)
	      << st.len;
    out_trace << "  ";
    out_trace << std::setw(3) << st.lid;
    out_trace << "  ";
    out_trace << std::setw(3) << st.tid;
    out_trace << "  ";
    out_trace << std::setw(3) << st.qid;
    out_trace << "  ";
    out_trace << std::setw(12) << st.packetType;
    out_trace << "  ";
    out_trace << std::setw(10) << (st.discardable ? "Yes" : "No") << " ";
    out_trace << "  ";
    out_trace << std::setw(9) << (st.truncatable ? "Yes" : "No");
    out_trace << std::endl;
  }

  void read_nal(const streamTrace &st, buffer &buf) {
    buf = buf.slice(0, st.len);
    if (in_file.tellg() != st.startPos) {
      in_file.seekg(st.startPos);
    }
    in_file.read(buf.data<char>(), buf.size());
    assert(in_file.gcount() == st.len);
  }
};

const std::string nal_filter::trace_headline{
  "Start-Pos.  Length  LId  TId  QId   Packet-Type  Discardable  Truncatable\n"
  "==========  ======  ===  ===  ===  ============  ===========  ===========\n"
};

}

using namespace std;
using namespace uep;

int main(int argc, char **argv) {
  log::init("filter_received.log");
  log::default_logger basic_lg = log::basic_lg::get();
  log::default_logger perf_lg = log::perf_lg::get();

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " {stream name}";
  }

  std::string stream_name{argv[1]};

  std::ifstream in_file{stream_name + ".264", ios_base::binary};
  std::ofstream out_file{stream_name + "_filtered.264", ios_base::binary};
  std::ifstream in_trace{stream_name + ".trace"};
  std::ofstream out_trace{stream_name + "_filtered.trace"};

  nal_filter fil{in_file, in_trace, out_file, out_trace};
  fil.run();
  return 0;
}
