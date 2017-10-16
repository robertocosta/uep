#include <cassert>
#include <codecvt>
#include <ctime>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <ostream>
#include <set>
#include <sstream>
#include <string>

#include <boost/asio.hpp>

#include "controlMessage.pb.h"
#include "data_client_server.hpp"
#include "log.hpp"
#include "lt_param_set.hpp"
#include "nal_reader.hpp"
#include "protobuf_rw.hpp"
#include "uep_encoder.hpp"
#include "utils.hpp"

/*
  1: client to server: streamName
  2: server to client: TXParam
  2.1 decoder parameters
  2.1.1 K size_t (unsigned long)
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

namespace uep {
namespace net {

/** Set of parameters needed by the server to setup a new connection. */
struct uep_server_parameters{
  std::initializer_list<std::size_t> Ks;
  std::initializer_list<std::size_t> RFs;
  std::size_t EF;
  double c;
  double delta;
  std::size_t packet_size;
  bool ack;
  double sendRate;
  std::string tcp_port_num;
};

/** Default values for the server parameters. */
const uep_server_parameters DEFAULT_SERVER_PARAMETERS{
  {8,16},
  {2,1},
  2,
  0.1,
  0.5,
  1000,
  true,
  10240,
  "12312"
};

class control_server; // Forward declaration for control_connection

/** Class to handle the control connection used to setup the
 *  data_server and data_client.
 */
class control_connection {
  friend class control_server; // needs to get a ref to the socket

  using enc_t = uep_encoder<>;
  using src_t = nal_reader;
  using ds_type = data_server<enc_t, src_t>;

  /** Enum used to keep track of the current state of the server. */
  enum connection_state {
    WAIT_STREAM, // Wait for the stream name (initial state)
    SEND_PARAMS, // Send the client parameters
    WAIT_CLIENT_PORT, // Wait for the client data port
    SEND_SERVER_PORT, // Send the server data port
    WAIT_START, // Wait for the Start from the client
    RUNNING // Data server is active
  };

public:
  /** Create a new control_connection and return a shared pointer to it. */
  static std::shared_ptr<control_connection>
  create(boost::asio::io_service& io_service,
	 const uep_server_parameters &sp) {
    auto ccptr = new control_connection(io_service, sp);
    return std::shared_ptr<control_connection>(ccptr);
  }

  /** Wait for the stream name from the client. */
  void start() {
    BOOST_LOG_SEV(basic_lg, log::debug) << "Start a new connection";
    wait_for_message();
  }

private:
  log::default_logger basic_lg, perf_lg;

  connection_state state;

  boost::asio::io_service& io;
  boost::asio::io_service::strand strand;
  boost::asio::ip::tcp::socket socket_;
  protobuf_reader proto_rd;
  protobuf_writer proto_wr;
  ds_type ds;
  uep_server_parameters srv_params;

  uep::protobuf::ControlMessage last_msg; /**< Last received message. */
  uep::protobuf::ControlMessage out_msg; /**< Hold the message to be sent. */
  std::string streamName;
  unsigned short clientPort;

  control_connection(boost::asio::io_service& io_service,
		     const uep_server_parameters &sp) :
    basic_lg(boost::log::keywords::channel = log::basic),
    perf_lg(boost::log::keywords::channel = log::performance),
    state(WAIT_STREAM),
    io(io_service),
    strand(io),
    socket_(io),
    proto_rd(io, socket_),
    proto_wr(io, socket_),
    ds(io),
    srv_params(sp) {
  }

  boost::asio::ip::tcp::socket &socket() {
    return socket_;
  }

  /** Schedule a TCP read to receive a message. */
  void wait_for_message() {
    using namespace std::placeholders;
    auto handler = strand.wrap(std::bind(&control_connection::handle_new_message,
					 this, _1, _2));
    proto_rd.async_read_one(last_msg, handler);
  }

