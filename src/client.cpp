#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "controlMessage.pb.h"


using boost::asio::ip::tcp;
std::string port_num = "12312";
std::string nomePrimoStream = "primo stream";


int main(int argc, char* argv[]) {
	try {
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
		
		
		for (int ii=0;ii<1;ii++)
		{
			std::size_t written;
			controlMessage::StreamName firstMessage;
			firstMessage.set_streamname(nomePrimoStream);
			std::stringstream st;
			std::ostream* output = &st;
			std::string s;
			if (firstMessage.SerializeToOstream(output)) {
				s = st.str();
				written = socket.write_some(boost::asio::buffer(s));
				std::cout << "Sent " << written << " bytes with stream name: \""<<nomePrimoStream<<"\"\n";
			}
			

			size_t len = socket.read_some(boost::asio::buffer(buf), error);
			s = std::string(buf.data(), len);
			
			if (error == boost::asio::error::eof)
				break; // Connection closed cleanly by peer.
			else if (error)
				throw boost::system::system_error(error); // Some other error.
			
			//std::cout.write(buf.data(), len);
			controlMessage::TXParam secondMessage;
			
			// std::streambuf::setg()
			if (secondMessage.ParseFromString(s)) {
				std::cout << "Message received from server:\n";
				std::cout << "K="<<secondMessage.k()<<"; ";
				std::cout << "c="<<secondMessage.c()<<"; ";
				std::cout << "Delta="<<secondMessage.delta()<<"; ";
				std::cout << "RFM="<<secondMessage.rfm()<<"; ";
				std::cout << "RFL="<<secondMessage.rfl()<<"; ";
				std::cout << "EF="<<secondMessage.ef()<<"; ";
				std::cout << "ACK="<<(secondMessage.ack()?"TRUE":"FALSE")<<"; ";
				std::cout << "fileSize="<<secondMessage.filesize()<<"\n";
				std::cout << "Creation of decoder...\n";
				int udpPort = 1444; // = new decoder(...)
				controlMessage::Connect connectMessage;
				connectMessage.set_port(udpPort);
				if (connectMessage.SerializeToOstream(output)) {
					s = st.str();
					written = socket.write_some(boost::asio::buffer(s));
					std::cout << "Connecting now...\n";
					// waiting for ConnACK
					len = socket.read_some(boost::asio::buffer(buf), error);
					s = std::string(buf.data(), len);
					
					if (error == boost::asio::error::eof)
						break; // Connection closed cleanly by peer.
					else if (error)
						throw boost::system::system_error(error); // Some other error.
					
					//std::cout.write(buf.data(), len);
					controlMessage::ConnACK connACKMessage;
					
					// std::streambuf::setg()
					if (connACKMessage.ParseFromString(s)) {
						std::cout << "Connection ACK message received from server:\n";
						std::cout << "udp port="<<connACKMessage.port()<<"\n";
						// setting destination port for decoder's ACK
						std::cout << "Sending PLAY command\n";
						controlMessage::Play playMessage;
						if (playMessage.SerializeToOstream(output)) {
							s = st.str();
							written = socket.write_some(boost::asio::buffer(s));
							std::cout << "PLAY command sent.\n";
						}
					}
				}
				
			}
			
		}
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
