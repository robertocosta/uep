#ifndef UEP_SERVER_HPP
#define UEP_SERVER_HPP

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
  bool oneshot;
};

/** Default values for the server parameters. */
extern const uep_server_parameters DEFAULT_SERVER_PARAMETERS;

class control_server; // Forward declaration for control_connection

/** Class to handle the control connection used to setup the
 *  data_server and data_client.
 */
class control_connection {
  friend control_server; // needs to get a ref to the socket

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
	 control_server &psrv,
	 const uep_server_parameters &sp);

  /** Wait for the stream name from the client. */
  void start();

private:
  log::default_logger basic_lg, perf_lg;

  control_server &parent_srv;

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
		     control_server &psrv,
		     const uep_server_parameters &sp);

  boost::asio::ip::tcp::socket &socket();

  /** Schedule a TCP read to receive a message. */
  void wait_for_message();
  /** Handle a new control message. */
  void handle_new_message(const boost::system::error_code &ec, std::size_t bytes);
  /** Use the streamName to setup the data server. */
  void handle_stream_name();
  /** Send the parameters to the client. */
  void send_client_params();
  /** Open the server socket to send to the received client port. */
  void handle_client_port();
  /** Send the UDP server port to the client. */
  void send_server_port();
  /** Start the UDP server. */
  void handle_start();
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
		 const uep_server_parameters &srv_params);

  /** Delete the shared pointer to a connection held by the server. If
   *	there are no other shared pointers to the connection it will be
   *	destroyed.
   */
  void forget_connection(const control_connection &c);

private:
  log::default_logger basic_lg, perf_lg;
  
  boost::asio::io_service &io;
  boost::asio::io_service::strand strand;
  boost::asio::ip::tcp::acceptor acceptor;
  uep_server_parameters server_params;
  std::list<std::shared_ptr<control_connection>> active_conns;

  /** Wait for a new connection attempt. */
  void start_accept();
  /** Handle a new connection attempt. */
  void handle_accept(std::shared_ptr<control_connection> new_connection,
		     const boost::system::error_code& error);
};

}}

#endif
