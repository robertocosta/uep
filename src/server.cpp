#include <ctime>
#include <iostream>
#include <string>
#include<boost/algorithm/string.hpp>
#include <sstream>
#include <fstream>
#include <codecvt>
#include <set>

//#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "controlMessage.pb.h"
#include <boost/array.hpp>
//#include "encoder.hpp"
#include "uep_encoder.hpp"
#include "log.hpp"
#include <ostream>
#include <boost/iostreams/device/file.hpp>
#include <fstream>
#include <boost/iostreams/stream.hpp>

#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include "data_client_server.hpp"
/*
	1: client to server: streamName
	2: server to client: TXParam
		2.1 decoder parameters
			2.1.1 K	size_t (unsigned long)
			2.1.2 c (double)
			2.1.3 delta (double)
			2.1.4 RFM (uint8_t)
			2.1.5 RFL (uint8_t)
			2.1.6 EF (uint8_t)
		2.2 ACK enabled
		2.3 File size
	3: client to server: Connect
		3.1 udp port where to send data
		When the server receives it the encoder must be created
	4: server to client: ConnACK
		4.1 udp port where to receive ack
	5: client to server: Play
		DataServer.start
*/

inline bool file_exists (const std::string& name) {
		std::ifstream f(name.c_str());
	bool out = f.good();
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

using namespace std;
using boost::asio::ip::tcp;
using namespace uep;
using namespace uep::net;
using namespace uep::log;

// DEFAULT PARAMETER SET
typedef all_parameter_set<uep_encoder<>::parameter_set> all_params;
all_params ps;

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
	uint ef;
	size_t max_count;
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
		for (uint i=0; i<currInd.size(); i++) { currInd[i]=0; currRep[i]=0; }
		currQid = 0;
		efReal = 0;
		streamName = ps.streamName;

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
		max_count = videoTrace[videoTrace.size()-1].startPos + videoTrace[videoTrace.size()-1].len;
		std::cout << "max_count: " << std::to_string(max_count) << std::endl;
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
			if (file_exists(streamN)) {
				std::cout << streamN << " already created.\n";
			} else {
				uint ii=sliceDataInd;
				while (ii<videoTrace.size()) {
					if (((videoTrace[ii].packetType == 3) && (videoTrace[ii].qid == i))||
						((videoTrace[ii].qid >= ps.Ks.size()) && (i == ps.Ks.size()-1))) {
						//std::vector<char> slice;
						std::vector<char> slice = readByteFromFile("dataset/"+streamName+".264",videoTrace[ii].startPos,videoTrace[ii].len);
						if (!writeCharVecToFile(streamN,slice)) {
							std::cout << "error in writing file\n";
						}
						//std::cout << "read " << ii << "th row - qid: " << std::to_string(i) << " -> " << streamN << std::endl;
					}
					ii++;
				}
			}
		}

		for (uint8_t i=0; i<Ks.size(); i++) {
			std::string streamN = "dataset/"+streamName+"."+std::to_string(i)+".264";
			files[i] = std::ifstream(streamN, std::ios::in|std::ios::binary);
			if (!files[i].is_open()) throw std::runtime_error("Failed opening file");
		}

	}

	fountain_packet next_packet() {
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
		std::cout << streamN << ": from " << std::to_string(currInd[currQid]) << ", length: " << std::to_string(Ks[currQid]) << std::endl;
		/*
		std::cout << "next_packet():" << std::endl;
		for (uint i=0; i < read.size(); i++) {
			std::cout << read[i];
		}
		std::cout << std::endl;*/
		fountain_packet fp(read);
		//std::cout << "currQid: " << std::to_string(currQid) << "; Ks.size(): " << std::to_string(Ks.size()) << std::endl;
		fp.setPriority(currQid);

		cerr << "packet_source::next_packet size=" << fp.size()
		     << " priority=" << static_cast<size_t>(fp.getPriority())
		     << endl;

		return fp;
	}

	explicit operator bool() const {
		bool out = true;
		for (uint i=0; i<currInd.size(); i++) {
			out = out && (currInd[i] < max_count);
		}
		return out;
	}

	bool operator!() const { return !static_cast<bool>(*this); }

	boost::random::mt19937 gen;
};

class tcp_connection: public boost::enable_shared_from_this<tcp_connection> {
	/*	Using shared_ptr and enable_shared_from_this
			because we want to keep the tcp_connection object alive
			as long as there is an operation that refers to it.*/
public:
	typedef boost::shared_ptr<tcp_connection> pointer;
	typedef uep_encoder<> enc_t;
	typedef packet_source src_t;
	typedef uep::net::data_server<enc_t,src_t> ds_type;

