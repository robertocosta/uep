#ifndef UEP_SVC_SEGMENTER_HPP
#define UEP_SVC_SEGMENTER_HPP

#include "log.hpp"
#include "packets.hpp"

// Keep this order
#include "H264AVCCommonLib.h"
#include "H264AVCVideoIoLib.h"
#include "H264AVCDecoderLib.h"
#include "CreaterH264AVCDecoder.h"
#include "ReadBitstreamFile.h"
#include "WriteBitstreamToFile.h"

/** Exception class that carries a JSVM error code. */
class jsvm_error : public std::runtime_error {
public:
  explicit jsvm_error(ErrVal ec, const std::string &what_arg) :
    std::runtime_error(what_arg),
    error_code(ec) {
  }
  explicit jsvm_error(ErrVal ec) : jsvm_error(ec, "") {
  }

  ErrVal code() const {
    return error_code;
  }

private:
  ErrVal error_code;
};

/** Macro to run a JSVM function and throw if the return value
 *  indicates failure.
 */
#define EV_CHECK(statement, what_arg) do {		\
    ErrVal ev = (statement);				\
    if (ev != Err::m_nOK) {				\
      throw jsvm_error(ev, (what_arg));			\
    }							\
  } while (false)

namespace uep {

/** Four-byte start code to insert before NAL units. */
constexpr std::array<char,4> NAL_STARTCODE{0,0,0,1};

/** Class that extracts NAL units from an H264 stream and provides
 *  them grouped into segments with an assigned priority. The grouping
 *  is done when the NAL units would not be useful by themselves
 *  (e.g. prefix units).
 */
class svc_segmenter {
public:
  explicit svc_segmenter(const std::string &filename);
  ~svc_segmenter();

  /** Extract the next segment with the priority level set. */
  fountain_packet next_segment();

  /** Return true when there are no more segments to be extracted. */
  bool eof() const;

  /** Return true when there are more segments to be extracted. */
  explicit operator bool() const;
  /** Return true when there are no more segments to be extracted. */
  bool operator!() const;

private:
  log::default_logger basic_lg, perf_lg;

  ReadBitstreamFile *reader; /**< Extract the NALs from a file. This
			      *	  expects the stream to contain
			      *	  startcodes before each NAL.
			      */
  h264::H264AVCPacketAnalyzer *pkt_analyzer; /**< NAL Analyzer. */

  bool reader_eos; /**< Set when the reader reaches the end of stream. */
  buffer_type last_segment; /**< Hold the segment while it is being
			     *	 constructed.
			     */
  std::uint8_t last_priority; /**< The priority assigned to the
			       *   current segment.
			       */
  h264::PacketDescription last_pdesc; /**< Last pkt description
				       *   produced by the analyzer. 
				       */
  h264::SEI::SEIMessage *last_sei; /**< Pointer to the last SEI
				    *   message produced by the
				    *   analyzer.
				    */

  /** Extract the next packet from the file and append it to the
   *  buffer, prefixing it with the startcode. If there are no more
   *  NALs to be read this method sets reader_eos.
   */
  void extract_into_segment();
  /** Assign a priority to the current segment. If the method returns
   *  true `last_priority` contains the assigned priority and the
   *  segment is complete. Otherwise the segment should be packed with
   *  the next NAL.
   */
  bool assign_priority();
};

class svc_assembler {
public:
  explicit svc_assembler(const std::string &filename);
  ~svc_assembler();

  void push_segment(const buffer_type &buf);
  
private:
  log::default_logger basic_lg, perf_lg;

  WriteBitstreamToFile *writer;
};

}

#endif
