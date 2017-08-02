#include <ctime>
#include <iostream>
#include <string>
//#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "controlMessage.pb.h"
#include <boost/array.hpp>
#include "encoder.hpp"

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

using boost::asio::ip::tcp;
using namespace uep;
using namespace uep::net;

// DEFAULT PARAMETER SET
struct all_params: public robust_lt_parameter_set, public lt_uep_parameter_set {
	all_params() {
		K = 8;
		c = 0.1;
		delta = 0.01;
		RFM = 3;
		RFL = 1;
		EF = 2;
	}
	std::string streamName;
	bool ack = true;
	double sendRate = 10240;
	size_t fileSize = 20480;
	int tcp_port_num = 12312;
	/* PARAMETERS CHOICE */ 
	int Ls = 64;
};
all_params ps;

// PACKET SOURCE
struct packet_source {
	typedef all_params parameter_set;
	int Ls;
	int rfm;
	int rfl;
	int ef;
	size_t max_count;
	size_t currInd;
	int rfmReal;
	int rflReal;
	int efReal;
	std::string streamName;
	std::ifstream file;
	explicit packet_source(const parameter_set &ps) {
		Ls = ps.Ls;
		rfm = ps.RFM;
		rfl = ps.RFL;
		ef = ps.EF;
		max_count = ps.fileSize;
		currInd = 0;
		rfmReal = 0;
		rflReal = 0;
		efReal = 0;
		streamName = ps.streamName;
		/* RANDOM GENERATION OF FILE */
		int fileSize = max_count; // file size = fileSize * Ls;
		std::ofstream newFile (streamName+".txt");
		if (newFile.is_open()) { 
			for (int ii = 0; ii<fileSize; ii++) {
				boost::random::uniform_int_distribution<> dist(0, 255);
				newFile << ((char) dist(gen));
			}
		}
		newFile.close();

		file = std::ifstream(streamName+".txt", std::ios::in|std::ios::binary|std::ios::ate);
		if (!file.is_open()) throw std::runtime_error("Failed opening file");
		else max_count = file.tellg();
	}

	packet next_packet() {
		if (currInd >= max_count) throw std::runtime_error("Max packet count");
		if (efReal<ef) {
			if (rfmReal<rfm) rfmReal++;
			else {
				if (rfmReal==rfm) {
					rfmReal++;
					currInd += Ls;
				} 
				if (rflReal<rfl) rflReal++;
				else {
					rfmReal = 0;
					rflReal = 0;
					efReal++;
					currInd -= Ls;
				}
			}	
		} else {
			currInd += 2*Ls;
			rfmReal = 0;
			rflReal = 0;
			efReal = 0;
		}
		file.seekg (currInd, std::ios::beg);
		char * memblock;
		memblock = new char [Ls];
		file.read (memblock, Ls);
		packet p;
		p.resize(Ls);
		p.assign(memblock, Ls+memblock);
		/*for (int i=0; i < Ls; i++) {
			p[i] = memblock[i];
		}*/	
		return p;
	}

	explicit operator bool() const {
		return currInd < max_count;
	}

	bool operator!() const { return !static_cast<bool>(*this); }
	
	boost::random::mt19937 gen;
};

class tcp_connection: public boost::enable_shared_from_this<tcp_connection> {
	/* 	Using shared_ptr and enable_shared_from_this 
		because we want to keep the tcp_connection object alive 
		as long as there is an operation that refers to it.*/
	public:
		typedef boost::shared_ptr<tcp_connection> pointer;
		typedef uep::net::data_server<lt_encoder<std::mt19937>,packet_source> ds_type;
		static pointer create(boost::asio::io_service& io_service) {
			return pointer(new tcp_connection(io_service));
		}

		tcp::socket& socket() {
			return socket_;
		}

		void firstHandler(const boost::system::error_code& error, std::size_t bytes_transferred ) {
			std::string s = std::string(buf.data(), bytes_transferred);
			controlMessage::StreamName firstMessage;
			if (firstMessage.ParseFromString(s)) {
				s = firstMessage.streamname();
			}
			std::cout << "Stream name received from client: \"" << s << "\"\n";
			ps.streamName = s;
			
			/* CREATION OF DATA SERVER */
			std::cout << "Creation of encoder...\n";
			ds_type *ds = new ds_type(io);
			ds_p.reset(ds);
			ds->setup_encoder(ps); // setup the encoder inside the data_server
			ds->setup_source(ps); // setup the source  inside the data_server
			ds->target_send_rate(ps.sendRate); // Set a target send rate of 10240 byte/s = 10 pkt/s
			ds->enable_ack(ps.ack);
			
			// SENDING ENCODER PARAMETERS
			controlMessage::TXParam secondMessage;
			secondMessage.set_k(ps.K);
			secondMessage.set_c(ps.c);
			secondMessage.set_delta(ps.delta);
			secondMessage.set_rfm(ps.RFM);
			secondMessage.set_rfl(ps.RFL);
			secondMessage.set_ef(ps.EF);
			secondMessage.set_ack(ps.ack);
			secondMessage.set_filesize(ps.fileSize);

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
			std::cout << "Binding of data server on: "<<remAddr<<":"<<port<<"\n";
			ds_p->open(remAddr, portStr);
			//boost::asio::ip::udp::endpoint udpEndpoint = ds_p->server_endpoint();
			uint32_t udpPort = ds_p->server_endpoint().port(); // =(udp port for ACK to server)
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
			ds_p->start();
			std::cout << "Called start" << std::endl;
		}

		void start() {	
			boost::system::error_code error;
			socket_.async_read_some(boost::asio::buffer(buf), std::bind(
				&tcp_connection::firstHandler,
				shared_from_this(),
				std::placeholders::_1, std::placeholders::_2));

			if (error == boost::asio::error::eof)
				std::cout.write("error",5); // Connection closed cleanly by peer.
			else if (error)
				throw boost::system::system_error(error); // Some other error.
			//std::cout << buf.data() << std::endl;

		}

	private:
		boost::array<char,128> buf;
		std::unique_ptr<ds_type> ds_p;
		tcp::socket socket_;
		boost::asio::io_service& io;
		
		//uep::lt_encoder<std::mt19937> enc;
		//uep::net::data_server<lt_encoder<std::mt19937>,random_packet_source> ds;
		//boost::asio::io_service io;
		tcp_connection(boost::asio::io_service& io_service): socket_(io_service), io(io_service) {
			
		}
		// handle_write() is responsible for any further actions 
		// for this client connection.
		void handle_write(const boost::system::error_code& /*error*/,size_t /*bytes_transferred*/) {

		}

};

class tcp_server {
	public:
		// Constructor: initialises an acceptor to listen on TCP port tcp_port_num.
		tcp_server(boost::asio::io_service& io_service): 
			acceptor_(io_service, tcp::endpoint(tcp::v4(), ps.tcp_port_num)) {
			// start_accept() creates a socket and 
			// initiates an asynchronous accept operation 
			// to wait for a new connection.
			start_accept();
		}

	private:
		void start_accept() {
			// creates a socket
			tcp_connection::pointer new_connection =
			tcp_connection::create(acceptor_.get_io_service());
			active_conns.push_front(new_connection);

			// initiates an asynchronous accept operation 
			// to wait for a new connection. 
			acceptor_.async_accept(new_connection->socket(),
				std::bind(&tcp_server::handle_accept, this, new_connection,std::placeholders::_1));
		}

		// handle_accept() is called when the asynchronous accept operation 
		// initiated by start_accept() finishes. It services the client request
		void handle_accept(tcp_connection::pointer new_connection,const boost::system::error_code& error) {
			if (!error) {
				new_connection->start();
			}
			// Call start_accept() to initiate the next accept operation.
			start_accept();
	
		}

		tcp::acceptor acceptor_;

  std::list<boost::shared_ptr<tcp_connection>> active_conns;
};

int main() {
	try {
		// We need to create a server object to accept incoming client connections.
		boost::asio::io_service io_service;

		// The io_service object provides I/O services, such as sockets, 
		// that the server object will use.
		tcp_server server(io_service);

		// Run the io_service object to perform asynchronous operations.
		io_service.run();
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
