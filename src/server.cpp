
	/* Rispondi con ACK a client
		e trasmette i parametri del decoder
		e i parametri della connessione
			1. se usare o meno gli ACK
			2. Dimensione del file
	Si mette in attesa del messaggio CONNECT(porta) dal client
	Ricevuto il quale crea l'encoder 
	E manda un ack al client con la porta usata dall'encoder (udp)
	Si mette in attesa del messaggio PLAY
	DataServer.start
	*/	

#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "controlMessage.pb.h"
#include <boost/array.hpp>

using boost::asio::ip::tcp;
int port_num = 12312;

std::string make_daytime_string() {
	std:: time_t now = std::time(0);
	return std::ctime(&now);
}

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
		4: server to client: ConnACK
			4.1 udp port where to receive ack
		5: client to server: Play
	*/
	//std::vector<item> v;

class tcp_connection: public boost::enable_shared_from_this<tcp_connection> {
  // Using shared_ptr and enable_shared_from_this 
  // because we want to keep the tcp_connection object alive 
  // as long as there is an operation that refers to it.
	
	public:
		typedef boost::shared_ptr<tcp_connection> pointer;

		static pointer create(boost::asio::io_service& io_service) {
			return pointer(new tcp_connection(io_service));
		}

		tcp::socket& socket() {
			return socket_;
		}

		// Call boost::asio::async_write() to serve the data to the client. 
		// We are using boost::asio::async_write(), 
		// rather than ip::tcp::socket::async_write_some(), 
		// to ensure that the entire block of data is sent.
		void firstHandler(const boost::system::error_code& error, std::size_t bytes_transferred ) {
			
		}
		void start() {
			// We use a boost::array to hold the received data. 
			boost::array<char, 128> buf;
			boost::system::error_code error;

			// The boost::asio::buffer() function automatically determines 
			// the size of the array to help prevent buffer overruns.
			socket_.async_read_some(boost::asio::buffer(buf), std::bind(&tcp_connection::firstHandler,this,std::placeholders::_1, std::placeholders::_2));
			
			// When the server closes the connection, 
			// the ip::tcp::socket::read_some() function will exit with the boost::asio::error::eof error, 
			// which is how we know to exit the loop.
			if (error == boost::asio::error::eof)
				std::cout.write("error",5); // Connection closed cleanly by peer.
			else if (error)
				throw boost::system::system_error(error); // Some other error.
			std::cout << buf.data() << std::endl;
			
		// The data to be sent is stored in the class member m_message 
			// as we need to keep the data valid 
			// until the asynchronous operation is complete.
			
			controlMessage::TXParam second;
			second.set_k(5);
			second.set_c(0.1);
			second.set_delta(0.5);
			second.set_rfm(2);
			second.set_rfl(1);
			second.set_ef(2);
			second.set_ack(true);
			second.set_filesize(10240);
			
			std::stringstream st;
			std::ostream* output = &st;
			std::string s;
			if (second.SerializeToOstream(output)) {
				s = st.str();
				try {
					s = s.substr(2,s.length()-2);
				} catch (const std::out_of_range& e) {
					std::cout << "Il messaggio deve essere di lunghezza maggiore di 1\n";
				}
				//std::cout << s << std::endl;
			}
			//std::cout << output << std::endl;

			m_message = s;

			// When initiating the asynchronous operation, 
			// and if using boost::bind(), 
			// we must specify only the arguments 
			// that match the handler's parameter list. 
			// In this code, both of the argument placeholders 
			// (boost::asio::placeholders::error 
			// and boost::asio::placeholders::bytes_transferred) 
			// could potentially have been removed, 
			// since they are not being used in handle_write().

			boost::asio::async_write(socket_, boost::asio::buffer(m_message),
				boost::bind(&tcp_connection::handle_write, shared_from_this(),
					boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
						
			

		}

	private:
		tcp_connection(boost::asio::io_service& io_service): socket_(io_service) {
			
		}
		
		// handle_write() is responsible for any further actions 
		// for this client connection.
		void handle_write(const boost::system::error_code& /*error*/,size_t /*bytes_transferred*/) {

		}

		tcp::socket socket_;
		std::string m_message;
};

class tcp_server {
	public:
		// Constructor: initialises an acceptor to listen on TCP port port_num.
		tcp_server(boost::asio::io_service& io_service): acceptor_(io_service, tcp::endpoint(tcp::v4(), port_num)) {
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