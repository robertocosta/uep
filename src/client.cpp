#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "controlMessage.pb.h"
#include "data_client_server.hpp"
#include "decoder.hpp"

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
	std::string tcp_port_num = "12312";
	uint32_t udp_port_num = 12345;
	std::string streamName = "primo stream";
	bool ack = true;
	int sendRate = 10240;
	size_t fileSize = 20480;

	/* PARAMETERS CHOICE */ 
	int Ls = 64;
};

// MEMORY SINK
struct memory_sink {
  typedef all_params parameter_set;
  explicit memory_sink(const parameter_set&) {}

  std::vector<packet> received;

  void push(const packet &p) {
    packet p_copy(p);
    using std::move;
    push(move(p));
  }

  void push(packet &&p) {
    using std::move;
    received.push_back(move(p));
  }

  explicit operator bool() const { return true; }
  bool operator!() const { return false; }
};

typedef uep::net::data_client<lt_decoder,memory_sink> dc_type;
all_params ps;
boost::asio::io_service io;
std::unique_ptr<dc_type> dc_p;

int main(int argc, char* argv[]) {
	
	try {
		if (argc != 2) {
			std::cerr << "Usage: client <host>" << std::endl;
			return 1;
		}

		tcp::resolver resolver(io);
		tcp::resolver::query query(argv[1], ps.tcp_port_num);
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

		tcp::socket socket(io);
		boost::asio::connect(socket, endpoint_iterator);
		
		boost::array<char, 128> buf;
		boost::system::error_code error;
		
		
		for (int ii=0;ii<1;ii++)
		{
			std::size_t written;
			controlMessage::StreamName firstMessage;
			firstMessage.set_streamname(ps.streamName);
			std::stringstream st;
			std::ostream* output = &st;
			std::string s;
			if (firstMessage.SerializeToOstream(output)) {
				s = st.str();
				written = socket.write_some(boost::asio::buffer(s));
				std::cout << "Sent " << written << " bytes with stream name: \""<<ps.streamName<<"\"\n";
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

				// CREATION OF DECODER
				ps.K = secondMessage.k();
				ps.c = secondMessage.c();
				ps.delta = secondMessage.delta();
				ps.RFM = secondMessage.rfm();
				ps.RFL = secondMessage.rfl();
				ps.EF = secondMessage.ef();
				char portStr[10];
				sprintf(portStr, "%u", ps.udp_port_num);
				dc_type *dc = new dc_type(io);
				dc_p.reset(dc);
				dc->setup_decoder(ps); 
				dc->setup_sink(ps); 
				dc->enable_ack(ps.ack);
				dc->bind(portStr);
				std::cout << "Bound to " << dc->client_endpoint() << std::endl;
				
				controlMessage::Connect connectMessage;
				connectMessage.set_port(ps.udp_port_num);
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
						//char portStr[10];
						//sprintf(portStr, "%u", connACKMessage.port());
						//dc->bind(portStr);
						std::string remAddr = socket.remote_endpoint().address().to_string();
						std::cout << "Start receiving from " << remAddr << std::endl;
						dc->start_receive(remAddr, connACKMessage.port());
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
		std::cout << "Run" << std::endl;
		io.run();
		std::cout << "Done" << std::endl;
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
