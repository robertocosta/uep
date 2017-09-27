#ifndef UEP_NET_DATA_CLIENT_SERVER_HPP
#define UEP_NET_DATA_CLIENT_SERVER_HPP

#include <chrono>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include "counter.hpp"
#include "log.hpp"
#include "packets_rw.hpp"

namespace uep { namespace net {

/** Maximum payload size that can be carried by a UDP packet. */
static constexpr std::size_t UDP_MAX_PAYLOAD = 0x10000;

/** Receive coded packets via a UDP socket.
 *
 *  This class listens on a socket for fountain_packets, passes them
 *  to an object of class Decoder, then passes the decoded packets to
 *  an object of class Sink.
 */
template <class Decoder, class Sink>
class data_client {
public:
  typedef Decoder decoder_type;
  typedef Sink sink_type;
  typedef typename Decoder::parameter_set decoder_parameter_set;
  typedef typename Sink::parameter_set sink_parameter_set;

  /** Construct a data_client tied to an io_service. This does not
   *  bind the socket yet.
   */
  explicit data_client(boost::asio::io_service &io);

  /** Replace the decoder object with one built from the given
   *  parameters.
   */
  void setup_decoder(const decoder_parameter_set &ps);
  /** Replace the sink object with one built from the given
   *  parameters.
   */
  void setup_sink(const sink_parameter_set &ps);

  /** Bind the socket to the given port. */
  void bind(const std::string &service);
  /** Bind the socket to the given port. */
  void bind(unsigned short port);

  /** Listen asynchronously for packets coming from a given remote
   *  (source) endpoint.
   */
  void start_receive(const boost::asio::ip::udp::endpoint &src_ep);
  /** Listen asynchronously for packets coming from a given remote
   *  (source) IP address and port.
   */
  void start_receive(const std::string &src_addr, unsigned short src_port);
  /** Listen asynchronously for packets coming from a given remote
   *  (source) IP address and port.
   */
  void start_receive(const std::string &src_addr, const std::string &src_port);

  /** Schedule the stopping of the data_client. */
  void stop();

  /** Get the UDP endpoint that the client socket is currently bound
   *  to.
   */
  boost::asio::ip::udp::endpoint client_endpoint() const;

  /** Enable or disable the sending of ACKs. */
  void enable_ack(bool b);
  /** Return true when the sending of ACKs is enabled. */
  bool is_ack_enabled() const;
  /** Set the number of packets to be received before stopping. */
  void expected_count(std::size_t ec);
  /** Get the number of packets to be received before stopping. */
  std::size_t expected_count() const;
  /** True if the client is not listening for packets. */
  bool is_stopped() const;
  /** Set the timeout interval in seconds. A value of 0 disables the
   *  timeout.
   */
  void timeout(double t);
  /** Get the current timeout value in seconds. */
  double timeout() const;

  /** Add an handler that will be called when this client stops. */
  template <class H>
  void add_stop_handler(const H &h);
  /** Abort all handlers set up with add_stop_handler. Returns the
   *  number of aborted handlers.
   */
  std::size_t cancel_stop_handlers();

  /** Return a const reference to the sink object. */
  const Sink &sink() const;
  /** Return a const reference to the decoder object. */
  const Decoder &decoder() const;

private:
  log::default_logger basic_lg, perf_lg;

  std::unique_ptr<Decoder> decoder_; /**< Use a pointer to allow
				      *   default-construction.
				      */
  std::unique_ptr<Sink> sink_; /**< Use a pointer to allow
				*   default-construction.
				*/

  boost::asio::io_service &io_service_;
  boost::asio::io_service::strand strand_; /**< Strand used to
					    *   synchronize the
					    *   asynchronous methods
					    *   across threads.
					    */
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint listen_endpoint_; /**< The local
						    *   endpoint where
						    *   the
						    *   data_client is
						    *   listening.
						    */
  boost::asio::ip::udp::endpoint server_endpoint_; /**< The remote
						    *   endpoint from
						    *   where the
						    *   data_client
						    *   expects to
						    *   receive
						    *   packets.
						    */
  buffer_type recv_buffer; /**< Buffer that holds the last received
			    *	UDP payload.
			    */
  buffer_type ack_buffer; /**< Buffer to hold the raw ack during
			   *   the async transmission.
			   */

