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

typedef all_parameter_set<uep_encoder<>::parameter_set> all_params;
all_params parset;


inline bool file_exists (const std::string& name) {
  std::ifstream f(name.c_str());
	bool out = f.good();
	f.close();
	return out;
}
inline bool file_exists (const std::string& name, size_t *length) {
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

std::vector<streamTrace> videoTrace;

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
						elem.packetType = 1;
					} else if (s == "ParameterSet") {
						elem.packetType = 2;
					} else if (s == "SliceData") {
						elem.packetType = 3;
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
std::vector<char> header;
int headerSize;
int sliceDataInd;
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
	std::string streamName;
	std::vector<std::ifstream> files;

	explicit packet_source(const parameter_set &ps) {
		Ks = ps.Ks;
		rfs = ps.RFs;
		ef = ps.EF;
		currInd.resize(Ks.size());
		currRep.resize(Ks.size());
    files.resize(Ks.size());
    max_count.resize(Ks.size());
		for (uint i=0; i<currInd.size(); i++) { currInd[i]=0; currRep[i]=0; max_count[i] = 0;}
		currQid = 0;
		efReal = 0;
		streamName = ps.streamName;
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
		videoTrace = loadTrace(streamName);
		//max_count = videoTrace[videoTrace.size()-1].startPos + videoTrace[videoTrace.size()-1].len;
		//std::cout << "max_count: " << std::to_string(max_count) << std::endl;
		// parse .trace to produce a txt with parts repeated
		// first rows of videoTrace are: stream header and parameter set. must be passed through TCP
		int fromHead = videoTrace[0].startPos;
		int toHead;
		headerSize = 0;
		for (toHead = fromHead; videoTrace[toHead].packetType < 3; toHead++) { headerSize += videoTrace[toHead].len; }
		//int fromSliceData = videoTrace[toHead].startPos;
		sliceDataInd = toHead;
		toHead = videoTrace[toHead-1].startPos + videoTrace[toHead-1].len;
		// int fromSliceData = toHead + 1;

		header = readByteFromFile("dataset/"+streamName+".264",fromHead,headerSize);

		for (uint8_t i=0; i<Ks.size(); i++) {
      std::string streamN = "dataset/"+streamName+"."+std::to_string(i)+".264";
      size_t leng=0;
			if (file_exists(streamN, &leng)) {
				std::cout << streamN << " already created. Length: "<< leng <<"\n";
			} else {
				uint ii=sliceDataInd;
				while (ii<videoTrace.size()) {
					if (((videoTrace[ii].packetType == 3) && (videoTrace[ii].qid == i))||
						((uint(videoTrace[ii].qid) >= ps.Ks.size()) && (i == ps.Ks.size()-1))) {
						//std::vector<char> slice;
	    			std::vector<char> slice = readByteFromFile("dataset/"+streamName+".264",videoTrace[ii].startPos,videoTrace[ii].len);
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
			std::string streamN = "dataset/"+streamName+"."+std::to_string(i)+".264";
			files[i] = std::ifstream(streamN, std::ios::in|std::ios::binary);
			if (!files[i].is_open()) throw std::runtime_error("Failed opening file");
		}

	}

	/*fountain_packet next_packet() { // next_packet using lt_encoder
		if (currInd[currQid] >= max_count) throw std::runtime_error("Max packet count");
		if (efReal<ef) {
			if (currQid < Ks.size()) {
				if (currRep[currQid]<rfs[currQid]) {
					currRep[currQid]++;
				} else {
					currRep[currQid] = 0;
					currQid++;
				}
			}
			if (currQid == Ks.size()) {
				currRep[currQid] = 0;
				efReal++;
				currQid = 0;
			}
		}
		if (efReal == ef) {
			currQid = 0;
			efReal = 0;
			for (uint i=0; i<Ks.size(); i++) {
				currRep[i] = 0;
				currInd[i] += Ks[i];
			}
		}
		std::string streamN = "dataset/"+streamName+"."+std::to_string(currQid)+".264";
		std::vector<char> read = readByteFromFile(streamN,currInd[currQid],Ks[currQid]);
		fountain_packet fp(read);
		fp.setPriority(currQid);
		return fp;
	}*/
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

	explicit operator bool() const {
		bool out = true;
		for (uint i=0; i<currInd.size(); i++) {
			out = out && (currInd[i] < max_count[i]);
		}
		return out;
	}

	bool operator!() const { return !static_cast<bool>(*this); }

	boost::random::mt19937 gen;
};


namespace uep {
size_t totalLength=0;
nal_reader::nal_reader(const parameter_set &ps) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance) {
  stream_name = ps.streamName;
  BOOST_LOG_SEV(basic_lg, log::trace) << "Creating reader for \""
				      << stream_name << "\"";
	file.open("dataset/"+stream_name+".264", ios_base::in|ios_base::binary);
	file.seekg(0, ios::end);
	totalLength = file.tellg();
	file.seekg(0, ios::beg);
  trace.open("dataset/"+stream_name+".trace", ios_base::in);
  BOOST_LOG_SEV(basic_lg, log::trace) << "Opened stream and trace files";

  read_header(); // Read header NALs into hdr
}

streamTrace nal_reader::read_trace_line() {
  string line;
  getline(trace,line);

  BOOST_LOG_SEV(basic_lg, log::trace) << "Reading trace line"
				      << ". Raw line: "
				      << line;

  if (line.empty()) {
    throw runtime_error("Empty trace line");
  }

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

  BOOST_LOG_SEV(basic_lg, log::trace) << "Trimmed line: "
				      << whiteSpacesTrimmed;

  istringstream iss(whiteSpacesTrimmed);
  streamTrace elem;
  iss >> std::hex >> elem.startPos;
  if (iss.fail()) { // Was not a packet line, take next one
    BOOST_LOG_SEV(basic_lg, log::trace) << "Skip non-packet line";
    trace.clear();
    return read_trace_line();
  }

  iss >> std::dec;
  iss >> elem.len;
  iss >> elem.lid;
  iss >> elem.tid;
  iss >> elem.qid;

  BOOST_LOG_SEV(basic_lg, log::trace) << "Read offset="
				      << std::hex << elem.startPos
				      << " length="
				      << std::dec << elem.len
				      << " Lid=" << elem.lid
				      << " Tid=" << elem.tid
				      << " Qid=" << elem.qid;

  string pkt_type;
  iss >> std::ws; // Skip whitespace
  getline(iss, pkt_type, ' ');
  BOOST_LOG_SEV(basic_lg, log::trace) << "Raw pkt_type=" << pkt_type;
  if (pkt_type == "StreamHeader") {
    elem.packetType = 1;
  } else if (pkt_type == "ParameterSet") {
    elem.packetType = 2;
  } else if (pkt_type == "SliceData") {
    elem.packetType = 3;
  }
  else throw runtime_error("Unknown packet type");

  string disc;
  getline(iss, disc, ' ');
  if (disc == "Yes") {
    elem.discardable = true;
  } else if (disc == "No") {
    elem.discardable = false;
  }
  else throw runtime_error("Unknown discardable field");

  string trunc;
  getline(iss, trunc, ' ');
  if (trunc == "Yes") {
    elem.discardable = true;
  } else if (trunc == "No") {
    elem.discardable = false;
  }
  else throw runtime_error("Unknown truncatable field");

  return elem;
}

void nal_reader::read_header() {
  BOOST_LOG_SEV(basic_lg, log::trace) << "Reading header";
  streamTrace st_line = read_trace_line();
  buffer_type nal;
  while (st_line.packetType == 1 || st_line.packetType == 2) {
    BOOST_LOG_SEV(basic_lg, log::trace) << "Found packet with type "
					<< st_line.packetType
					<< " offset="
					<< std::hex << st_line.startPos;
    nal = read_nal(st_line);
    hdr.insert(hdr.end(), nal.begin(), nal.end());
    BOOST_LOG_SEV(basic_lg, log::trace) << "Read into header buffer"
					<< ". Total size=" << hdr.size();
    st_line = read_trace_line();
  }
  // Don't drop the first slice NAL
  if (last_nal.empty()) {
    BOOST_LOG_SEV(basic_lg, log::trace) << "Read the first NAL";
    last_nal = read_nal(st_line);
    next_chunk = last_nal.cbegin();
    if (st_line.discardable && st_line.qid > 0) {
      last_priority = 1;
    }
    else {
      last_priority = 0;
    }
  }
}

buffer_type nal_reader::read_nal(const streamTrace &st) {
  buffer_type buf(st.len);
  file.seekg(st.startPos);
  file.read(buf.data(), buf.size());
  assert(file.gcount() == st.len);
  return buf;
}

fountain_packet nal_reader::next_packet() {
  if (!has_packet()) throw runtime_error("Out of packets");
  if (last_nal.empty()) {
    BOOST_LOG_SEV(basic_lg, log::trace) << "Read a new NAL";
    streamTrace st = read_trace_line();
    last_nal = read_nal(st);
    next_chunk = last_nal.cbegin();
    if (/*st.discardable &&*/ st.qid > 0) {
      last_priority = 1;
    }
    else {
      last_priority = 0;
    }
  }

  // Size of NAL chunk to extract
  std::size_t chunk_size = min(static_cast<size_t>(last_nal.cend() - next_chunk),
			       pkt_size);
  BOOST_LOG_SEV(basic_lg, log::trace) << "Build a pkt of " << chunk_size
				      << " bytes"
				      << " priority " << last_priority;

  fountain_packet fp;
  fp.buffer().assign(next_chunk, next_chunk + chunk_size);
  next_chunk += chunk_size;
  fp.setPriority(last_priority);

  if (next_chunk == last_nal.cend()) {
    last_nal.clear();
  }

  fp.buffer().resize(pkt_size, 0); // Pad with zeros
  return fp;
}

const buffer_type &nal_reader::header() const {
  return hdr;
}

bool nal_reader::has_packet() const {
  return !trace.eof();
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
