#include <fstream>
#include <iostream>
#include <vector>

#include <boost/asio.hpp>

#include "controlMessage.pb.h"
#include "data_client_server.hpp"
#include "log.hpp"
#include "nal_reader.hpp"
#include "nal_writer.hpp"
#include "protobuf_rw.hpp"
#include "uep_decoder.hpp"

namespace uep { namespace net {

/** Set of parameters needed by the control_client to setup a
 *  connection.
 */
struct uep_client_parameters {
  std::string stream_name;
  std::string local_data_port;
  std::string remote_control_addr;
  std::string remote_control_port;
  double drop_prob;
};

/** Default values for the client parameters. */
const uep_client_parameters DEFAULT_CLIENT_PARAMETERS{
  "",
  "12345",
  "127.0.0.1",
  "12312",
  0
};

class control_client {
  using dec_t = uep_decoder;
  using sink_t = nal_writer;
  using dc_type = data_client<dec_t, sink_t>;

  enum client_state {
    SEND_STREAM, // Send the requested stream name (initial state)
    WAIT_PARAMS, // Wait for the client parameters
    SEND_CLIENT_PORT, // Send the local data port
    WAIT_SERVER_PORT, // Wait for the remote data port
    SEND_START, // Send the start command
    RUNNING // Data client is active
  };

public:
  /** Construct the client. This does not open the connection to the server. */
  explicit control_client(boost::asio::io_service &io_svc,
			  const uep_client_parameters &cl_par) :
    basic_lg(boost::log::keywords::channel = log::basic),
    perf_lg(boost::log::keywords::channel = log::performance),
    state(SEND_STREAM),
    strand(io_svc),
    tcp_socket(io_svc),
    proto_rd(io_svc, tcp_socket),
    proto_wr(io_svc, tcp_socket),
    dc(io_svc),
    client_params(cl_par) {
  }

  /** Open the connction to the server and start the receiving process. */
  void start() {
    using namespace boost::asio::ip;
    using namespace std::placeholders;

    tcp::resolver resolver(tcp_socket.get_io_service());
    tcp::resolver::query query(client_params.remote_control_addr,
			       client_params.remote_control_port);
    tcp::resolver::iterator ep_iter = resolver.resolve(query);
    auto h = strand.wrap(std::bind(&control_client::handle_connection,
				   this,
				   _1, _2));
    boost::asio::async_connect(tcp_socket, ep_iter, h);
  }

private:
  log::default_logger basic_lg, perf_lg;

  client_state state;

  boost::asio::io_service::strand strand;
  boost::asio::ip::tcp::socket tcp_socket;
  protobuf_reader proto_rd;
  protobuf_writer proto_wr;
  dc_type dc;
  uep_client_parameters client_params;

  uep::protobuf::ControlMessage last_msg;
  uep::protobuf::ControlMessage out_msg;
  buffer_type out_header;
  boost::asio::ip::udp::endpoint remote_data_ep;

  void handle_connection(const boost::system::error_code &ec,
			 boost::asio::ip::tcp::resolver::iterator i) {
    if (ec) throw boost::system::system_error(ec);
    if (i == decltype(i){}) throw std::runtime_error("Could not connect");

    if (client_params.stream_name.empty())
      throw std::runtime_error("Stream name is empty");
    out_msg.set_stream_name(client_params.stream_name);
    BOOST_LOG_SEV(basic_lg, log::trace) << "Sending the stream name";
    auto h = strand.wrap([this](const boost::system::error_code &ec,
				std::size_t bytes_tx) {
			   if (ec) throw boost::system::system_error(ec);
			   if (state != SEND_STREAM)
			     throw std::logic_error("Unexpected state");
			   state = WAIT_PARAMS;
			 });
    proto_wr.async_write_one(out_msg, h);
    wait_for_message();
  }

  void wait_for_message() {
    using namespace std::placeholders;
    auto handler = strand.wrap(std::bind(&control_client::handle_new_message,
					 this, _1, _2));
    proto_rd.async_read_one(last_msg, handler);
  }


  /** Handle a new control message. */
  void handle_new_message(const boost::system::error_code &ec, std::size_t bytes) {
    if (ec == boost::asio::error::operation_aborted) {
      return; // Client was stopped
    }

    if (ec) throw boost::system::system_error(ec);

    switch (state) {
    case WAIT_PARAMS:
      handle_params();
      state = SEND_CLIENT_PORT;
      send_client_port();
      break;
    case WAIT_SERVER_PORT:
      handle_server_port();
      state = SEND_START;
      send_start();
      break;
    case RUNNING:
      // handle stop msg
      break;
    default:
      throw std::runtime_error("Should never get a message in this state");
    }
    wait_for_message();
  }

