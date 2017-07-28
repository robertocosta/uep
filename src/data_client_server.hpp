#ifndef UEP_NET_DATA_CLIENT_SERVER_HPP
#define UEP_NET_DATA_CLIENT_SERVER_HPP

#include <chrono>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include "counter.hpp"
#include "packets_rw.hpp"

namespace uep {
namespace net {

/** Receive coded packets via a UDP socket.
 *
 *  This class listens on a socket for fountain_packets, passes them
 *  to an object of class Decoder, then passes the decoded packets to
 *  an object of class Sink.
 */
template <class Decoder, class Sink>
class data_client {
  /** Maximum payload size that can be carried by a UDP packet. */
  static const std::size_t UDP_MAX_PAYLOAD = 0x10000;

public:
  typedef Decoder decoder_type;
  typedef Sink sink_type;
  typedef typename Decoder::parameter_set decoder_parameter_set;
  typedef typename Sink::parameter_set sink_parameter_set;

  /** Construct a data_client tied to an io_service. This does not
   *  bind the socket yet.
   */
  explicit data_client(boost::asio::io_service &io) :
    io_service_(io),
    strand_(io_service_),
    socket_(io_service_),
    recv_buffer(UDP_MAX_PAYLOAD),
    ack_enabled(true),
    exp_count(0),
    is_stopped_(true) {
  }

  /** Replace the decoder object with one built from the given
   *  parameters.
   */
  void setup_decoder(const decoder_parameter_set &ps) {
    decoder_.reset(new Decoder(ps));
  }

  /** Replace the sink object with one built from the given
   *  parameters.
   */
  void setup_sink(const sink_parameter_set &ps) {
    sink_.reset(new Sink(ps));
  }

  /** Bind the socket to the given port. */
  void bind(const std::string &service) {
    using boost::asio::ip::udp;
    udp::resolver resolver(io_service_);
    udp::resolver::query listen_q(service);
    listen_endpoint_ = *resolver.resolve(listen_q);
    socket_.open(udp::v4());
    socket_.bind(listen_endpoint_);
  }

  /** Listen asynchronously for packets coming from a given remote
   *  (source) endpoint.
   */
  void start_receive(const boost::asio::ip::udp::endpoint &src_ep) {
    using namespace std::placeholders;
    server_endpoint_ = src_ep;
    is_stopped_ = false;
    socket_.async_receive_from(boost::asio::buffer(recv_buffer),
			       server_endpoint_,
			       std::bind(&data_client::handle_received,
					 this,
					 _1, _2));
  }

  /** Listen asynchronously for packets coming from a given remote
   *  (source) IP address and port.
   */
  void start_receive(const std::string &src_addr, unsigned short src_port) {
    using namespace boost::asio;
    ip::address a = ip::address::from_string(src_addr);
    start_receive(ip::udp::endpoint(a, src_port));
  }

  /** Schedule the stopping of the data_client. */
  void stop() {
    strand_.dispatch(std::bind(&data_client::handle_stop, this));
  }

  /** Enable or disable the sending of ACKs. */
  void enable_ack(bool b) {
    ack_enabled = b;
  }

  /** Return true when the sending of ACKs is enabled. */
  bool is_ack_enabled() const {
    return ack_enabled;
  }

  /** Set the number of packets to be received before stopping. */
  void expected_count(std::size_t ec) {
    exp_count = ec;
  }

  /** Get the number of packets to be received before stopping. */
  std::size_t expected_count() const {
    return exp_count;
  }

  /** True if the client is not listening for packets. */
  bool is_stopped() const {
    return is_stopped_;
  };

  /** Return a const reference to the sink object. */
  const Sink &sink() const {
    return *sink_;
  }

  /** Return a const reference to the decoder object. */
  const Decoder &decoder() const {
    return *decoder_;
  }

private:
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

  std::vector<char> recv_buffer; /**< Buffer to hold the last received
				  *   UDP payload.
				  */
  std::vector<char> ack_buffer; /**< Buffer to hold the raw ack during
				 *   the async transmission.
				 */

  std::atomic<bool> ack_enabled; /**< Set when the data_client should
				  *   send back ACKs.
				  */
  std::atomic<std::size_t> exp_count; /**< The number of packets that
				       *   should be received before
				       *   stopping the client. A
				       *   value of 0 means that the
				       *   client will never stop to
				       *   listen.
				       */
  bool is_stopped_;

