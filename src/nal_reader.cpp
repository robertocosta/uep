#include "nal_reader.hpp"

#include <cmath>

#include <ctime>
#include <iostream>
#include <string>
#include<boost/algorithm/string.hpp>
#include <sstream>
#include <fstream>
#include <codecvt>
#include <set>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <ostream>
#include <boost/iostreams/device/file.hpp>
#include <fstream>
#include <boost/iostreams/stream.hpp>

#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "uep_encoder.hpp"

using namespace std;
using namespace uep;

//#ifdef UEP_SPLIT_STREAMS

//typedef all_parameter_set<uep_encoder<>::parameter_set> all_params;
//all_params parset;


bool file_exists (const std::string& name) {
  std::ifstream f(name.c_str());
	bool out = f.good();
	f.close();
	return out;
}
bool file_exists (const std::string& name, size_t *length) {
  using namespace std;
  ifstream f(name.c_str());
  bool out = f.good();
  if (f.is_open()) {
    f.seekg(0, ios::end);
    *length = f.tellg();
  }
  f.close();
  return out;
}

//std::vector<unsigned char> readByteFromFile(const char* filename, int from, int len) {
std::vector<char> readByteFromFile(std::string filename, int from, int len) {
	std::ifstream ifs(filename, std::ifstream::binary);
	//std::vector<unsigned char> vec;
	std::vector<char> content;
	if (ifs) {
		content.resize(len);
		ifs.seekg (from, std::ios::beg);
		ifs.read(&content[0], len);
		ifs.close();
	}
	return content;
}
std::vector<char> readByteFromFile(std::string filename, int from, int len, bool * ok) {
	*ok = false;
	std::ifstream ifs(filename, std::ifstream::in | std::ifstream::binary);
	std::vector<char> content;
	if (!ifs.is_open()) {
		return content;
	}
	ifs.seekg(0, std::ios::end);
	int fileSize = ifs.tellg();
	if (fileSize<from+len) {
		if (from>=fileSize)
			return content;
		else {
			*ok = true;
			len = fileSize-from;
		}
	} else *ok = true;
	if (ifs && *ok) {
		content.resize(len);
		ifs.seekg (from, std::ios::beg);
		ifs.read(&content[0], len);
		ifs.close();
	}
	return content;
}
std::vector<char> readByteFromFileOpened(std::ifstream ifs, int from, int len) {
	std::vector<char> content;
	if (ifs) {
		content.resize(len);
		ifs.seekg (from, std::ios::beg);
		ifs.read(&content[0], len);
	}
	return content;
}

bool writeCharVecToFile(std::string filename, std::vector<char> v) {
	std::ofstream newFile;
	newFile.open(filename, std::ios_base::app); // append mode
	bool out = false;
	if (newFile.is_open()) {
		for (uint i=0; i<v.size(); i++) {
			newFile << v[i];
		}
		out = true;
	}
	newFile.close();
	return out;
}

bool overwriteCharVecToFile(std::string filename, std::vector<char> v) {
	std::ofstream newFile;
	newFile.open(filename, std::ofstream::out); // writing mode
	bool out = false;
	if (newFile.is_open()) {
		for (uint i=0; i<v.size(); i++) {
			newFile << v[i];
		}
		out = true;
	}
	newFile.close();
	return out;
}