  void handle_params() {
    const protobuf::ClientParameters &cp = last_msg.client_parameters();
    assert(cp.ks_size() == cp.rfs_size());
    assert(cp.ks_size() != 0);
    assert(cp.ef() != 0);
    assert(cp.c() != 0);
    assert(cp.delta() != 0);
    std::vector<std::size_t> Ks, RFs;
    for (int i = 0; i < cp.ks_size(); ++i) {
      Ks.push_back(cp.ks(i));
      RFs.push_back(cp.rfs(i));
    }
    out_header.assign(cp.header().begin(), cp.header().end());
    dc.setup_decoder(Ks.begin(), Ks.end(),
		     RFs.begin(), RFs.end(),
		     cp.ef(),
		     cp.c(),
		     cp.delta());
    dc.setup_sink(out_header, client_params.stream_name);
    dc.enable_ack(cp.ack());
    //dc.expected_count(0);
    //dc.timeout(1);
    //dc.add_stop_handler();
    dc.drop_probability(client_params.drop_prob);
    dc.bind(client_params.local_data_port);
  }

  void send_client_port() {
    unsigned short cp = dc.client_endpoint().port();
    out_msg.set_client_port(cp);
    BOOST_LOG_SEV(basic_lg, log::trace) << "Sending the client port "
					<< cp;
    auto h = strand.wrap([this](const boost::system::error_code &ec,
				std::size_t bytes_tx) {
			   if (ec) throw boost::system::system_error(ec);
			   if (state != SEND_CLIENT_PORT)
			     throw std::logic_error("Unexpected state");
			   state = WAIT_SERVER_PORT;
			 });
    proto_wr.async_write_one(out_msg, h);
  }

  void handle_server_port() {
    using namespace std::placeholders;

    unsigned short sp = static_cast<unsigned short>(last_msg.server_port());
    assert(sp != 0);
    remote_data_ep.port(sp);
    remote_data_ep.address(tcp_socket.remote_endpoint().address());
    auto dc_stop_h = strand.wrap(std::bind(&control_client::handle_dc_stop,
					   this, _1));
    dc.add_stop_handler(dc_stop_h);
    dc.start_receive(remote_data_ep);
  }

  void send_start() {
    out_msg.set_start_stop(protobuf::START);
    BOOST_LOG_SEV(basic_lg, log::trace) << "Sending the start command";
    auto h = strand.wrap([this](const boost::system::error_code &ec,
				std::size_t bytes_tx) {
			   if (ec) throw boost::system::system_error(ec);
			   if (state != SEND_START)
			     throw std::logic_error("Unexpected state");
			   state = RUNNING;
			 });
    proto_wr.async_write_one(out_msg, h);
  }

  void handle_dc_stop(const boost::system::error_code &ec) {
    if (ec) throw boost::system::system_error(ec);

    BOOST_LOG_SEV(basic_lg, log::debug) << "TCP client is stopping";
    tcp_socket.cancel();
    tcp_socket.close();
  }
};

}}


using namespace boost::asio;
using namespace std;
using namespace uep::log;
using namespace uep::net;
using namespace uep;

int main(int argc, char* argv[]) {
  log::init("client.log");
  default_logger basic_lg(boost::log::keywords::channel = basic);
  default_logger perf_lg(boost::log::keywords::channel = performance);

  boost::asio::io_service io;
  uep_client_parameters client_params = DEFAULT_CLIENT_PARAMETERS;

  if (argc < 3) {
    std::cerr << "Usage: client <stream> <server>"
	      << " [<server_port>] [<listen_port>]"
	      << " [<drop probability>]"
	      << std::endl;
    return 1;
  }

  client_params.stream_name = argv[1];
  client_params.remote_control_addr = argv[2];
  if (argc > 3) {
    client_params.remote_control_port = argv[3];
  }
  if (argc > 4) {
    client_params.local_data_port = argv[4];
  }
  if (argc > 5) {
    client_params.drop_prob = std::stod(argv[5]);
  }
  if (argc > 6) {
    std::cerr << "Too many args" << std::endl;
    return 1;
  }

  BOOST_LOG_SEV(basic_lg, log::info) << "Requesting stream \""
				     << client_params.stream_name
				     << "\" from "
				     << client_params.remote_control_addr
				     << ":"
				     << client_params.remote_control_port
				     << " Local port: "
				     << client_params.local_data_port;

  control_client cc{io, client_params};

  cc.start();

  std::cout << "Run" << std::endl;
  io.run();
  std::cout << "Done" << std::endl;

  return 0;
}