  /** Schedule the transmission of an ACK to the server. */
  void schedule_ack(std::size_t blockno) {
    using namespace std::placeholders;

    if (!ack_enabled) return;

    ack_buffer = build_raw_ack(blockno);
    socket_.async_send_to(boost::asio::buffer(ack_buffer),
			  server_endpoint_,
			  strand_.wrap(std::bind(&data_client::handle_sent_ack,
						 this,
						 _1, _2)));
  }

  /** Called when a new packet is received. */
  void handle_received(const boost::system::error_code& ec, std::size_t size) {
    using std::move;
    using namespace std::placeholders;

    if (ec == boost::asio::error::operation_aborted) return; // was cancelled
    if (ec) throw boost::system::system_error(ec);
    if (size == 0) throw std::runtime_error("Empty packet");

    recv_buffer.resize(size);
    fountain_packet p;
    try {
      p = parse_raw_data_packet(recv_buffer);
    }
    catch (std::runtime_error e) {
      // should handle malformed packets
      throw e;
    }

    //std::size_t current_blockno = p.block_number();
    decoder_->push(move(p));

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
      socket_.async_receive_from(boost::asio::buffer(recv_buffer),
				 server_endpoint_,
				 std::bind(&data_client::handle_received,
					   this,
					   _1, _2));
    }
    else { // Stop the client
      stop();
    }
  }

  /** Called when the ACK has been sent. */
  void handle_sent_ack(const boost::system::error_code& ec, std::size_t size) {
    if (ec == boost::asio::error::operation_aborted) return; // was cancelled
    if (ec) throw boost::system::system_error(ec);
    if (size != ack_buffer.size()) throw std::runtime_error("Was not sent fully");
  }

  /** Called after stop(). */
  void handle_stop() {
    socket_.cancel();
    socket_.close();
    is_stopped_ = true;
  }
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
    encoder_.reset(new Encoder(ps));
  }

  /** Replace the source with a new one built using the given
   *  parameter set.
   */
  void setup_source(const source_parameter_set &ps) {
    source_.reset(new Source(ps));
  }

  /** Resolve the destination (client) endpoint and bind the socket. */
  void open(const std::string &dest_host, const std::string &dest_service) {
    using boost::asio::ip::udp;
    udp::resolver resolver(io_service_);
    udp::resolver::query query(udp::v4(), dest_host, dest_service);
    auto i = resolver.resolve(query);
    if (i == udp::resolver::iterator()) {
      throw std::runtime_error("No endpoint found");
    }
    client_endpoint_ = *i;
    socket_.open(udp::v4());
  }

  /** Schedule the start of a transmission toward the client endpoint. */
  void start() {
    strand_.dispatch(std::bind(&data_server::handle_started, this));
  }

  /** Schedule the stop of the current transmission. */
  void stop() {
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

  /** Return a const reference to the source object. */
  const Source &source() const {
    return *source_;
  }

  /** Return a const referece to the encoder object. */
  const Encoder &encoder() const {
    return *encoder_;
  }

private:
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

  bool is_stopped_; /**< Set when the data_server should not send
		     *	 packets.
		     */
  std::atomic<bool> ack_enabled; /**< Set when the data_server should
				  *   listen for incoming ACKs.
				  */
  std::atomic<std::size_t> max_per_block; /**< Maximum number of
					   *   packets that can be
					   *   generated for each
					   *   block before skipping
					   *   to the next.
					   */
  std::vector<char> last_pkt; /**< Last _raw_ coded packet generated
			       *   by the encoder.
			       */
  std::vector<char> last_ack; /**< Last _raw_ ack packet received. */
  std::chrono::steady_clock::time_point last_sent_time;
  boost::asio::steady_timer pkt_timer; /**< Timer used to schedule the
					*   packet transmissions.
					*/

  /** Encode the next packet and schedule its transmission according
   *  to the target send rate.
   */
  void schedule_next_pkt() {
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
      is_stopped_ = true;
      return;
    }

    // Check if the max number has been reached
    if (max_per_block <= encoder_->coded_count()) {
      encoder_->next_block();
      if (!*encoder_) { // The source did not have 2 blocks
	is_stopped_ = true;
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
						this, _1)));
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
						      _1, _2)));
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
						 _1, _2)));
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
    is_stopped_ = false;
    schedule_next_pkt();
    listen_for_acks();
  }

  /** Called after stop(). */
  void handle_stopped() {
    is_stopped_ = true;
    pkt_timer.cancel();
    socket_.cancel();
  }
};

}
}

#endif
