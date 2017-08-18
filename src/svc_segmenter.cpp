#include "svc_segmenter.hpp"

using namespace std;

namespace uep {



svc_segmenter::svc_segmenter(const std::string &filename) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  reader(nullptr),
  pkt_analyzer(nullptr),
  reader_eos(false),
  last_sei(nullptr) {
  EV_CHECK(ReadBitstreamFile::create(reader), "Creating reader");
  EV_CHECK(reader->init(filename), "Initing reader");
  EV_CHECK(h264::H264AVCPacketAnalyzer::create(pkt_analyzer),
	   "Creating pkt_analyzer");
  EV_CHECK(pkt_analyzer->init(), "Initing pkt_analyzer");
}

svc_segmenter::~svc_segmenter() {
  if (reader != nullptr) {
    reader->uninit();
    reader->destroy();
  }
  if (pkt_analyzer != nullptr) {
    pkt_analyzer->uninit();
    pkt_analyzer->destroy();
  }
  delete last_sei;
}

fountain_packet svc_segmenter::next_segment() {
  last_segment.clear();
  bool pack;
  do {
    extract_into_segment();
    pack = assign_priority();
  } while (pack && !eof());

  fountain_packet p;
  p.buffer() = std::move(last_segment);
  p.setPriority(last_priority);
  return p;
}

bool svc_segmenter::eof() const {
  return reader_eos;
}

svc_segmenter::operator bool() const {
  return !eof();
}

bool svc_segmenter::operator!() const {
  return eof();
}

void svc_segmenter::extract_into_segment() {
  BinData *data(nullptr);
  try {
    EV_CHECK(reader->extractPacket(data, reader_eos),
	     "Extracting packet from reader");
    if (reader_eos) {
      throw runtime_error("Reader reached the end of file");
    }

    delete last_sei;
    last_sei = nullptr;
    EV_CHECK(pkt_analyzer->process(data, last_pdesc, last_sei), "Analyzing packet");

    const char *d_begin = (char*)(data->data());
    const char *d_end = d_begin + data->size();

    auto d_rend = std::find_if(std::make_reverse_iterator(d_end),
			       std::make_reverse_iterator(d_begin),
			       [](char c){return c != 0x00;});
    d_end = d_rend.base();

    last_segment.insert(last_segment.end(),
			NAL_STARTCODE.cbegin(),
			NAL_STARTCODE.cend());
    last_segment.insert(last_segment.end(), d_begin, d_end);
  }
  catch (const std::runtime_error &e) {
    EV_CHECK(reader->releasePacket(data), "Releasing packet after exception");
    throw e;
  }
  EV_CHECK(reader->releasePacket(data), "Releasing packet");

  // Peek next packet to update the eos flag
  Int pos;
  EV_CHECK(reader->getPosition(pos), "Getting reader position");
  EV_CHECK(reader->extractPacket(data, reader_eos),
	   "Extracting packet from reader");
  EV_CHECK(reader->releasePacket(data), "Releasing packet");
  EV_CHECK(reader->setPosition(pos), "Setting reader position");
}

bool svc_segmenter::assign_priority() {
  BOOST_LOG_SEV(basic_lg, log::trace)
    << "Assign priority to packet."
    << " ParameterSet=" << last_pdesc.ParameterSet
    << " Scalable=" << last_pdesc.Scalable
    << " Layer=" << last_pdesc.Layer
    << " Level=" << last_pdesc.Level
    << " FGSLayer=" << last_pdesc.FGSLayer
    << " ApplyToNext=" << last_pdesc.ApplyToNext
    << " NalUnitType=" << last_pdesc.NalUnitType
    << " SPSid=" << last_pdesc.SPSid
    << " PPSid=" << last_pdesc.PPSid
    // skip arrays
    << " uiNumLevelsQL=" << last_pdesc.uiNumLevelsQL
    << " bDiscardable=" << last_pdesc.bDiscardable
    << " uiFirstMb=" << last_pdesc.uiFirstMb
    << " bDiscardableHRDSEI=" << last_pdesc.bDiscardableHRDSEI;

  switch (last_pdesc.NalUnitType) {
  case h264::NAL_UNIT_SPS:
  case h264::NAL_UNIT_PPS:
  case h264::NAL_UNIT_SPS_EXTENSION:
  case h264::NAL_UNIT_SUBSET_SPS:
    BOOST_LOG_SEV(basic_lg, log::trace) << "NAL has a Parameter Set";
    last_priority = 0; // Max priority: PSs are required to decode
    return false;
  case h264::NAL_UNIT_SEI:
    if (last_sei && last_sei->getMessageType() == h264::SEI::SCALABLE_SEI) {
      BOOST_LOG_SEV(basic_lg, log::trace) << "NAL is a Scalable SEI";
      last_priority = 0; // Max priority: contains SVC params
      return false;
    }
    else if (last_pdesc.ApplyToNext) {
      BOOST_LOG_SEV(basic_lg, log::trace) << "NAL is a SEI with ApplyToNext";
      return true; // Pack with next NAL
    }
    else {
      // Can be integrity check nals
      return true;
      throw std::runtime_error("Unexpected SEI");
    }
  case h264::NAL_UNIT_PREFIX:
    BOOST_LOG_SEV(basic_lg, log::trace) << "NAL is a Prefix";
    return true; // Pack with the next NAL
  case h264::NAL_UNIT_CODED_SLICE:
  case h264::NAL_UNIT_CODED_SLICE_IDR:
  case h264::NAL_UNIT_CODED_SLICE_SCALABLE:
    BOOST_LOG_SEV(basic_lg, log::trace) << "NAL is a Coded Slice";
    if (last_pdesc.NalUnitType == h264::NAL_UNIT_CODED_SLICE_IDR) {
      last_priority = 0;
      return false;
    }
    // Copy condition from JSVM    
    if (last_pdesc.ParameterSet || (last_pdesc.Level == 0 &&
				    last_pdesc.FGSLayer == 0)
	/*!last_pdesc.bDiscardable*/) {
      BOOST_LOG_SEV(basic_lg, log::trace) << "Not discardable NAL";
      last_priority = 0; // Max priority
    }
    else {
      // Start from 1, increase with Quality layer num
      last_priority = 1 + last_pdesc.FGSLayer; 
    }
    return false;
  default:
    throw std::runtime_error("Unexpected NAL type");
  }
}

svc_assembler::svc_assembler(const std::string &filename) : writer(nullptr) {
  EV_CHECK(WriteBitstreamToFile::create(writer), "Creating writer");
  EV_CHECK(writer->init(filename), "Initing writer");
}

svc_assembler::~svc_assembler() {
  if (writer != nullptr) {
    writer->uninit();
    writer->destroy();
  }
}

void svc_assembler::push_segment(const buffer_type &buf) {
  BinData data((UChar*)buf.data(), buf.size());
  try {
    EV_CHECK(writer->writePacket(&data), "Writing segment");
  }
  catch (const runtime_error &e) {
    data.reset();
    throw e;
  }
  data.reset();
}

}