  std::atomic_bool ack_enabled; /**< Set when the data_client should
				 *   send back ACKs.
				 */
  std::atomic_size_t exp_count; /**< The number of packets that should
				 *   be received before stopping the
				 *   client. A value of 0 means that
				 *   the client will never stop to
				 *   listen.
				 */
  std::atomic_bool is_stopped_;
  boost::asio::steady_timer timeout_timer; /**< Timer used to stop the
					   *   reception after some
					   *   time if packets are not
					   *   received.
					   */

  std::list<
    std::function<
      void(const boost::system::error_code&)
      >
    > stop_handlers; /**< Holds the handlers to call after a stop. */

  /** Duration of an interval without packets after which the client
   *  stops. If this is 0 the timeout is disabled.
   */
  std::atomic<std::chrono::steady_clock::duration> timeout_;

  /** Setup the socket to asynchronously receive a packet. */
  void async_receive_pkt();
  /** Setup the timer to expire after the timeout value. */
  void reset_timer();
  /** Schedule the transmission of an ACK to the server. */
  void schedule_ack(std::size_t blockno);

  /** Called when a new packet is received. */
  void handle_received(const boost::system::error_code& ec, std::size_t size);
  /** Called when the ACK has been sent. */
  void handle_sent_ack(const boost::system::error_code& ec, std::size_t size);
  /** Called when the timeout timer expires or is cancelled. */
  void handle_timeout(const boost::system::error_code& ec);
  /** Called after stop(). */
  void handle_stop();
};

/** Send the packets output by an encoder through a UDP socket.
 *
 *  This class extracts packets from a Source object, testing if they
 *  are available by converting the source to bool. The Source class
 *  must also implement a next_packet() method that returns a new
 *  packet.
 *
 *  The Encoder class receives the packets produced by the source via
 *  push(packet&&) or push(const packet&). It generates coded packets
 *  using next_coded() and skips to the next block when next_block()
 *  is called.
 *
 *  The send rate can be dynamically limited by specifying it in
 *  bit/s. If it is too high the server sends at maximum rate.
 */
template <class Encoder, class Source>
class data_server {
public:
  typedef Encoder encoder_type;
  typedef Source source_type;
  typedef typename Encoder::parameter_set encoder_parameter_set;
  typedef typename Source::parameter_set source_parameter_set;

  /** Construct a data_server tied to an io_service. This does not
   *  bind the socket yet.
   */
  explicit data_server(boost::asio::io_service &io) :
    basic_lg(boost::log::keywords::channel = log::basic),
    perf_lg(boost::log::keywords::channel = log::performance),
    io_service_(io),
    strand_(io_service_),
    socket_(io_service_),
    target_send_rate_(std::numeric_limits<double>::infinity()),
    is_stopped_(true),
    ack_enabled(true),
    max_per_block(Encoder::MAX_SEQNO),
    last_ack(ack_header_size),
    pkt_timer(io_service_) {
  }

  /** Replace the encoder with a new one built using the given
   *  parameter set.
   */
  void setup_encoder(const encoder_parameter_set &ps) {
    BOOST_LOG_SEV(basic_lg, log::trace) << "Setting up the encoder";
    encoder_.reset(new Encoder(ps));
  }

  /** Replace the source with a new one built using the given
   *  parameter set.
   */
  void setup_source(const source_parameter_set &ps) {
    BOOST_LOG_SEV(basic_lg, log::trace) << "Setting up the source";
    source_.reset(new Source(ps));
  }

  /** Resolve the destination (client) endpoint and bind the socket. */
  void open(const std::string &dest_host, const std::string &dest_service) {
    using namespace boost::asio;
    using ip::udp;

    BOOST_LOG_SEV(basic_lg, log::trace) << "Opening the server UDP port";
    udp::resolver resolver(io_service_);
    udp::resolver::query query(udp::v4(), dest_host, dest_service);
    auto i = resolver.resolve(query);
    if (i == udp::resolver::iterator()) {
      throw std::runtime_error("No endpoint found");
    }
    client_endpoint_ = *i;
    BOOST_LOG_SEV(basic_lg, log::info) << "Server will send data to: "
				       << client_endpoint_;
    // Send from any address and pick a free port
    udp::endpoint local(ip::address::from_string("0.0.0.0"), 0);
    socket_.open(udp::v4());
    socket_.bind(local);
    BOOST_LOG_SEV(basic_lg, log::info) << "Server UDP port bound to: "
				       << socket_.local_endpoint();
  }

