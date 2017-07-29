#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "controlMessage.pb.h"


using boost::asio::ip::tcp;
std::string port_num = "12312";
std::string nomePrimoStream = "primo stream";


int main(int argc, char* argv[]) {
	try {
		controlMessage::StreamName firstMessage;
		firstMessage.set_streamname("primo stream");
		std::stringstream st;
		std::ostream* output = &st;
		std::string s;
		if (firstMessage.SerializeToOstream(output)) {
			s = st.str();
			try {
				s = s.substr(2,s.length()-2);
			} catch (const std::out_of_range& e) {
				std::cout << "Il messaggio deve essere di lunghezza maggiore di 1\n";
			}
			//std::cout << s << std::endl;
		}
		if (argc != 2) {
			std::cerr << "Usage: client <host>" << std::endl;
			return 1;
		}

		boost::asio::io_service io_service;

		tcp::resolver resolver(io_service);
		tcp::resolver::query query(argv[1], port_num);
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

		tcp::socket socket(io_service);
		boost::asio::connect(socket, endpoint_iterator);
		
		boost::array<char, 128> buf;
		boost::system::error_code error;
		
		
		for (;;)
		{
			std::size_t written = socket.write_some(boost::asio::buffer(s));
			std::cout << "Sent " << written << " bytes" << std::endl;

			size_t len = socket.read_some(boost::asio::buffer(buf), error);
			//output = buf.data();
			
			if (error == boost::asio::error::eof)
				break; // Connection closed cleanly by peer.
			else if (error)
				throw boost::system::system_error(error); // Some other error.
			
			std::cout << "Messaggio ricevuto:\n**** "<<buf.data()<<" ****\n";
			//std::cout.write(buf.data(), len);
			controlMessage::TXParam secondMessage;
			std::cout << "\nasd\n";
			
			boost::asio::const_buffers_1 fixedBuf(&buf, len);
			const unsigned char* p1 = boost::asio::buffer_cast<const unsigned char*>(fixedBuf);
			// std::streambuf::setg()
			if (secondMessage.ParseFromString(p1) {
				std::cout << "il secondo messaggio dal server è stato letto\n";
				std::cout << secondMessage.k() << std::endl;
			}
			std::cout << "\nasd\n";
		}
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}