std::vector<streamTrace> loadTrace(std::string streamName) {
	std::ifstream file;
	file = std::ifstream("dataset/"+streamName+".trace", std::ios::in|std::ios::binary);
	if (!file.is_open()) throw std::runtime_error("Failed opening file");
	file.seekg (0, std::ios::beg);
	std::string line;
	std::string header;
	//std::regex regex("^0x\\([0-9]+\\)=[0-9]+No$");
	uint16_t lineN = 0;
	uint16_t nRows = 0;
	while (!file.eof()) {
		std::getline(file,line);
		nRows++;
	}
	file.close();
	file = std::ifstream("dataset/"+streamName+".trace", std::ios::in|std::ios::binary|std::ios::ate);
	if (!file.is_open()) throw std::runtime_error("Failed opening file");
	file.seekg (0, std::ios::beg);

	std::vector<streamTrace> sTp;
	//std::cout << nRows << std::endl;
	while (!file.eof()) {
		std::getline(file,line);
		//std::cout << line << std::endl;
		if (line.length()==0) {
			break;
		}
		lineN++;
		std::istringstream iss(line);
		if (lineN>2) {
			std::string whiteSpacesTrimmed;
			int n = line.find(" ");
			std::string s = line;
			while (n>=0) {
				if (n==0) {
					s = s.substr(1,s.length());
				} else if (n>0) {
					whiteSpacesTrimmed += s.substr(0,n) + " ";
					s = s.substr(n,s.length());
				}
				n = s.find(" ");
			}
			whiteSpacesTrimmed += s;
			streamTrace elem;
			for (int i=0; i<8; i++) {
				int n = whiteSpacesTrimmed.find(" ");
				std::string s = whiteSpacesTrimmed.substr(0,n);
				int nn;

				//std::cout << i << std::endl;
				switch (i) {
				case (0):
					elem.startPos = strtoul(s.substr(s.find("x")+1,s.length()).c_str(), NULL, 16);
					break;
				case (1): case (2): case(3): case (4):
					if (s == "0") {
						nn = 0;
					} else {
						nn = stoi( s );
					}
					break;
				case (5):
					if (s == "StreamHeader") {
						elem.packetType = streamTrace::stream_header;
					} else if (s == "ParameterSet") {
						elem.packetType = streamTrace::parameter_set;
					} else if (s == "SliceData") {
						elem.packetType = streamTrace::slice_data;
					}
					break;
				case (6):
					if (s == "Yes") {
						elem.discardable = true;
					} else if (s == "No") {
						elem.discardable = false;
					}
					break;
				case (7):
					if (s == "Yes") {
						elem.truncatable = true;
					} else if (s == "No") {
						elem.truncatable = false;
					}
				break;
				}
				switch (i) {
				case (1):
					elem.len = nn;
					break;
				case (2):
					elem.lid = nn;
					break;
				case (3):
					elem.tid = nn;
					break;
				case (4):
					elem.qid = (uint16_t)nn;
					break;
				}
				whiteSpacesTrimmed = whiteSpacesTrimmed.substr(n+1,whiteSpacesTrimmed.length());
			}
			sTp.push_back(elem);
		}
	}
	file.close();
	return (sTp);
}

// PACKET SOURCE

int sliceDataInd;

bool packet_source::operator!() const { return !static_cast<bool>(*this); }

packet_source::operator bool() const {
	bool out = true;
	for (uint i=0; i<currInd.size(); i++) {
		out = out && (currInd[i] < max_count[i]);
	}
	return out;
}