		static pointer create(boost::asio::io_service& io_service) {
			return pointer(new tcp_connection(io_service));
		}

		tcp::socket& socket() {
			return socket_;
		}

	void start() {
		boost::system::error_code error;
		socket_.async_read_some(boost::asio::buffer(buf),
														std::bind(&tcp_connection::firstHandler,
																			shared_from_this(),
																			std::placeholders::_1,
																			std::placeholders::_2));

		if (error == boost::asio::error::eof)
			std::cerr << "error" << std::endl; // Connection closed cleanly by peer.
		else if (error)
			throw boost::system::system_error(error); // Some other error.
		//std::cout << buf.data() << std::endl;
	}

private:
	boost::asio::io_service& io;
	tcp::socket socket_;
	ds_type ds;

	all_params ps;

	std::array<char,4096> buf;

	tcp_connection(boost::asio::io_service& io_service) :
		io(io_service), socket_(io_service), ds(io_service) {
		// set default values from global ps
		ps = ::ps;
	}

		void firstHandler(const boost::system::error_code& error, std::size_t bytes_transferred ) {
			std::string s = std::string(buf.data(), bytes_transferred);
			// Protobuf messages are not delimited: this works when a single
			// message is received in one segment
			controlMessage::StreamName firstMessage;
			if (firstMessage.ParseFromString(s)) {
				s = firstMessage.streamname();
			}
			std::cout << "Stream name received from client: \"" << s << "\"\n";
			ps.streamName = s;

			/* CREATION OF DATA SERVER */
			//BOOST_LOG_SEV(basic_lg, debug) << "Creation of encoder...\n";
			std::cout << "Creation of encoder...\n";
			enc_t::parameter_set enc_ps(ps); // extract only the encoder params
			src_t::parameter_set src_ps(ps); // extract only the source params
			ds.setup_encoder(enc_ps); // setup the encoder inside the data_server
			ds.setup_source(src_ps); // setup the source  inside the data_server

			ds.target_send_rate(ps.sendRate);
			ds.enable_ack(ps.ack);

			// SENDING ENCODER PARAMETERS
			controlMessage::TXParam secondMessage;
			secondMessage.add_ks(ps.Ks[0]);
			secondMessage.add_ks(ps.Ks[1]);
			secondMessage.set_c(ps.c);
			secondMessage.set_delta(ps.delta);
			secondMessage.add_rfs(ps.RFs[0]);
			secondMessage.add_rfs(ps.RFs[1]);

			secondMessage.set_ef(ps.EF);
			secondMessage.set_ack(ps.ack);
			secondMessage.set_filesize(ps.fileSize);

			for (int i=0; i<headerSize; i++) {
				secondMessage.add_header(std::to_string(header[i]));
			}

			if (secondMessage.SerializeToString(&s)) {
				std::cout << "Sending encoder's parameters to client...\n";
				/*	Call boost::asio::async_write() to serve the data to the client.
					We are using boost::asio::async_write(),
					rather than ip::tcp::socket::async_write_some(),
					to ensure that the entire block of data is sent.*/
				boost::asio::async_write(socket_, boost::asio::buffer(s),std::bind(
					&tcp_connection::handle_write,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2));
				// waiting for Connection request, then secondHandler
				boost::system::error_code error;
				socket_.async_read_some(boost::asio::buffer(buf), std::bind(
					&tcp_connection::secondHandler,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2));
				if (error == boost::asio::error::eof)
					std::cout.write("error",5); // Connection closed cleanly by peer.
				else if (error)
					throw boost::system::system_error(error); // Some other error.
			}
		}

