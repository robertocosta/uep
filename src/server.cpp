#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind.hpp>
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

using boost::asio::ip::tcp;
int port_num = 12312;
/* PARAMETERS CHOICE */
int k = 8; 
double c = 0.1;
double delta = 0.01;
int rfm = 3;
int rfl = 1;
int ef = 2;

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

class tcp_connection: public boost::enable_shared_from_this<tcp_connection> {
	/* 	Using shared_ptr and enable_shared_from_this 
		because we want to keep the tcp_connection object alive 
		as long as there is an operation that refers to it.*/
	public:
		typedef boost::shared_ptr<tcp_connection> pointer;

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

			/* GENERATION OF FILE */
			int fileLength = 10240;
			int MIp = 3; // most important part, # of bytes
			boost::iostreams::stream_buffer<boost::iostreams::file_sink> streamBuf(s+".txt");
			std::ostream outStr(&streamBuf);
			/*
			std::string chars(
					"abcdefghijklmnopqrstuvwxyz"
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"1234567890");
			boost::random::mt19937 rng;
			boost::random::uniform_int_distribution<> index_dist(0, chars.size() - 1);
			*/
			for (int ii = 0; ii<fileLength; ii++) {
				// 8-bit word generation
				//outStr << chars[index_dist(rng)];
				boost::random::uniform_int_distribution<> dist(0, 255);
				outStr << (char) dist(gen);
			}
			//outStr << std::endl;
			
			/* UEP */
			std::streampos size;
			char * memblock;
			char * firstPart;
			char * secondPart;
			std::ifstream file (s+".txt", std::ios::in|std::ios::binary|std::ios::ate);
			std::ofstream newFile (s+"-uep.txt");
			if (newFile.is_open()) {
				if (file.is_open()) {
					size = file.tellg();
					memblock = new char [k];
					firstPart = new char[MIp];
					secondPart = new char[k-MIp];
					for (uint32_t ind = 0; ind+k < size; ind = ind + k) {
						file.seekg (ind, std::ios::beg);
						file.read (memblock, k);
						strncpy(firstPart, memblock, MIp);
						strncpy(secondPart, memblock+MIp, k-MIp);
						for (int jj=0; jj<ef; jj++) {
							for (int ii=0; ii<rfm; ii++)
								newFile.write(firstPart,MIp);
							for (int ii=0; ii<rfl; ii++)
								newFile.write(secondPart,k-MIp);
						}
					}
					file.close();
					newFile.close();
					delete[] memblock;
					delete[] firstPart;
					delete[] secondPart;
				} else std::cout << "Unable to open file in reading mode";
			} else std::cout << "Unable to open file in writing mode";

			/* CREATION OF ENCODER */
			std::cout << "Creation of encoder...\n";
			uep::lt_encoder<std::mt19937> enc(k, c, delta);
			
			// SENDING ENCODER PARAMETERS
			controlMessage::TXParam secondMessage;
			secondMessage.set_k(k);
			secondMessage.set_c(c);
			secondMessage.set_delta(delta);
			secondMessage.set_rfm(rfm);
			secondMessage.set_rfl(rfl);
			secondMessage.set_ef(ef);
			secondMessage.set_ack(true);
			secondMessage.set_filesize(fileLength);

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
			std::cout << "Connect req. received from client on port \"" << port << "\"\n";
			std::cout << "Creation of data server...\n";
			//ds(io);
			// data_server (boost::asio::io_service &io)
			// void 	setup_encoder (const encoder_parameter_set &ps)
			// void 	setup_source (const source_parameter_set &ps)
			// void 	open (const std::string &dest_host, const std::string &dest_service)

			uint32_t udpPort = 1445; // =(udp port for ACK from new data_server())
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
		boost::array<char, 128> buf;
		//uep::lt_encoder<std::mt19937> enc;
		//uep::net::data_server<lt_encoder<std::mt19937>,random_packet_source> ds;
		tcp_connection(boost::asio::io_service& io_service): socket_(io_service) {
			
		}
		// handle_write() is responsible for any further actions 
		// for this client connection.
		void handle_write(const boost::system::error_code& /*error*/,size_t /*bytes_transferred*/) {

		}
		boost::random::mt19937 gen;
		boost::asio::io_service io;
		tcp::socket socket_;
};

class tcp_server {
	public:
		// Constructor: initialises an acceptor to listen on TCP port port_num.
		tcp_server(boost::asio::io_service& io_service): 
			acceptor_(io_service, tcp::endpoint(tcp::v4(), port_num)) {
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

			// initiates an asynchronous accept operation 
			// to wait for a new connection. 
			acceptor_.async_accept(new_connection->socket(),
				boost::bind(&tcp_server::handle_accept, this, new_connection,
					boost::asio::placeholders::error));
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