  /** Schedule the start of a transmission toward the client endpoint. */
  void start() {
    BOOST_LOG_SEV(basic_lg, log::info) << "UDP server is starting";
    strand_.dispatch(std::bind(&data_server::handle_started, this));
  }

  /** Schedule the stop of the current transmission. */
  void stop() {
    BOOST_LOG_SEV(basic_lg, log::info) << "UDP server is stopping";
    strand_.dispatch(std::bind(&data_server::handle_stopped, this));
  }

  /** Schedule the passage to the next block of packets. */
  void next_block() {
    strand_.dispatch(std::bind(&Encoder::next_block, encoder_.get()));
  }

  /** Set the target send rate in bit/s. */
  void target_send_rate(double sr) {
    target_send_rate_ = sr;
  }

  /** Get the current target send rate (bit/s). */
  double target_send_rate() const {
    return target_send_rate_;
  }

  /** Set the maximum sequence number before skipping to the next
   *  block.
   */
  void max_sequence_number(std::size_t m) {
    if (m > Encoder::MAX_SEQNO)
      throw std::overflow_error("Cannot exceed the Encoder's limit");
    if (m < 1)
      throw std::underflow_error("Must send at least one packet");
    max_per_block = m;
  }

  std::size_t max_sequence_number() const {
    return max_per_block;
  }

  /** Get the UDP endpoint that the server socket is currently bound
   *  to.
   */
  boost::asio::ip::udp::endpoint server_endpoint() const {
    return socket_.local_endpoint();
  }

  /** Enable or disable the sending of ACKs. */
  void enable_ack(bool b) {
    ack_enabled = b;
  }

  /** Return true when the sending of ACKs is enabled. */
  bool is_ack_enabled() const {
    return ack_enabled;
  }

  /** True when the data_server is not sending data and not listening
   *  for ACKs.
   */
  bool is_stopped() const {
    return is_stopped_;
  }

  /** Add an handler that will be called when this client stops. */
  template <class H>
  void add_stop_handler(const H &h);
  /** Abort all handlers set up with add_stop_handler. Returns the
   *  number of aborted handlers.
   */
  std::size_t cancel_stop_handlers();

  /** Return a const reference to the source object. */
  const Source &source() const {
    return *source_;
  }

  /** Return a const referece to the encoder object. */
  const Encoder &encoder() const {
    return *encoder_;
  }

private:
  log::default_logger basic_lg, perf_lg;

  std::unique_ptr<Encoder> encoder_; /**< The encoder. Use a pointer
				      *	  to allow for
				      *	  default-construction.
				      */
  std::unique_ptr<Source> source_; /**< The source. Use a pointer to
				    *	allow for
				    *	default-construction.
				    */

  boost::asio::io_service &io_service_;
  boost::asio::io_service::strand strand_; /**< Strand used to
					    *   synchronize the
					    *   asynchronous methods
					    *   across threads.
					    */
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint client_endpoint_;

  std::atomic<double> target_send_rate_; /**< Target send rate in
					  *   bit/s.
					  */

  std::atomic_bool is_stopped_; /**< Set when the data_server should
				 *	 not send packets.
				 */
  std::atomic_bool ack_enabled; /**< Set when the data_server should
				 *   listen for incoming ACKs.
				 */
  std::atomic_size_t max_per_block; /**< Maximum number of packets
				     *   that can be generated for
				     *   each block before skipping to
				     *   the next.
				     */
  buffer_type last_pkt; /**< Last _raw_ coded packet generated
			       *   by the encoder.
			       */
  buffer_type last_ack; /**< Last _raw_ ack packet received. */
  std::chrono::steady_clock::time_point last_sent_time;
  boost::asio::steady_timer pkt_timer; /**< Timer used to schedule the
					*   packet transmissions.
					*/

  std::list<
    std::function<
      void(const boost::system::error_code&)
      >
    > stop_handlers; /**< Holds the handlers to call after a stop. */