packet_source::packet_source(const parameter_set &ps) {
	//paramSet = ps;
	Ks = ps.Ks;
	rfs = ps.RFs;
	ef = ps.EF;
	nomeStream = ps.streamName;
	size_t leng = 0;
	if (file_exists("dataset/"+nomeStream+".264", &leng)) totalLength = leng;
	else throw std::runtime_error("Failed opening file");
	currInd.resize(Ks.size());
	currRep.resize(Ks.size());
	files.resize(Ks.size());
	max_count.resize(Ks.size());
	for (uint i=0; i<currInd.size(); i++) { currInd[i]=0; currRep[i]=0; max_count[i] = 0;}
	currQid = 0;
	efReal = 0;
	//streamName = ps.streamName;
	Ls = 64;
	/* RANDOM GENERATION OF FILE */
	/*
	bool textFile = true;
	int fileSize = max_count; // file size = fileSize * Ls;
	std::ofstream newFile (streamName+".txt");
	//std::cout << streamName << ".txt\n";
	if (newFile.is_open()) {
		for (int ii = 0; ii<fileSize; ii++) {
			if (!textFile) {
				boost::random::uniform_int_distribution<> dist(0, 255);
				newFile << ((char) dist(gen));
			} else {
				boost::random::uniform_int_distribution<> dist(65, 90);
				int randN = dist(gen);
				newFile << ((char) randN);
			}
		}
	}
	newFile.close();
	*/
	videoTrace = loadTrace(nomeStream);
	//max_count = videoTrace[videoTrace.size()-1].startPos + videoTrace[videoTrace.size()-1].len;
	//std::cout << "max_count: " << std::to_string(max_count) << std::endl;
	// parse .trace to produce a txt with parts repeated
	// first rows of videoTrace are: stream header and parameter set. must be passed through TCP
	int fromHead = videoTrace[0].startPos;
	int toHead;
	headerLength = 0;
	for (toHead = fromHead; videoTrace[toHead].packetType < 3; toHead++) { headerLength += videoTrace[toHead].len; }
	//int fromSliceData = videoTrace[toHead].startPos;
	sliceDataInd = toHead;
	toHead = videoTrace[toHead-1].startPos + videoTrace[toHead-1].len;
	// int fromSliceData = toHead + 1;

	header = readByteFromFile("dataset/"+nomeStream+".264",fromHead,headerLength);

	for (uint8_t i=0; i<Ks.size(); i++) {
		std::string streamN = "dataset/"+nomeStream+"."+std::to_string(i)+".264";
		size_t leng=0;
		if (file_exists(streamN, &leng)) {
			std::cout << streamN << " already created. Length: "<< leng <<"\n";
		} else {
			uint ii=sliceDataInd;
			while (ii<videoTrace.size()) {
				if (((videoTrace[ii].packetType == 3) && (videoTrace[ii].qid == i))||
					((uint(videoTrace[ii].qid) >= ps.Ks.size()) && (i == ps.Ks.size()-1))) {
					//std::vector<char> slice;
					std::vector<char> slice = readByteFromFile("dataset/"+nomeStream+".264",videoTrace[ii].startPos,videoTrace[ii].len);
					 leng += videoTrace[ii].len;
					if (!writeCharVecToFile(streamN,slice)) {
						std::cout << "error in writing file\n";
					}
					//std::cout << "read " << ii << "th row - qid: " << std::to_string(i) << " -> " << streamN << std::endl;
				}
				ii++;
			}
		}
		max_count[i] = leng;
	}

	for (uint8_t i=0; i<Ks.size(); i++) {
		std::string streamN = "dataset/"+nomeStream+"."+std::to_string(i)+".264";
		files[i] = std::ifstream(streamN, std::ios::in|std::ios::binary);
		if (!files[i].is_open()) throw std::runtime_error("Failed opening file");
	}
}

fountain_packet packet_source::next_packet() { // using uep_encoder
	if (currInd[currQid]*Ls >= max_count[currQid]) throw std::runtime_error("Max packet count");
	std::string streamN = "dataset/"+nomeStream+"."+std::to_string(currQid)+".264";
	bool readingOk;
	std::vector<char> read = readByteFromFile(streamN,currInd[currQid]*Ls,Ls, &readingOk);
	currInd[currQid]++;
	if (!readingOk) throw std::runtime_error("Impossible to read source file");
	if (read.size() < Ls) { // last bits
		for (uint i=0; i<Ls-read.size(); i++) read.push_back(' ');
	}
	fountain_packet fp(read);
	fp.setPriority(currQid);
	cerr << "packet_source::next_packet size=" << fp.size()
		<< " priority=" << static_cast<size_t>(fp.getPriority())
		<< endl;
	if (currInd[currQid] % Ks[currQid] == 0) {
		currQid++;
		std::cout << "changing Qid\n";
	}
	if (currQid == Ks.size()) {
		currQid = 0;
		std::cout << "resetting Qid\n";
	}
	return fp;
}

size_t packet_source::totLength() const {
	return totalLength;
}