  /** Handle a new control message. */
  void handle_new_message(const boost::system::error_code &ec, std::size_t bytes) {
    if (ec) throw boost::system::system_error(ec);

    switch (state) {
    case WAIT_STREAM:
      streamName = last_msg.stream_name();
      if (streamName.empty()) throw std::runtime_error("Empty stream name");
      handle_stream_name();
      state = SEND_PARAMS;
      send_client_params();
      break;
    case WAIT_CLIENT_PORT:
      clientPort = static_cast<unsigned short>(last_msg.client_port());
      if (clientPort == 0) throw std::runtime_error("Client port == 0");
      handle_client_port();
      state = SEND_SERVER_PORT;
      send_server_port();
      break;
    case WAIT_START: {
      uep::protobuf::StartStop ss = last_msg.start_stop();
      if (ss == uep::protobuf::START) {
	handle_start();
	state = RUNNING;
      }
      break;
    }
    case RUNNING:
      // handle stop msg
      break;
    default:
      throw std::runtime_error("Should never get a message in this state");
    }
    wait_for_message();
  }

  /** Use the streamName to setup the data server. */
  void handle_stream_name() {
    std::cout << "Stream name received from client: \"" << streamName << "\"\n";

    /* CREATION OF DATA SERVER */
    //BOOST_LOG_SEV(basic_lg, debug) << "Creation of encoder...\n";
    std::cout << "Creation of encoder...\n";
    // setup the encoder inside the data_server
    ds.setup_encoder(srv_params.Ks.begin(), srv_params.Ks.end(),
		     srv_params.RFs.begin(), srv_params.RFs.end(),
		     srv_params.EF,
		     srv_params.c,
		     srv_params.delta);
    // setup the source  inside the data_server
    ds.setup_source(streamName, srv_params.packet_size);
    ds.source().use_end_of_stream(true);

    ds.target_send_rate(srv_params.sendRate);
    ds.enable_ack(srv_params.ack);
  }

  /** Send the parameters to the client. */
  void send_client_params() {
    using namespace uep::protobuf;

    // SENDING ENCODER PARAMETERS
    const auto &Ks = srv_params.Ks;
    const auto &RFs = srv_params.RFs;
    ClientParameters &cp = *(out_msg.mutable_client_parameters());

    auto j = Ks.begin();
    auto k = RFs.begin();
    while (j != Ks.end()) {
      cp.add_ks(*j++);
      cp.add_rfs(*k++);
    }

    cp.set_c(srv_params.c);
    cp.set_delta(srv_params.delta);

    cp.set_ef(srv_params.EF);
    cp.set_ack(srv_params.ack);

    const buffer_type &hdr = ds.source().header();
    cp.set_header(hdr.data(), hdr.size());
    cp.set_headersize(hdr.size());
    cp.set_filesize(ds.source().totLength());

    BOOST_LOG_SEV(basic_lg, log::trace) << "Sending the client parameters";
    auto h = strand.wrap([this](const boost::system::error_code &ec,
				std::size_t bytes_tx) {
			   if (ec) throw boost::system::system_error(ec);
			   if (state != SEND_PARAMS)
			     throw std::logic_error("Unexpected state");
			   state = WAIT_CLIENT_PORT;
			 });
    proto_wr.async_write_one(out_msg, h);
  }

  /** Open the server socket to send to the received client port. */
  void handle_client_port() {
    using namespace boost::asio;
    ip::address remote_addr = socket_.remote_endpoint().address();
    ip::udp::endpoint remote_ep{remote_addr, clientPort};
    BOOST_LOG_SEV(basic_lg, log::debug) << "Opening UDP server for client "
					<< remote_ep;
    ds.open(remote_ep);
  }