  /** Encode the next packet and schedule its transmission according
   *  to the target send rate.
   */
  void schedule_next_pkt() {
    BOOST_LOG_SEV(basic_lg, log::debug) << "Called schedule_next_pkt";
    using std::move;
    //using std::chrono::seconds;
    using std::chrono::microseconds;
    using namespace std::placeholders;

    if (is_stopped_) return;

    // Load the encoder until it has 2 full blocks: allow next_block
    // to still have a block
    while (*source_ && (encoder_->size() < 2*encoder_->K())) {
      encoder_->push(source_->next_packet());
    }

    // Empty encoder and no more data: stop
    if (!*encoder_) {
      BOOST_LOG_SEV(basic_lg, log::info) <<
	"Data server out of data to send";
      stop();
      return;
    }

    // Check if the max number has been reached
    if (max_per_block <= encoder_->coded_count()) {
      encoder_->next_block();
      if (!*encoder_) { // The source did not have 2 blocks
	BOOST_LOG_SEV(basic_lg, log::info) <<
	  "Data server out of data to send after reaching max_per_block";
	stop();
	return;
      }
    }

    fountain_packet p = encoder_->next_coded();
    bool is_first = last_pkt.empty();
    last_pkt = build_raw_packet(move(p));

    if (is_first) { // First packet: no need to wait
      pkt_timer.expires_from_now(microseconds(0));
    }
    else { // Set an interarrival time to have the target send rate
      double sr = target_send_rate_;
      decltype(last_sent_time) next_send = last_sent_time +
	microseconds(static_cast<long>(last_pkt.size() * (1e6 / sr)));
      pkt_timer.expires_at(next_send);
    }

    // Schedule the timer
    pkt_timer.async_wait(strand_.wrap(std::bind(&data_server::handle_send_timer,
						this, std::placeholders::_1)));
  }

  /** Listen asynchronously for incoming ACK packets. */
  void listen_for_acks() {
    using namespace std::placeholders;

    if (is_stopped_) return; // Not yet started or has finished
    if (!ack_enabled) return;

    // Listen
    socket_.async_receive_from(boost::asio::buffer(last_ack), client_endpoint_,
			       strand_.wrap(std::bind(&data_server::handle_ack,
						      this,
						      std::placeholders::_1, std::placeholders::_2)));
  }

  /** Called when a packet is received. */
  void handle_ack(const boost::system::error_code &ec, std::size_t recv_size) {
    if (ec == boost::asio::error::operation_aborted) return; // cancelled
    if (ec) throw boost::system::system_error(ec);

    if (recv_size != ack_header_size)
      throw std::runtime_error("The packet has a wrong size");

    std::size_t ack_blockno_;
    try {
      // This is the next _wanted_ block
      ack_blockno_ = parse_raw_ack_packet(last_ack);
    }
    catch (std::runtime_error e) {
      // should ignore wrong packets?
      throw e;
    }

    // Fill the encoder buffer with enough packets
    circular_counter<std::size_t>
      current_blockno(Encoder::MAX_BLOCKNO),
      ack_blockno(Encoder::MAX_BLOCKNO);
    current_blockno.set(encoder_->blockno());
    ack_blockno.set(ack_blockno_);
    std::size_t diff = current_blockno.forward_distance(ack_blockno);
    if (diff > Encoder::BLOCK_WINDOW || diff == 0) {
      // Old ACK
      // Keep listening
      listen_for_acks();
      return;
    }
    std::size_t required_pkts = diff * encoder_->K();
    while (*source_ && encoder_->size() < required_pkts)
      encoder_->push(source_->next_packet());

    // Skip blocks
    encoder_->next_block(ack_blockno_);

    // Reschedule the next coded packet: must be rebuilt
    pkt_timer.cancel();
    schedule_next_pkt();
    // Keep listening
    listen_for_acks();
  }

  /** Called when the packet timer has expired or was cancelled. */
  void handle_send_timer(const boost::system::error_code &ec) {
    using namespace std::placeholders;

    if (ec == boost::asio::error::operation_aborted) return; // cancelled
    if (ec) throw boost::system::system_error(ec);

    // Start transmission
    last_sent_time = std::chrono::steady_clock::now();
    socket_.async_send_to(boost::asio::buffer(last_pkt),
			  client_endpoint_,
			  strand_.wrap(std::bind(&data_server::handle_sent,
						 this,
						 std::placeholders::_1, std::placeholders::_2)));
  }

  /** Called at the end of a transmission. */
  void handle_sent(const boost::system::error_code &ec,
		   std::size_t sent_size) {
    if (ec == boost::asio::error::operation_aborted) return; // cancelled
    if (ec) throw boost::system::system_error(ec);
    if (sent_size != last_pkt.size())
      throw std::runtime_error("Did not send all the packet");

    schedule_next_pkt();
  }

  /** Called after start(). */
  void handle_started() {
    BOOST_LOG_SEV(basic_lg, log::debug) << "Called handle_started";
    is_stopped_ = false;
    schedule_next_pkt();
    listen_for_acks();
  }

  /** Called after stop(). */
  void handle_stopped() {
    is_stopped_ = true;
    pkt_timer.cancel();
    socket_.cancel();
    BOOST_LOG(perf_lg) << "data_server::stopped sent_pkts="
		       << encoder_->total_coded_count();
    BOOST_LOG_SEV(basic_lg, log::debug) << "UDP server is stopped";

    // Call all handlers
    for (const auto &f : stop_handlers) {
      f(boost::system::error_code());
    }
    stop_handlers.clear();
  }
};

//	    data_client<Decoder,Sink> template definitions

template <class Decoder, class Sink>
data_client<Decoder,Sink>::data_client(boost::asio::io_service &io) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  io_service_(io),
  strand_(io_service_),
  socket_(io_service_),
  recv_buffer(UDP_MAX_PAYLOAD),
  ack_enabled(true),
  exp_count(0),
  is_stopped_(true),
  timeout_timer(io),
  timeout_(std::chrono::steady_clock::duration::zero()) {
}

template <class Decoder, class Sink>
void data_client<Decoder,Sink>::setup_decoder(const decoder_parameter_set &ps) {
  BOOST_LOG_SEV(basic_lg, log::trace) << "Setting up the decoder";
  decoder_ = std::make_unique<Decoder>(ps);
}

template <class Decoder, class Sink>
void data_client<Decoder,Sink>::setup_sink(const sink_parameter_set &ps) {
  BOOST_LOG_SEV(basic_lg, log::trace) << "Setting up the sink";
  sink_ = std::make_unique<Sink>(ps);
}

template <class Decoder, class Sink>
void data_client<Decoder,Sink>::bind(const std::string &service) {
  using namespace boost::asio::ip;
  using boost::asio::ip::udp;

  BOOST_LOG_SEV(basic_lg, log::trace) << "Resolving the client port";

  udp::resolver resolver(io_service_);
  udp::resolver::query listen_q(service);
  udp::endpoint ep = *resolver.resolve(listen_q);
  bind(ep.port()); // Take the first one
}

template <class Decoder, class Sink>
void data_client<Decoder,Sink>::bind(unsigned short port) {
  using namespace boost::asio::ip;
  using boost::asio::ip::udp;

  BOOST_LOG_SEV(basic_lg, log::trace) << "Binding the client UDP port";

  listen_endpoint_.port(port);
  listen_endpoint_.address(boost::asio::ip::address_v4::any());

  socket_.open(udp::v4());
  socket_.bind(listen_endpoint_);

  BOOST_LOG_SEV(basic_lg, log::info) << "Client bound to UDP: "
				     << socket_.local_endpoint();
}

template <class Decoder, class Sink>
void data_client<Decoder,Sink>::start_receive(const boost::asio::ip::udp::endpoint &src_ep) {
  using namespace std::placeholders;

  server_endpoint_ = src_ep;
  BOOST_LOG_SEV(basic_lg, log::info) << "Start receiving UDP data pkts from: "
				     << server_endpoint_;
  BOOST_LOG(perf_lg) << "data_client::start_receive";

  socket_.non_blocking(true); // Read without blocking in handle_received
  is_stopped_ = false;
  async_receive_pkt();
  reset_timer();
}

template <class Decoder, class Sink>
void data_client<Decoder,Sink>::start_receive(const std::string &src_addr,
					      unsigned short src_port) {
  using namespace boost::asio;
  ip::address a = ip::address::from_string(src_addr);
  start_receive(ip::udp::endpoint(a, src_port));
}

template <class Decoder, class Sink>
void data_client<Decoder,Sink>::start_receive(const std::string &src_addr,
					      const std::string &src_port) {
  using boost::asio::ip::udp;
  udp::resolver resolver(io_service_);
  udp::resolver::query query(udp::v4(), src_addr, src_port);
  auto i = resolver.resolve(query);
  if (i == udp::resolver::iterator()) {
    throw std::runtime_error("No endpoint found");
  }
  start_receive(*i); // pick the first found
}

template <class Decoder, class Sink>
void data_client<Decoder,Sink>::stop() {
  BOOST_LOG_SEV(basic_lg, log::info) << "Stopping the UDP client";
  strand_.dispatch(std::bind(&data_client::handle_stop, this));
}

template <class Decoder, class Sink>
boost::asio::ip::udp::endpoint data_client<Decoder,Sink>::client_endpoint() const {
  return socket_.local_endpoint();
}

template <class Decoder, class Sink>
void data_client<Decoder,Sink>::enable_ack(bool b) {
  ack_enabled = b;
}

template <class Decoder, class Sink>
bool data_client<Decoder,Sink>::is_ack_enabled() const {
  return ack_enabled;
}

template <class Decoder, class Sink>
void data_client<Decoder,Sink>::expected_count(std::size_t ec) {
  exp_count = ec;
}

template <class Decoder, class Sink>
std::size_t data_client<Decoder,Sink>::expected_count() const {
  return exp_count;
}

template <class Decoder, class Sink>
bool data_client<Decoder,Sink>::is_stopped() const {
  return is_stopped_;
};

template <class Decoder, class Sink>
void data_client<Decoder,Sink>::timeout(double t) {
  using namespace std::chrono;

  duration<double> td(t);
  timeout_ = duration_cast<steady_clock::duration>(td);
}

template <class Decoder, class Sink>
double data_client<Decoder,Sink>::timeout() const {
  using namespace std::chrono;

  return duration_cast<duration<double>>(timeout_.load()).count();
}

template <class Decoder, class Sink>
const Sink &data_client<Decoder,Sink>::sink() const {
  return *sink_;
}

template <class Decoder, class Sink>
const Decoder &data_client<Decoder,Sink>::decoder() const {
  return *decoder_;
}

template <class Decoder, class Sink>
void data_client<Decoder,Sink>::async_receive_pkt() {
  socket_.async_receive_from(boost::asio::buffer(recv_buffer),
			     server_endpoint_,
			     strand_.wrap(std::bind(&data_client::handle_received,
						    this,
						    std::placeholders::_1,
						    std::placeholders::_2)));
}

template <class Decoder, class Sink>
void data_client<Decoder,Sink>::reset_timer() {
  auto t = timeout_.load();
  if (t.count() > 0) {
    timeout_timer.expires_from_now(t);
    timeout_timer.async_wait(strand_.wrap(std::bind(&data_client::handle_timeout,
						    this,
						    std::placeholders::_1)));
  }
}

template <class Decoder, class Sink>
void data_client<Decoder,Sink>::schedule_ack(std::size_t blockno) {
  if (!ack_enabled) return;

  ack_buffer = build_raw_ack(blockno);
  socket_.async_send_to(boost::asio::buffer(ack_buffer),
			server_endpoint_,
			strand_.wrap(std::bind(&data_client::handle_sent_ack,
					       this,
					       std::placeholders::_1,
					       std::placeholders::_2)));
}

template <class Decoder, class Sink>
void data_client<Decoder,Sink>::handle_received(const boost::system::error_code& ec,
						std::size_t size) {
  if (ec == boost::asio::error::operation_aborted) return; // was cancelled
  if (ec) throw boost::system::system_error(ec);

  if (size == 0) throw std::runtime_error("Empty packet");

  std::list<fountain_packet> recv_list;

  // Insert first packet
  fountain_packet p;
  try {
    p = parse_raw_data_packet(recv_buffer);
  }
  catch (const std::runtime_error &e) {
    // should handle malformed packets
    throw e;
  }
  recv_list.push_back(std::move(p));

  // Read more packets if available
  while (socket_.available() > 0) {
    try {
      socket_.receive_from(boost::asio::buffer(recv_buffer),
			   server_endpoint_);
      p = parse_raw_data_packet(recv_buffer);
    }
    catch(const boost::system::system_error &e) {
      if (e.code() == boost::asio::error::would_block) {
	BOOST_LOG_SEV(basic_lg, log::warning) <<
	  "data_client socket would block after checking available";
	break;
      }
      else throw e;
    }
    catch (const std::runtime_error &e) {
      // should handle malformed packets
      throw e;
    }
    recv_list.push_back(std::move(p));
  }

  BOOST_LOG(perf_lg) << "data_client::handle_received received_count="
		     << recv_list.size();

  reset_timer();

  decoder_->push(std::make_move_iterator(recv_list.begin()),
		 std::make_move_iterator(recv_list.end()));

  // Extract eventual decoded packets
  while (*decoder_ && *sink_) {
    sink_->push(decoder_->next_decoded());
  }

  // ACK when the block is decoded
  if (decoder_->has_decoded()) {
    auto bnc = decoder_->block_number_counter();
    schedule_ack(bnc.next());
  }

  // Keep listening if not all packets have been decoded or failed
  if (exp_count == 0 ||
      (decoder_->total_decoded_count() +
       decoder_->total_failed_count()) < exp_count) {
    async_receive_pkt();
  }
  else {
    BOOST_LOG_SEV(basic_lg, log::info) << "Data client reached expected_count";
    stop();
  }
}

template <class Decoder, class Sink>
void data_client<Decoder, Sink>::handle_sent_ack(const boost::system::error_code& ec,
						 std::size_t size) {
  if (ec == boost::asio::error::operation_aborted) return; // was cancelled
  if (ec) throw boost::system::system_error(ec);
  if (size != ack_buffer.size()) throw std::runtime_error("Was not sent fully");
}

template <class Decoder, class Sink>
void data_client<Decoder, Sink>::handle_timeout(const boost::system::error_code& ec) {
  if (ec == boost::asio::error::operation_aborted) return; // was cancelled
  if (ec) throw boost::system::system_error(ec);

  // Assume all successive packets failed
  if (exp_count > 0) {
    std::size_t total_queued = decoder_->total_decoded_count() +
      decoder_->total_failed_count();
    std::size_t blocks_queued = total_queued / decoder_->K();
    // Flush also the current block (already enqueued but still waiting on it)
    if (decoder_->has_decoded()) --blocks_queued;
    std::size_t nblocks = std::ceil(static_cast<double>(exp_count) /
				    decoder_->K());
    decoder_->flush_n_blocks(nblocks - blocks_queued);

    // Extract eventual decoded packets
    while (*decoder_ && *sink_) {
      sink_->push(decoder_->next_decoded());
    }
  }

  BOOST_LOG_SEV(basic_lg, log::warning) << "Data client timed out";
  stop();
}

template <class Decoder, class Sink>
template <class H>
void data_client<Decoder, Sink>::add_stop_handler(const H &h) {
  stop_handlers.push_back(strand_.wrap(h));
}

template <class Decoder, class Sink>
std::size_t data_client<Decoder, Sink>::cancel_stop_handlers() {
  std::size_t cnt = 0;
  for (const auto &f : stop_handlers) {
    f(boost::asio::error::operation_aborted);
    ++cnt;
  }
  stop_handlers.clear();
  return cnt;
}

template <class Decoder, class Sink>
void data_client<Decoder, Sink>::handle_stop() {
  timeout_timer.cancel();
  socket_.cancel();
  socket_.close();
  is_stopped_ = true;
  BOOST_LOG(perf_lg) << "data_client::stopped"
		     << " received_pkts="
		     << decoder_->total_received_count()
		     << " decoded_pkts="
		     << decoder_->total_decoded_count()
		     << " failed_pkts="
		     << decoder_->total_failed_count()
		     << " avg_push_time="
		     << decoder_->average_push_time();

  BOOST_LOG_SEV(basic_lg, log::debug) << "UDP client is stopped";

  // Call all handlers
  for (const auto &f : stop_handlers) {
    f(boost::system::error_code());
  }
  stop_handlers.clear();
}

//	   data_server<Encoder,Source> template definitions

template <class Encoder, class Source>
template <class H>
void data_server<Encoder, Source>::add_stop_handler(const H &h) {
  stop_handlers.push_back(strand_.wrap(h));
}

template <class Encoder, class Source>
std::size_t data_server<Encoder, Source>::cancel_stop_handlers() {
  std::size_t cnt = 0;
  for (const auto &f : stop_handlers) {
    f(boost::asio::error::operation_aborted);
    ++cnt;
  }
  stop_handlers.clear();
  return cnt;
}



}}

#endif
