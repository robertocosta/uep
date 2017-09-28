#ifndef UEP_NAL_WRITER_HPP
#define UEP_NAL_WRITER_HPP

#include <fstream>
#include <queue>
#include <ostream>
#include <sstream>

#include "log.hpp"
#include "lt_param_set.hpp"
#include "packets.hpp"

namespace uep {

class nal_writer {
public:
  /** Type of the parameter set used to setup the writer. */
  typedef net_parameter_set parameter_set;

  /** Construct using a given parameter set. */
  explicit nal_writer(const parameter_set &ps);
  /** Construct a writer that will write to `strname`. The given
   *  header is prepended to the output stream. The stream name is
   *  mapped to `dataset_client/${strname}.264`.
   */
  explicit nal_writer(const buffer_type &header, const std::string &strname);
  /** Construct a writer that will write to the given ostream. */
  explicit nal_writer(std::ostream &out, const buffer_type &header);
  explicit nal_writer(std::ostream &out);

  ~nal_writer();

  void push(const fountain_packet &p);
  void flush();

  // Can always be pushed to. Remove this?
  explicit operator bool() const { return true; }
  bool operator!() const { return false; }

private:
  log::default_logger basic_lg, perf_lg;

  std::string stream_name;

  std::unique_ptr<std::ofstream> file_backend;
  std::ostream &file;

  buffer_type nal_buf; /**< Holds the partially received NALs. */
  std::size_t buf_prio; /**< The priority of the NALs in the buffer. */

  /** Look in the NAL buffer, enqueue any full NALs found and remove
   *  them from the buffer. The argument is set to true if there can
   *  not be partial NALs left in the buffer.
   */
  void enqueue_nals(bool must_end);

  std::string filename() const;
};

/** Search for the start of a NAL in the given range. */
template <class Iter>
Iter find_startcode(Iter first, Iter last);
/** Search for the end of a NAL in the given range. */
template <class Iter>
Iter find_nal_end(Iter first, Iter last);

	       //// function template definitions ////

template <class Iter>
Iter find_startcode(Iter first, Iter last) {
  static const char sc[3] = {0x00, 0x00, 0x01};
  auto i = std::search(first, last, sc, sc+3);
  return i;
}

template <class Iter>
Iter find_nal_end(Iter first, Iter last) {
  log::default_logger basic_lg = log::basic_lg::get();
  log::default_logger perf_lg = log::perf_lg::get();

  static const char zeros[2] = {0x00, 0x00};

  // Search for a sequence [0, 0, (0|1|2)] that cannot be present in the NAL
  // Typically will find the 4- or 3-byte startcode
  Iter i = first;
  for (;;) {
    i = std::search(i, last, zeros, zeros+2);

    if (i == last) {
      BOOST_LOG_SEV(basic_lg, log::trace) << "Not found 0x00, 0x00";
    }
    else if (last - i < 3) {
      BOOST_LOG_SEV(basic_lg, log::trace) << "Found 0x00, 0x00 at "
					  << (void*) &*i
					  << " too short (<3)";
    }

    if (i == last) return i; // Not found
    if (last - i < 3) return last; // No end: needs 3 bytes

    char next = *(i+2);
    if (next == 0x00 ||
	next == 0x01 ||
	next == 0x02) { // Found
      BOOST_LOG_SEV(basic_lg, log::trace) << "Found the end at "
					  << (void*) &*i
					  << " next char="
					  << std::hex << (unsigned int) next;
      return i;
    }
    else {
      BOOST_LOG_SEV(basic_lg, log::trace) << "Not found at "
					  << (void*) &*i
					  << " next char="
					  << std::hex << (unsigned int) next;
      i+=3; // Retry after this match
    }
  }
}

}

#endif
