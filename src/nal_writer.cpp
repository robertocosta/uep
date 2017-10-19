#include "nal_writer.hpp"

using namespace std;

namespace uep {

nal_writer::nal_writer(const buffer_type &header,
		       const std::string &strname) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  stream_name(strname),
  file_backend(new ofstream(filename(), ios_base::binary)),
  file(*file_backend),
  buf_prio(0),
  eos_recvd(false) {
  BOOST_LOG_SEV(basic_lg, log::trace) << "Create a NAL writer for "
				      << stream_name;

  file.write(header.data(), header.size());
  file.flush();
  BOOST_LOG_SEV(basic_lg, log::trace) << "Written the header";
}

nal_writer::nal_writer(const parameter_set &ps) :
  nal_writer(ps.header, ps.streamName) {
}

nal_writer::nal_writer(std::ostream &out) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  file(out),
  buf_prio(0) {
  BOOST_LOG_SEV(basic_lg, log::trace) << "Create a NAL writer with a given ostream";
}

nal_writer::nal_writer(std::ostream &out, const buffer_type &header) :
  nal_writer(out) {
  file.write(header.data(), header.size());
  file.flush();
  BOOST_LOG_SEV(basic_lg, log::trace) << "Written the header";
}

nal_writer::~nal_writer() {
  flush();
}

void nal_writer::push(const fountain_packet &p) {
  if (eos_recvd) {
    throw std::runtime_error("The EOS was received");
  }

  std::size_t prio = p.getPriority();
  BOOST_LOG_SEV(basic_lg, log::trace) << "Writer has a new packet"
				      << " with prio=" << prio;
  if (prio == buf_prio) {
    nal_buf.insert(nal_buf.end(), p.buffer().begin(), p.buffer().end());
    BOOST_LOG_SEV(basic_lg, log::trace) << "Appended " << p.buffer().size()
					<< " bytes to the nal_buf";
    enqueue_nals(false);
  }
  else {
    enqueue_nals(true);
    buf_prio = prio;
    nal_buf = p.buffer();
    BOOST_LOG_SEV(basic_lg, log::trace) << "Set " << p.buffer().size()
					<< " bytes in the nal_buf";
    enqueue_nals(false);
  }
}

void nal_writer::flush() {
  enqueue_nals(true);
  file.flush();
}

void nal_writer::enqueue_nals(bool must_end) {
  BOOST_LOG_SEV(basic_lg, log::trace) << "Try to enqueue NALs"
				      << " must_end=" << std::boolalpha
				      << must_end;
  auto i = nal_buf.begin();
  BOOST_LOG_SEV(basic_lg, log::trace) << "nal_buf starts at " << (void*) &*i;
  BOOST_LOG_SEV(basic_lg, log::trace) << "nal_buf size: " << nal_buf.size();
  for(;;) {
    i = find_startcode(i, nal_buf.end());
    BOOST_LOG_SEV(basic_lg, log::trace) << "startcode found at " << (void*) &*i;

    if (i == nal_buf.end()) { // No more NALs
      if (must_end) {
	BOOST_LOG_SEV(basic_lg, log::trace) << "Found no startcode and must_end:"
					    << " drop " << nal_buf.size()
					    << " bytes from the nal_buf";
	nal_buf.clear();
      }
      else {
	BOOST_LOG_SEV(basic_lg, log::trace) << "Found no startcode:"
					    << " drop " << nal_buf.size()-3
					    << " bytes from the nal_buf";
	// Leave 3 bytes (possible begin of startcode)
	nal_buf.erase(nal_buf.begin(), nal_buf.end() - 3);
      }
      return;
    }

    // Check for the EOS code
    const char *const eos_code_begin =
      nal_reader::EOS_NAL.data();
    const char *const eos_code_end =
      eos_code_begin + nal_reader::EOS_NAL.size();
    if (static_cast<size_t>(nal_buf.end() - i) >=
	3 + nal_reader::EOS_NAL.size()) {
      bool is_eos = std::equal(eos_code_begin, eos_code_end,
			       i + 3);
      if (is_eos) {
	BOOST_LOG_SEV(basic_lg, log::info) << "Received the EOS";
	eos_recvd = true;
	nal_buf.clear();
	file.flush();
	return;
      }
    }

    // +3 bytes to not stop on the found startcode
    auto end = find_nal_end(i + 3, nal_buf.end());
    BOOST_LOG_SEV(basic_lg, log::trace) << "NAL end found at " << (void*) &*end;

    if (end == nal_buf.end()) { // No end found: NAL may continue
      if (must_end) { // NAL can not continue: enqueue
	BOOST_LOG_SEV(basic_lg, log::trace) << "must_end: write to file "
					    << end-i
					    << " bytes, clear the nal_buf";
	const char *ic = &(*i);
	size_t cnt = end-i;
  file.write(ic, cnt);
  file.flush();
	nal_buf.clear();
      }
      else {
	// Leave partial NAL
	BOOST_LOG_SEV(basic_lg, log::trace) << "Keep partial NAL, erase "
					    << i - nal_buf.begin()
					    << " bytes from the nal_buf";
	nal_buf.erase(nal_buf.begin(), i);
      }
      return;
    }

    // Full NAL found: enqueue
    BOOST_LOG_SEV(basic_lg, log::trace) << "Full NAL in buf, write "
					<< end-i
					<< " bytes";
    const char *ic = &(*i);
    size_t cnt = end-i;
    file.write(ic, cnt);
    file.flush();    
    i = end;
    BOOST_LOG_SEV(basic_lg, log::trace) << "Advance i to " << (void*) &*i;
  }
}

std::string nal_writer::filename() const {
  return "dataset_client/" + stream_name + ".264";
}

nal_writer::operator bool() const {
  return !eos_recvd;
}

bool nal_writer::operator!() const {
  return eos_recvd;
}

}
