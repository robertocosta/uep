#include "server.hpp"

#include <unistd.h>

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

const uep_server_parameters DEFAULT_SERVER_PARAMETERS{
  {50,1000},
  {10,1},
  1,
  0.1,
  0.5,
  50,
  true,
  0,
  "12312",
  true,
  uep_encoder<>::MAX_SEQNO
};

std::shared_ptr<control_connection>
control_connection::create(boost::asio::io_service& io_service,
			   control_server &psrv,
			   const uep_server_parameters &sp) {
  auto ccptr = new control_connection(io_service, psrv, sp);
  return std::shared_ptr<control_connection>(ccptr);
}

void control_connection::start() {
  BOOST_LOG_SEV(basic_lg, log::debug) << "Start a new connection";
  wait_for_message();
}

control_connection::control_connection(boost::asio::io_service& io_service,
				       control_server &psrv,
				       const uep_server_parameters &sp) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  parent_srv(psrv),
  state(WAIT_STREAM),
  io(io_service),
  strand(io),
  socket_(io),
  proto_rd(io, socket_),
  proto_wr(io, socket_),
  ds(io),
  srv_params(sp) {
}

boost::asio::ip::tcp::socket &control_connection::socket() {
  return socket_;
}

void control_connection::wait_for_message() {
  using namespace std::placeholders;
  auto handler = strand.wrap(std::bind(&control_connection::handle_new_message,
				       this, _1, _2));
  proto_rd.async_read_one(last_msg, handler);
}

void control_connection::handle_new_message(const boost::system::error_code &ec,
					    std::size_t bytes) {
  if (ec == boost::asio::error::eof) {
    // Client closed the connction
    socket_.cancel();
    //socket_.close();
    parent_srv.forget_connection(*this);
    return;
  }

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

void control_connection::handle_stream_name() {
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
  ds.max_sequence_number(srv_params.max_n_per_block);
}

void control_connection::send_client_params() {
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

void control_connection::handle_client_port() {
  using namespace boost::asio;
  ip::address remote_addr = socket_.remote_endpoint().address();
  ip::udp::endpoint remote_ep{remote_addr, clientPort};
  BOOST_LOG_SEV(basic_lg, log::debug) << "Opening UDP server for client "
				      << remote_ep;
  ds.open(remote_ep);
}

void control_connection::send_server_port() {
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

void control_connection::handle_start() {
  ds.start();
}

control_server::control_server(boost::asio::io_service& io_service,
			       const uep_server_parameters &srv_params) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
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
  BOOST_LOG_SEV(basic_lg, log::info) << "Server is listening on "
				     << acceptor.local_endpoint();
}

void control_server::forget_connection(const control_connection &c) {
  const control_connection *const cptr = &c;
  auto i = std::find_if(active_conns.cbegin(), active_conns.cend(),
			[cptr](const std::shared_ptr<control_connection> &ptr){
			  return ptr.get() == cptr;
			});
  if (i != active_conns.cend()) {
    BOOST_LOG_SEV(basic_lg, log::debug) << "Forget connection from "
					<< (*i)->socket().remote_endpoint();
    active_conns.erase(i);
    if (server_params.oneshot) {
      acceptor.cancel();
    }
  }
}

void control_server::start_accept() {
  using namespace std::placeholders;

  std::shared_ptr<control_connection> new_connection =
    control_connection::create(io, *this, server_params);
  auto &socket = new_connection->socket();
  acceptor.async_accept(socket,
			strand.wrap(std::bind(&control_server::handle_accept,
					      this,
					      new_connection,
					      _1)));
}

void control_server::handle_accept(std::shared_ptr<control_connection> new_connection,
				   const boost::system::error_code& error) {
  if (error == boost::asio::error::operation_aborted) return;

  if (error) {
    std::cerr << "Error on async_accept: " << error.message() << std::endl;
  }
  else if (!server_params.oneshot || active_conns.empty()) {
    BOOST_LOG_SEV(basic_lg, log::debug) << "New connection from "
					<< new_connection->socket().remote_endpoint();
    active_conns.push_back(new_connection);
    new_connection->start();
  }

  // Call start_accept() to initiate the next accept operation.
  start_accept();
}

}}

using namespace std;
using namespace uep;

int main(int argc, char **argv) {
  log::init("server.log");
  log::default_logger basic_lg = log::basic_lg::get();
  log::default_logger perf_lg = log::perf_lg::get();

  net::uep_server_parameters srv_params = net::DEFAULT_SERVER_PARAMETERS;

  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, "p:r:n:l")) != -1) {
    switch (c) {
    case 'p':
      srv_params.tcp_port_num = optarg;
      break;
    case 'r':
      srv_params.sendRate = std::strtod(optarg, nullptr);
      break;
    case 'n':
      srv_params.max_n_per_block = std::strtoull(optarg, nullptr, 10);
      break;
    case 'l':
      srv_params.oneshot = false;
      break;
    default:
      std::cerr << "Usage: " << argv[0]
		<< " [-p <local control port>]"
		<< " [-r <send rate>]"
		<< " [-n <max pkts per block>]"
		<< " [-l]"
		<< std::endl;
      return 2;
    }
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