//#endif

namespace uep {

std::string nal_reader::filename() const {
  return "dataset/"+stream_name+".264";
}

std::string nal_reader::tracename() const {
  return "dataset/"+stream_name+".trace";
}

nal_reader::nal_reader(const std::string &strname, std::size_t pktsize) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  stream_name(strname),
  pkt_size(pktsize) {
  BOOST_LOG_SEV(basic_lg, log::trace) << "Creating reader for \""
				      << stream_name << "\"";
  file.open(filename(), ios_base::binary|ios_base::ate);
  totalLength = file.tellg();
  file.seekg(0);
  trace.open(tracename());
  BOOST_LOG_SEV(basic_lg, log::trace) << "Opened stream and trace files";

  read_header(); // Read header NALs into hdr
}

nal_reader::nal_reader(const parameter_set &ps) :
  nal_reader(ps.streamName, ps.packet_size) {
}

streamTrace nal_reader::read_trace_line() {
  string line;
  getline(trace,line);
  BOOST_LOG_SEV(basic_lg, log::trace) << "Reading trace line"
				      << ". Raw line: "
				      << line;

  // Skip empty lines after this
  string nextline;
  size_t oldpos;
  while (nextline.empty() && trace) {
    oldpos = trace.tellg();
    getline(trace, nextline);
  }
  if (!nextline.empty()) {
    trace.seekg(oldpos);
  }

  if (line.empty()) {
    throw runtime_error("Empty trace line");
  }

  istringstream iss(line);
  streamTrace elem;

  iss >> std::hex >> elem.startPos;
  if (iss.fail()) { // Was not a packet line, ignore and take next one
    BOOST_LOG_SEV(basic_lg, log::trace) << "Skip non-packet line";
    trace.clear();
    return read_trace_line();
  }

  iss >> std::dec;
  iss >> elem.len;
  iss >> elem.lid;
  iss >> elem.tid;
  iss >> elem.qid;

  string pkt_type;
  iss >> std::ws; // Skip whitespace
  getline(iss, pkt_type, ' ');
  BOOST_LOG_SEV(basic_lg, log::trace) << "Raw pkt_type=" << pkt_type;
  if (pkt_type == "StreamHeader") {
    elem.packetType = streamTrace::stream_header;
  } else if (pkt_type == "ParameterSet") {
    elem.packetType = streamTrace::parameter_set;
  } else if (pkt_type == "SliceData") {
    elem.packetType = streamTrace::slice_data;
  }
  else throw runtime_error("Unknown packet type");

  string disc;
  iss >> std::ws; // Skip whitespace
  getline(iss, disc, ' ');
  BOOST_LOG_SEV(basic_lg, log::trace) << "Raw disc=" << disc;
  if (disc == "Yes") {
    elem.discardable = true;
  } else if (disc == "No") {
    elem.discardable = false;
  }
  else throw runtime_error("Unknown discardable field");

  string trunc;
  iss >> std::ws; // Skip whitespace
  getline(iss, trunc, ' ');
  BOOST_LOG_SEV(basic_lg, log::trace) << "Raw trunc=" << trunc;
  if (trunc == "Yes") {
    elem.truncatable = true;
  } else if (trunc == "No") {
    elem.truncatable = false;
  }
  else throw runtime_error("Unknown truncatable field");

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

void nal_reader::read_header() {
  BOOST_LOG_SEV(basic_lg, log::trace) << "Reading header";
  streamTrace st_line = read_trace_line();
  buffer_type nal = read_nal(st_line);
  while (st_line.packetType == streamTrace::stream_header ||
	 st_line.packetType == streamTrace::parameter_set) {
    BOOST_LOG_SEV(basic_lg, log::trace) << "Found packet with type "
					<< st_line.packetType
					<< " offset="
					<< std::hex << st_line.startPos;
    hdr.insert(hdr.end(), nal.begin(), nal.end());
    BOOST_LOG_SEV(basic_lg, log::trace) << "Read into header buffer"
					<< ". Total size=" << hdr.size();
    st_line = read_trace_line();
    nal = read_nal(st_line);
  }

  // Don't drop the first slice NAL
  assert(last_nal.empty());
  last_nal = std::move(nal);
  last_prio = classify(st_line);
}

buffer_type nal_reader::read_nal(const streamTrace &st) {
  buffer_type buf(st.len);
  if (file.tellg() != st.startPos) {
    file.seekg(st.startPos);
  }
  file.read(buf.data(), buf.size());
  assert(file.gcount() == st.len);
  return buf;
}

void nal_reader::pack_nals() {
  if (last_nal.empty()) throw runtime_error("Out of NALs");

  std::size_t prio = last_prio;
  buffer_type packed = std::move(last_nal);
  last_nal.clear();

  while(trace) {
    streamTrace st_line = read_trace_line();
    last_nal = read_nal(st_line);
    last_prio = classify(st_line);

    // Append the next NAL if same priority
    if (last_prio == prio && packed.size() <= MAX_PACKED_SIZE) {
      packed.insert(packed.end(), last_nal.begin(), last_nal.end());
      last_nal.clear();
    }
    else { // Different priority
      break;
    }
  }

  // Segment and pad the packed NALs
  size_t npkts = static_cast<size_t>(ceil(static_cast<double>(packed.size()) /
					  pkt_size));
  auto i = packed.cbegin();
  for (size_t n = 0; n < npkts-1; ++n) {
    fountain_packet fp;
    fp.setPriority(prio);
    fp.buffer().resize(pkt_size);
    std::copy(i, i + pkt_size, fp.buffer().begin());
    pkt_queue.push(std::move(fp));
    i += pkt_size;
  }
  fountain_packet fp;
  fp.setPriority(prio);
  fp.buffer().resize(pkt_size, 0x00); // Pad with zeros the last segment
  std::copy(i, packed.cend(), fp.buffer().begin());
  pkt_queue.push(std::move(fp));
}

fountain_packet nal_reader::next_packet() {
  if (pkt_queue.empty()) {
    pack_nals();
  }

  fountain_packet fp = std::move(pkt_queue.front());
  pkt_queue.pop();
  return fp;
}

/*
	fountain_packet next_packet() { // using uep_encoder
		if (currInd[currQid]*Ls >= max_count[currQid]) throw std::runtime_error("Max packet count");
		std::string streamN = "dataset/"+streamName+"."+std::to_string(currQid)+".264";
		bool readingOk;
		std::vector<char> read = readByteFromFile(streamN,currInd[currQid]*Ls,Ls, &readingOk);
		currInd[currQid]++;
		if (!readingOk) throw std::runtime_error("Impossible to read source file");
		if (read.size() < Ls) { // last bits
			for (uint i=0; i<Ls-read.size(); i++) read.push_back(' ');
		}
		fountain_packet fp(read);
		fp.setPriority(currQid);
		cerr << "packet_source::next_packet size=" << fp.size()
      << " priority=" << static_cast<size_t>(fp.getPriority())
      << endl;
    if (currInd[currQid] % Ks[currQid] == 0) {
			currQid++;
			std::cout << "changing Qid\n";
		}
		if (currQid == Ks.size()) {
			currQid = 0;
			std::cout << "resetting Qid\n";
		}
		return fp;
	}
	*/

std::size_t nal_reader::classify(const streamTrace &st) {
  if (st.discardable && st.qid > 0) {
    return 1;
  }
  else {
    return 0;
  }
}

const buffer_type &nal_reader::header() const {
  return hdr;
}

bool nal_reader::has_packet() const {
  return !(pkt_queue.empty() && last_nal.empty() && trace.eof()) ;
}

nal_reader::operator bool() const {
  return has_packet();
}

size_t nal_reader::totLength() const {
	return totalLength;
}

bool nal_reader::operator!() const {
  return !has_packet();
}

}