  /** Send the UDP server port to the client. */
  void send_server_port() {
    unsigned short sp = ds.server_endpoint().port();
    out_msg.set_server_port(sp);
    BOOST_LOG_SEV(basic_lg, log::trace) << "Sending the server port ("
					<< sp << ")";
    auto h = strand.wrap([this](const boost::system::error_code &ec,
				std::size_t bytes_tx) {
			   if (ec) throw boost::system::system_error(ec);
			   if (state != SEND_SERVER_PORT)
			     throw std::logic_error("Unexpected state");
			   state = WAIT_START;
			 });
    proto_wr.async_write_one(out_msg, h);
  }

  /** Start the UDP server. */
  void handle_start() {
    ds.start();
  }
};

/** Class that accepts incoming connections on the TCP control port
 *	and setups a new control_connection for each one.
 *
 *  This class holds a shared pointer to every new connection. They
 *  must be deleted using forget_connection.
 */
class control_server {
public:
  /** Builds using the given io_service. The server parameters will be
   *  used for each new connection.
   */
  control_server(boost::asio::io_service& io_service,
		 const uep_server_parameters &srv_params) :
    io(io_service),
    strand(io),
    acceptor(io_service),
    server_params(srv_params) {
    using boost::asio::ip::tcp;

    tcp::resolver resolver(io);
    tcp::resolver::query listen_q(server_params.tcp_port_num);
    tcp::endpoint ep = *resolver.resolve(listen_q); // Take the first one

    acceptor.open(ep.protocol());
    acceptor.bind(ep);
    acceptor.listen();

    // start_accept() creates a socket and
    // initiates an asynchronous accept operation
    // to wait for a new connection.
    start_accept();
  }

  /** Delete the shared pointer to a connection held by the server. If
   *	there are no other shared pointers to the connection it will be
   *	destroyed.
   */
  void forget_connection(const control_connection &c) {
    const control_connection *const cptr = &c;
    auto i = std::find_if(active_conns.cbegin(), active_conns.cend(),
			  [cptr](const std::shared_ptr<control_connection> &ptr){
			    return ptr.get() == cptr;
			  });
    if (i != active_conns.cend()) {
      active_conns.erase(i);
    }
  }

private:
  boost::asio::io_service &io;
  boost::asio::io_service::strand strand;
  boost::asio::ip::tcp::acceptor acceptor;
  uep_server_parameters server_params;
  std::list<std::shared_ptr<control_connection>> active_conns;

  /** Wait for a new connection attempt. */
  void start_accept() {
    using namespace std::placeholders;

    std::shared_ptr<control_connection> new_connection =
      control_connection::create(io, server_params);
    auto &socket = new_connection->socket();
    acceptor.async_accept(socket,
			  strand.wrap(std::bind(&control_server::handle_accept,
						this,
						new_connection,
						_1)));
  }

  /** Handle a new connection attempt. */
  void handle_accept(std::shared_ptr<control_connection> new_connection,
		     const boost::system::error_code& error) {
    if (error) {
      std::cerr << "Error on async_accept: " << error.message() << std::endl;
    }
    else {
      active_conns.push_back(new_connection);
      new_connection->start();
    }

    // Call start_accept() to initiate the next accept operation.
    start_accept();
  }
};

}}

using namespace std;
using namespace uep;

int main(int argc, char **argv) {
  log::init("server.log");
  log::default_logger basic_lg = log::basic_lg::get();
  log::default_logger perf_lg = log::perf_lg::get();

  net::uep_server_parameters srv_params = net::DEFAULT_SERVER_PARAMETERS;

  if (argc > 1) {
    srv_params.tcp_port_num = argv[1];
  }
  if (argc > 2) {
    std::cerr << "Too many args" << std::endl;
    return 1;
  }

  // We need to create a server object to accept incoming client connections.
  boost::asio::io_service io_service;

  // The io_service object provides I/O services, such as sockets,
  // that the server object will use.
  net::control_server server(io_service, srv_params);

  // Run the io_service object to perform asynchronous operations.
  BOOST_LOG_SEV(basic_lg, log::info) << "Run";
  io_service.run();
  BOOST_LOG_SEV(basic_lg, log::info) << "Stopped";

  return 0;
}