		void secondHandler(const boost::system::error_code& error, std::size_t bytes_transferred ) {
			//	data received stored in string s
			std::string s = std::string(buf.data(), bytes_transferred);
			//	converting received data to udp port number of the client
			controlMessage::Connect connectMessage;
			uint32_t port;
			if (connectMessage.ParseFromString(s)) {
				port = connectMessage.port();
			}
			else throw std::runtime_error("No port");
			std::cout << "Connect req. received from client on port " << port << "\n";
			// Opening UDP connection
			char portStr[10];
			sprintf(portStr, "%u", port);
			/*
			asio::ip::address remote_ad = socket_.remote_endpoint().address();
			std::string s = remote_ad.to_string();
			*/
			// GET REMOTE ADDRESS FROM TCP CONNECTION
			std::string remAddr = socket_.remote_endpoint().address().to_string();
			std::cout << "Binding of data server to "<<remAddr<<":"<<port<<"\n";
			ds.open(remAddr, portStr);
			//boost::asio::ip::udp::endpoint udpEndpoint = ds_p->server_endpoint();
			uint32_t udpPort = ds.server_endpoint().port(); // =(udp port for ACK to server)
			std::cout << "UDP port for ACKs: "<< udpPort << std::endl;
			std::cout << "Sending to client..." << std::endl;
			controlMessage::ConnACK connACKMessage;
			connACKMessage.set_port(udpPort);
			if (connACKMessage.SerializeToString(&s)) {
				boost::asio::async_write(socket_, boost::asio::buffer(s),std::bind(
					&tcp_connection::handle_write,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2));
				std::cout << "Connection ACK sent...\n";
				// waiting for play command
				boost::system::error_code error;
				socket_.async_read_some(boost::asio::buffer(buf), std::bind(
					&tcp_connection::thirdHandler,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2));
			}
		}

		void thirdHandler(const boost::system::error_code& error, std::size_t bytes_transferred ) {
			std::string s = std::string(buf.data(), bytes_transferred);
			// s should be empty
			std::cout << "PLAY.\n";
			ds.start();
			std::cout << "Called start" << std::endl;
		}

		// handle_write() is responsible for any further actions
		// for this client connection.
		void handle_write(const boost::system::error_code& /*error*/,size_t /*bytes_transferred*/) {

		}

};

class tcp_server {
public:
	// Constructor: initialises an acceptor to listen on TCP port tcp_port_num.
	tcp_server(boost::asio::io_service& io_service, const std::string &port) :
		acceptor_(io_service) {
		tcp::resolver resolver(io_service);
		tcp::resolver::query listen_q(port);
		tcp::endpoint ep = *resolver.resolve(listen_q); // Take the first one

		acceptor_.open(tcp::v4());
		acceptor_.bind(ep);
		acceptor_.listen();

		// start_accept() creates a socket and
		// initiates an asynchronous accept operation
		// to wait for a new connection.
		start_accept();
	}

private:
	tcp::acceptor acceptor_;
	std::set<tcp_connection::pointer> active_conns;

	void start_accept() {
		// creates a socket
		tcp_connection::pointer new_connection =
			tcp_connection::create(acceptor_.get_io_service());

		// initiates an asynchronous accept operation
		// to wait for a new connection.
		acceptor_.async_accept(new_connection->socket(),
													 std::bind(&tcp_server::handle_accept,
																		 this,
																		 new_connection,
																		 std::placeholders::_1));
	}

	// handle_accept() is called when the asynchronous accept operation
	// initiated by start_accept() finishes. It services the client request
	void handle_accept(tcp_connection::pointer new_connection, const boost::system::error_code& error) {
		if (!error) {
			active_conns.insert(new_connection);
			new_connection->start();
		}
		else {
			cerr << "Error on async_accept: " << error.message() << endl;
		}
		// Call start_accept() to initiate the next accept operation.
		start_accept();
	}
};

const string default_tcp_port = "12312";

int main(int argc, char **argv) {
	log::init("server.log");
	default_logger basic_lg(boost::log::keywords::channel = basic);
	default_logger perf_lg(boost::log::keywords::channel = performance);

	ps.EF = 2;
	ps.Ks = {256,512};
	ps.RFs = {2,1};
	ps.c = 0.1;
	ps.delta = 0.5;
	ps.ack = true;
	ps.sendRate = 10240;
	ps.fileSize = 20480;
	ps.tcp_port_num = default_tcp_port;

	string tcp_port = default_tcp_port;
	if (argc > 1) {
		tcp_port = argv[1];
	}
	if (argc > 2) {
		std::cerr << "Too many args" << std::endl;
		return 1;
	}

	// We need to create a server object to accept incoming client connections.
	boost::asio::io_service io_service;

	// The io_service object provides I/O services, such as sockets,
	// that the server object will use.
	tcp_server server(io_service, tcp_port);

	// Run the io_service object to perform asynchronous operations.
	BOOST_LOG_SEV(basic_lg, info) << "Run";
	io_service.run();
	BOOST_LOG_SEV(basic_lg, info) << "Stopped";

	return 0;
}

// Local Variables:
// tab-width: 2
// End: